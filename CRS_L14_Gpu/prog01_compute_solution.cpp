//----------------------------------------------------------------------------

#include "crsCuda.hpp"

#define ASSUMED_CACHELINE_SIZE 64

//---- some data to be kept with each thread ----
struct ThreadData
{
  std::thread th{};
  std::array<uint32_t, 2> seed{};
  int64_t insideCount{};
  // ensure at least two _aligned_ cache-lines between ``hot data''
  uint8_t padding[3*ASSUMED_CACHELINE_SIZE]{};
};

//---- some data common to every thread ----
struct SharedData
{
  std::vector<ThreadData> td{};
  std::vector<crs::CpuInfo> cpuInfo{};
  int count{};
};

//---- simple random generator ----
float // random value in [0.0;1.0]
rnd(std::array<uint32_t, 2> &seed)
{
  //  Adapted from MWC generator described in George Marsaglia's post
  //  ``Random numbers for C: End, at last?'' on sci.stat.math,
  //  sci.math, sci.math.num-analysis, sci.crypt, sci.physics.research,
  //  comp.os.msdos.djgpp (Thu, 21 Jan 1999 03:08:52 GMT)
  seed[0]=36969*(seed[0]&0x0000FFFFU)+(seed[0]>>16);
  seed[1]=18000*(seed[1]&0x0000FFFFU)+(seed[1]>>16);
  const uint32_t value=(seed[0]<<16)+seed[1];
  const float div=1.0f/8388607.0f; // 23-bit mantissa
  return float(value&0x007FFFFFU)*div;
}

//---- the actual work done by each thread ----
void
threadTask(int index,
           SharedData &sd)
{
  //---- access thread-specific data ----
  ThreadData &td=sd.td[index];

  //---- prevent this thread from migrating to another CPU ----
  crs::bindCurrentThreadToCpu(sd.cpuInfo[index].cpuId);

  //---- determine the part of the work to be done by this thread ----
  const int count=sd.count;
  const int workBegin=count*index/crs::len(sd.td);
  const int workEnd=count*(index+1)/crs::len(sd.td);

  //---- compute ----
  auto seed=td.seed;
  int64_t insideCount=0;
  for(int id=workBegin; id<workEnd; ++id)
  {
    for(int repeat=0; repeat<1000; ++repeat)
    {
      const float x=rnd(seed);
      const float y=rnd(seed);
      if((x*x+y*y)<=1.0f)
      {
        ++insideCount;
      }
    }
  }
  td.insideCount=insideCount;
}

//---- multi-CPU-based Monte-Carlo estimation of PI ----
void
cpuEstimation(const std::vector<std::string> &cmdLine,
              int count)
{
  crs::out("~~~~ %() ~~~~\n", __func__);
  const double t0=crs::gettimeofday();

  //---- prepare data for parallel work ----
  SharedData sd;
  const bool enableSmt=crs::find(cmdLine, "smt")!=-1;
  sd.cpuInfo=crs::detectCpuInfo(enableSmt);
  const int threadCount=crs::len(sd.cpuInfo);
  crs::out("using % threads\n", threadCount);
  sd.td.resize(threadCount);
  sd.count=count;

  const double t1=crs::gettimeofday();
  const double e1=crs::cpuEnergy();

  //---- launch parallel work ----
  for(int i=0;i<threadCount;++i)
  {
    sd.td[i].seed[0]=(uint32_t(std::rand())<<16)+std::rand();
    sd.td[i].seed[1]=(uint32_t(std::rand())<<16)+std::rand();
    sd.td[i].th=std::thread{threadTask, i, std::ref(sd)};
  }

  //---- wait for parallel work to terminate and collect results ----
  int64_t insideCount=0;
  for(int i=0; i<threadCount; ++i)
  {
    sd.td[i].th.join();
    insideCount+=sd.td[i].insideCount;
  }

  //---- display performances ----
  const double duration=crs::gettimeofday()-t1;
  const double energy=crs::cpuEnergy()-e1;
  const int64_t pointCount=1000*int64_t(count);
  const double estimatedPi=double(insideCount)*4.0/double(pointCount);
  const double delta=estimatedPi-M_PI;
  crs::out("CPU initialisation: % seconds\n"
           "CPU computation:    % seconds (% Joules)\n"
           "% points, % inside, estimated=%%%, delta=%\n",
           t1-t0, duration, energy,
           pointCount, insideCount,
           std::setprecision(12), estimatedPi, std::setprecision(6), delta);
}

