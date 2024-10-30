//----------------------------------------------------------------------------

#include "crsCuda.hpp"

#define ASSUMED_CACHELINE_SIZE 64

//---- some data to be kept with each thread ----
struct ThreadData
{
  std::thread th{};
  // ensure at least two _aligned_ cache-lines between ``hot data''
  uint8_t padding[3*ASSUMED_CACHELINE_SIZE]{};
};

//---- some data common to every thread ----
struct SharedData
{
  std::vector<ThreadData> td{};
  std::vector<crs::CpuInfo> cpuInfo{};
  float xFactor{};
  float yFactor{};
  int elemCount{};
  const float *X{};
  const float *Y{};
  float *Z{};
};

//---- the actual work done by each thread ----
void
threadTask(int index,
           SharedData &sd)
{
  //---- prevent this thread from migrating to another CPU ----
  crs::bindCurrentThreadToCpu(sd.cpuInfo[index].cpuId);

  //---- determine the part of the work to be done by this thread ----
  const int64_t count=sd.elemCount;
  const int64_t workBegin=count*index/crs::len(sd.td);
  const int64_t workEnd=count*(index+1)/crs::len(sd.td);

  //---- compute ----
  const float xFactor=sd.xFactor;
  const float yFactor=sd.yFactor;
  const float * RESTRICT X=sd.X;
  const float * RESTRICT Y=sd.Y;
  float * RESTRICT Z=sd.Z;
  TRY_TO_VECTORISE
  for(int64_t id=workBegin; id<workEnd; ++id)
  {
    Z[id]=xFactor*X[id]+yFactor*Y[id];
  }
}

//---- multi-CPU-based linear combination of two vectors ----
void
cpuLinearCombination(const std::vector<std::string> &cmdLine,
                     float xFactor,
                     float yFactor,
                     const std::vector<float> &X,
                     const std::vector<float> &Y,
                     std::vector<float> &Z)
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
  sd.elemCount=crs::len(Z);
  sd.xFactor=xFactor;
  sd.yFactor=yFactor;
  sd.X=data(X);
  sd.Y=data(Y);
  sd.Z=data(Z);

  const double t1=crs::gettimeofday();
  const double e1=crs::cpuEnergy();

  //---- launch parallel work ----
  for(int i=0; i<threadCount; ++i)
  {
    sd.td[i].th=std::thread{threadTask, i, std::ref(sd)};
  }

  //---- wait for parallel work to terminate ----
  for(int i=0; i<threadCount; ++i)
  {
    sd.td[i].th.join();
  }

  //---- display performances ----
  const double duration=crs::gettimeofday()-t1;
  const double energy=crs::cpuEnergy()-e1;
  crs::out("CPU initialisation: % seconds\n"
           "CPU computation:    % seconds (% Joules)\n"
           "% elements\n",
           t1-t0, duration, energy,
           sd.elemCount);
}

//---- GPU-based linear combination of two vectors ----
void
gpuLinearCombination(const std::vector<std::string> &cmdLine,
                     float xFactor,
                     float yFactor,
                     const std::vector<float> &X,
                     const std::vector<float> &Y,
                     std::vector<float> &Z)
{
  crs::out("~~~~ %() ~~~~\n", __func__);
  const double t0=crs::gettimeofday();

  //---- detect and use first GPU device ----
  crs::CudaPlatform platform;
  const auto &device=platform.device(0);

  //---- choose GPU-program layout ----
  const auto [blockSize, blockCount]=chooseLayout(device);

  //---- generate and build GPU-program ----
  crs::CudaProgram program{device, "linearCombination",
R"RAW_CUDA_CODE(
  //
  // ... À COMPLÉTER {1} ...
  //
  // Déclarer comme paramètres :
  // - deux buffers de réels que nous consulterons pour obtenir les valeurs
  //   des vecteurs ``X'' et ``Y'',
  // - un buffer de réels dans lequel nous écrirons les valeurs calculées du
  //   vecteur ``Z'',
  // - un entier ``count''  désignant le nombre total d'éléments de chaque
  //   vecteur,
  // - deux réels ``xFactor'' et ``yFactor'' désignant les facteurs
  //   multiplicatifs pour ``X'' et ``Y''.
  //
  extern "C" __global__
  void
  linearCombination( /* ...
                        ... */ )
  {
    const int gridSize=blockDim.x*gridDim.x;
    const int globalId=blockDim.x*blockIdx.x+threadIdx.x;
    //
    // ... À COMPLÉTER {2} ...
    //
    // Réaliser la boucle qui assure que l'ensemble des indices de calcul
    // traitent bien le nombre total d'éléments (paramètre ``count'').
    // Chaque itération réalisera dans ``Z'', à l'indice courant, la
    // combinaison linéaire de ``X'' et ``Y'' selon les facteurs reçus en
    // paramètres.
    //

    // ...
  }
)RAW_CUDA_CODE"};
  assertSuccess(program);

  //---- allocate suitable GPU-buffers ----
  const int elemCount=crs::len(Z);
  //
  // ... À COMPLÉTER {3} ...
  //
  // Créer sur le GPU les trois buffers nécessaires au programme.
  // Ils doivent être dimensionnés pour contenir les mêmes données que les
  // vecteurs ``X'', ``Y'' et ``Z''.
  //

  // ...

  //---- create GPU-command stream ----
  crs::CudaStream stream{device};

  //---- transfer data from host to GPU ----
  const double t1=crs::gettimeofday();
  const double e1=crs::cpuEnergy();
  //
  // ... À COMPLÉTER {4} ...
  //
  // Rendre le contenu de ``X'' et ``Y'' disponible dans les buffers
  // correspondants sur le GPU.
  //

  // ...

  //---- launch GPU-program with its prepared parameters ----
  //
  // ... À COMPLÉTER {5} ...
  //
  // Préciser à ``program'' les trois buffers qu'il doit utiliser ainsi que
  // les paramètres ``elemCount'', ``xFactor'' et ``yFactor'',
  // et lancer enfin son exécution.
  //

  // ...

  //---- transfer data from GPU to host ----
  //
  // ... À COMPLÉTER {6} ...
  //
  // Obtenir le contenu de ``Z'' depuis le buffer
  // correspondant sur le GPU.
  //

  // ...

  //---- wait for GPU-commands to finish ----
  stream.hostSync();

  //---- display performances ----
  const double duration=crs::gettimeofday()-t1;
  const double energy=crs::cpuEnergy()-e1;
  const double gpu_energy=duration*device.power();
  crs::out("GPU initialisation: % seconds\n"
           "GPU computation:    % seconds (% Joules + % Joules on CPU)\n"
           "% elements\n",
           t1-t0, duration, gpu_energy, energy,
           elemCount);
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
  const int elemCount=big ? 50'000'000 : 1'000'000;
  std::srand(int(std::time(nullptr)));
  const float xFactor=1.41f, yFactor=0.35f;
  std::vector<float> X(elemCount), Y(elemCount);
  for(int i=0; i<elemCount; ++i)
  {
    X[i]=float(std::rand())/float(RAND_MAX);
    Y[i]=float(std::rand())/float(RAND_MAX);
  }

  //---- use multi-CPU if requested ----
  std::vector<float> cpuZ;
  if(useCpu)
  {
    cpuZ.resize(elemCount);
    cpuLinearCombination(args, xFactor, yFactor, X, Y, cpuZ);
  }

  //---- use GPU if requested ----
  std::vector<float> gpuZ;
  if(useGpu)
  {
    gpuZ.resize(elemCount);
    gpuLinearCombination(args, xFactor, yFactor, X, Y, gpuZ);
  }

  //---- check CPU/GPU consistency ----
  if(useCpu&&useGpu)
  {
    int mismatchCount=0;
    for(int i=0; i<elemCount; ++i)
    {
      const double dz=cpuZ[i]-gpuZ[i];
      const double epsilon=1e-6;
      if(dz*dz>epsilon*epsilon)
      {
        ++mismatchCount;
      }
    }
    if(mismatchCount)
    {
      crs::out("!!! CPU/GPU mismatch for % elements !!!\n", mismatchCount);
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