//---- GPU-based Monte-Carlo estimation of PI ----
void
gpuEstimation(const std::vector<std::string> &cmdLine,
              int count)
{
  crs::out("~~~~ %() ~~~~\n", __func__);
  const double t0=crs::gettimeofday();

  //---- detect and use first GPU device ----
  crs::CudaPlatform platform;
  const auto &device=platform.device(0);

  //---- choose GPU-program layout ----
  const auto [blockSize, blockCount]=chooseLayout(device);

  //---- prepare host data ----
  const int gridSize=blockSize*blockCount;
  std::vector<int32_t> hostCounts(gridSize);

  //---- generate and build GPU-program ----
  crs::CudaProgram program{device, "estimatePi",
R"RAW_CUDA_CODE(
  uint2 // initial seed for random generation
  makeRndSeed(int gridSize,
              int globalId,
              float hostSeed0,
              float hostSeed1,
              float hostSeed2,
              float hostSeed3)
  {
    const float i=float(globalId+1)/float(gridSize);
    const float4 d=make_float4(hostSeed0*12.9898f+        i*78.233f,
                                       i*12.9898f+hostSeed1*78.233f,
                                       i*12.9898f+hostSeed2*78.233f,
                               hostSeed3*12.9898f+        i*78.233f);
    const float pi=acosf(-1.0f);
    const float4 s=make_float4(sinf(fmodf(d.x, pi))*43758.5453f,
                               sinf(fmodf(d.y, pi))*43758.5453f,
                               sinf(fmodf(d.z, pi))*43758.5453f,
                               sinf(fmodf(d.w, pi))*43758.5453f);
    const uint4 f=make_uint4(65535.0f*(s.x-floorf(s.x)),
                             65535.0f*(s.y-floorf(s.y)),
                             65535.0f*(s.z-floorf(s.z)),
                             65535.0f*(s.w-floorf(s.w)));
    return make_uint2((f.x<<16)+f.z,
                      (f.y<<16)+f.w);
  }

  float // random value in [0.0f;1.0f]
  rnd(uint2 &seed)
  {
    seed.x=36969*(seed.x&0x0000FFFF)+(seed.x>>16);
    seed.y=18000*(seed.y&0x0000FFFF)+(seed.y>>16);
    const unsigned int value=(seed.x<<16)+seed.y;
    const float div=1.0f/8388607.0f; // 23-bit mantissa
    return (value&0x007FFFFF)*div;
  }

  //
  // ... À COMPLÉTER {1} ...
  //
  // Déclarer comme paramètres :
  // - un buffer d'entiers ``counts'' dans lequel nous écrirons pour fournir
  //   les comptes des points aléatoires figurant dans le quart de disque,
  // - un entier ``count'' désignant le nombre total de milliers de points
  //   aléatoires à tirer,
  // - quatre réels ``hostSeed0'', ``hostSeed1'', ``hostSeed2'', ``hostSeed3''
  //   servant à fabriquer la graine du générateur pseudo-aléatoire.
  //
  extern "C" __global__
  void
  estimatePi(int *counts,
             int count,
             float hostSeed0,
             float hostSeed1,
             float hostSeed2,
             float hostSeed3)
  {
    const int gridSize=blockDim.x*gridDim.x;
    const int globalId=blockDim.x*blockIdx.x+threadIdx.x;
    uint2 seed=makeRndSeed(gridSize, globalId,
                           hostSeed0, hostSeed1, hostSeed2, hostSeed3);
    //
    // ... À COMPLÉTER {2} ...
    //
    // Réaliser la boucle qui assure que l'ensemble des indices de calcul
    // traitent bien le nombre total de milliers de points aléatoires à tirer
    // (paramètre ``count'').
    // * À chaque indice de calcul il faudra effectuer mille tirages de
    //   points aléatoires
    // * Chaque point aléatoire sera obtenu par deux appels à ``rnd(seed)''
    // * Compter à chaque fois que le produit scalaire du point avec
    //   lui-même est inférieur ou égal à ``1.0'' (dans le quart de disque).
    // Inscrire enfin dans le buffer ``counts'' le compte obtenu par l'indice
    // de calcul ``globalId''.
    //
    int insideCount=0;
    for(int id=globalId; id<count; id+=gridSize)
    {
      for(int repeat=0; repeat<1000; ++repeat)
      {
        const float x=rnd(seed);
        const float y=rnd(seed);
        if(x*x+y*y<=1.0f)
        {
          ++insideCount;
        }
      }
    }
    counts[globalId]=insideCount;
    // ...
  }
)RAW_CUDA_CODE"};
  assertSuccess(program);

  //---- allocate suitable GPU-buffer ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // Créer sur le GPU le buffer nécessaire au programme.
  // Il doit être dimensionné pour contenir les mêmes
  // données que le tableau ``hostCounts''.
  //
  crs::CudaBuffer<int32_t> gpuCounts(device, gridSize);
  // ...

  //---- create GPU-command stream ----
  crs::CudaStream stream{device};

  //---- launch GPU-program with its prepared parameters ----
  const double t1=crs::gettimeofday();
  const double e1=crs::cpuEnergy();
  const float hostSeed0=float(std::rand())/float(RAND_MAX);
  const float hostSeed1=float(std::rand())/float(RAND_MAX);
  const float hostSeed2=float(std::rand())/float(RAND_MAX);
  const float hostSeed3=float(std::rand())/float(RAND_MAX);
  //
  // ... À COMPLÉTER {4} ...
  // 
  // Préciser à ``program'' le buffer qu'il doit utiliser ainsi que le
  // paramètre ``count'' et les quatre réels aléatoires ``hostSeed?'',
  // et lancer enfin son exécution.
  //
  const void *gpuArgs[]={gpuCounts.programArg(),
                         &count,
                         &hostSeed0,
                         &hostSeed1,
                         &hostSeed2,
                         &hostSeed3};
  program.launch(stream, blockCount, blockSize, 0, gpuArgs);
  // ...

  //---- transfer data from GPU to host ----
  //
  // ... À COMPLÉTER {5} ...
  //
  // Obtenir le contenu de ``hostCounts'' depuis le buffer
  // correspondant sur le GPU.
  //
  gpuCounts.toHost(stream, data(hostCounts));
  // ...

  //---- wait for GPU-commands to finish ----
  stream.hostSync();

  //---- make use of computed data ----
  int64_t insideCount=0;
  for(int i=0; i<gridSize; ++i)
  {
    insideCount+=hostCounts[i];
  }

  //---- display performances ----
  const double duration=crs::gettimeofday()-t1;
  const double energy=crs::cpuEnergy()-e1;
  const double gpu_energy=duration*device.power();
  const int64_t pointCount=1000*int64_t(count);
  const double estimatedPi=double(insideCount)*4.0/double(pointCount);
  const double delta=estimatedPi-M_PI;
  crs::out("GPU initialisation: % seconds\n"
           "GPU computation:    % seconds (% Joules + % Joules on CPU)\n"
           "% points, % inside, estimated=%%%, delta=%\n",
           t1-t0, duration, gpu_energy, energy,
           pointCount, insideCount,
           std::setprecision(12), estimatedPi, std::setprecision(6), delta);
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- prepare application data ----
  const bool useCpu=crs::find(args, "cpu")!=-1;
  const bool useGpu=!useCpu||(crs::find(args, "gpu")!=-1);
  const bool big=crs::find(args, "big")!=-1;
  const int count=big ? 10'000'000 : 1'000'000;
  std::srand(int(std::time(nullptr)));

  //---- use multi-CPU if requested ----
  if(useCpu)
  {
    cpuEstimation(args, count);
  }

  //---- use GPU if requested ----
  if(useGpu)
  {
    gpuEstimation(args, count);
  }

  return 0;
}

//----------------------------------------------------------------------------
