//----------------------------------------------------------------------------

#include "crsCuda.hpp"

#define ASSUMED_CACHELINE_SIZE 64

struct Point_SOA
{
  std::vector<float> px{}, py{}; // positions
  std::vector<float> vx{}, vy{}; // velocities
  std::vector<float> r{}, g{}, b{}; // colours
};

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
  Point_SOA *points{};
  float xAccel{}, yAccel{}, dt{}, xLimit{}, yLimit{};
  std::atomic<int> step{0};
  std::atomic<int> inCurrentStep{0};
  std::atomic<int> inMotion{0};
  volatile bool done{false};
};

//---- the actual work to be done in each thread ----
void
threadWork(int index,
           SharedData &sd)
{
  //---- determine the part of the work to be done by this thread ----
  const int64_t count=crs::len(sd.points->px);
  const int64_t workBegin=count*index/crs::len(sd.td);
  const int64_t workEnd=count*(index+1)/crs::len(sd.td);

  //---- compute point motion ----
  const float xAccel=sd.xAccel, yAccel=sd.yAccel, dt=sd.dt,
              xLimit=sd.xLimit, yLimit=sd.yLimit;
  float * RESTRICT soa_px=data(sd.points->px);
  float * RESTRICT soa_py=data(sd.points->py);
  float * RESTRICT soa_vx=data(sd.points->vx);
  float * RESTRICT soa_vy=data(sd.points->vy);
  TRY_TO_VECTORISE
  for(int64_t id=workBegin; id<workEnd; ++id)
  {
    //-- read --
    float px=soa_px[id], py=soa_py[id];
    float vx=soa_vx[id], vy=soa_vy[id];
    //-- compute --
    px+=(vx+xAccel*0.5f*dt)*dt;
    py+=(vy+yAccel*0.5f*dt)*dt;
    vx+=xAccel*dt;
    vy+=yAccel*dt;
    if(px>xLimit)
    {
      px=xLimit;
      if(vx>=0.0f)
      {
        vx=-vx;
      }
    }
    else if(px<-xLimit)
    {
      px=-xLimit;
      if(vx<=0.0f)
      {
        vx=-vx;
      }
    }
    if(py>yLimit)
    {
      py=yLimit;
      if(vy>=0.0f)
      {
        vy=-vy;
      }
    }
    else if(py<-yLimit)
    {
      py=-yLimit;
      if(vy<=0.0f)
      {
        vy=-vy;
      }
    }
    //-- write --
    soa_px[id]=px;
    soa_py[id]=py;
    soa_vx[id]=vx;
    soa_vy[id]=vy;
  }

  if(!(sd.step%10))
  {
    //---- signal and wait for end of motion ----
    if(--sd.inMotion>0)
    {
      while(sd.inMotion)
      {
        // busy-wait
      }
    }

    //---- compute point colour ----
    float * RESTRICT soa_r=data(sd.points->r);
    float * RESTRICT soa_g=data(sd.points->g);
    float * RESTRICT soa_b=data(sd.points->b);
    TRY_TO_VECTORISE
    for(int64_t id=workBegin;id<workEnd;++id)
    {
      //-- read --
      const float px=soa_px[id], py=soa_py[id];
      const float vx=soa_vx[id], vy=soa_vy[id];
      //-- compute --
      const float r=(px+xLimit)/(2.0f*xLimit);
      const float g=(py+yLimit)/(2.0f*yLimit);
      const float b=std::min(1.0f, sqrtf(vx*vx+vy*vy));
      //-- write --
      soa_r[id]=r;
      soa_g[id]=g;
      soa_b[id]=b;
    }
  }
}

//---- multi-CPU-based simulation of many animated points ----
void
cpuSimulation(const std::vector<std::string> &cmdLine,
              Point_SOA &points,
              float xAccel,
              float yAccel,
              float dt,
              float xLimit,
              float yLimit,
              int stepCount)
{
  crs::out("~~~~ %() ~~~~\n", __func__);
  const double t0=crs::gettimeofday();
  const int pointCount=crs::len(points.px);

  //---- prepare data for parallel work ----
  SharedData sd;
  const bool enableSmt=crs::find(cmdLine, "smt")!=-1;
  sd.cpuInfo=crs::detectCpuInfo(enableSmt);
  const int threadCount=crs::len(sd.cpuInfo);
  crs::out("using % threads\n", threadCount);
  sd.td.resize(threadCount);

  //---- prevent main thread from migrating to another CPU ----
  crs::bindCurrentThreadToCpu(sd.cpuInfo[0].cpuId);

  //---- launch background threads once for all ----
  for(int i=1; i<threadCount; ++i) // skip thread 0 (main)
  {
    sd.td[i].th=std::thread{[index=i, &sd]()
      {
        //---- prevent this thread from migrating to another CPU ----
        crs::bindCurrentThreadToCpu(sd.cpuInfo[index].cpuId);

        for(int lastStep=0;;)
        {
          //---- wait for a new step ----
          int step;
          while(lastStep==(step=sd.step))
          {
            // busy-wait
          }
          lastStep=step;
          if(sd.done)
          {
            break;
          }

          //---- perform actual work for this step ----
          threadWork(index, sd);

          //---- signal end of this step ----
          --sd.inCurrentStep;
        }
      }};
  }

  const double t1=crs::gettimeofday();
  const double e1=crs::cpuEnergy();

  for(int i=0; i<stepCount; ++i)
  {
    //---- start a new step ----
    sd.points=&points;
    sd.xAccel=xAccel;
    sd.yAccel=yAccel;
    sd.dt=dt;
    sd.xLimit=xLimit;
    sd.yLimit=yLimit;
    sd.inCurrentStep=threadCount-1;
    sd.inMotion=threadCount;
    ++sd.step; // threads synchronise on this last change

    //---- work as thread 0 ----
    threadWork(0, sd);

    //---- wait for this step to end ----
    while(sd.inCurrentStep)
    {
      // busy-wait
    }
  }

  //---- signal end of work ----
  sd.done=true;
  ++sd.step;

  //---- wait for background threads to terminate ----
  for(int i=1; i<threadCount; ++i) // skip thread 0 (main)
  {
    sd.td[i].th.join();
  }

  //---- display performances ----
  const double duration=crs::gettimeofday()-t1;
  const double energy=crs::cpuEnergy()-e1;
  crs::out("CPU initialisation: % seconds\n"
           "CPU computation:    % seconds (% Joules)\n"
           "--> % Mp/s\n",
           t1-t0, duration, energy,
           1e-6*pointCount*stepCount/duration);
}

//---- GPU-based simulation of many animated points ----
void
gpuSimulation(const std::vector<std::string> &cmdLine,
              Point_SOA &points,
              float xAccel,
              float yAccel,
              float dt,
              float xLimit,
              float yLimit,
              int stepCount)
{
  crs::out("~~~~ %() ~~~~\n", __func__);
  const double t0=crs::gettimeofday();

  //---- detect and use first GPU device ----
  crs::CudaPlatform platform;
  const auto &device=platform.device(0);

  //---- choose GPU-program layout ----
  const auto [blockSize, blockCount]=chooseLayout(device);

  //---- generate and build the motion GPU-program ----
  crs::CudaProgram motionProgram{device, "motion",
R"RAW_CUDA_CODE(
  //
  // ... À COMPLÉTER {1} ...
  //
  // Déclarer comme paramètres :
  // - quatre buffers de réels que nous consulterons et modifierons pour
  //   ajuster les positions et vitesses des tableaux
  //   ``points.px'', ``points.py'', ``points.vx'', ``points.vy'',
  // - un entier ``count''  désignant le nombre total de points,
  // - deux réels ``xLimit'' et ``yLimit'' désignant les limites des
  //   positions des points selon les axes x et y,
  // - deux réels ``xAccel'' et ``yAccel'' désignant le vecteur
  //   accélération que subissent les points,
  // - un réel ``dt'' désignant le pas de temps de l'intégration numérique.
  //
  extern "C" __global__
  void
  motion(float *soa_px,
         float *soa_py,
         float *soa_vx,
         float *soa_vy,
         int count,
         float xLimit,
         float yLimit,
         float xAccel,
         float yAccel,
         float dt)
  {
    const int gridSize=blockDim.x*gridDim.x;
    const int globalId=blockDim.x*blockIdx.x+threadIdx.x;
    //
    // ... À COMPLÉTER {2} ...
    //
    // Réaliser la boucle qui assure que l'ensemble des indices de
    // calcul traitent bien le nombre total de points (paramètre ``count'').
    // * Obtenir depuis les buffers la position et la vitesse du point
    //   à l'indice courant.
    // * Modifier la position selon la formule :
    //     position += (vitesse + accélération*0.5f*dt) * dt
    // * Modifier la vitesse selon la formule :
    //     vitesse += accélération * dt
    // * Rester dans les limites imposées selon le principe :
    //     si position > limite
    //       position = limite
    //       si vitesse > 0.0f, vitesse = - vitesse
    //     sinon si position < - limite
    //       position = - limite
    //       si vitesse < 0.0f, vitesse = - vitesse
    //   (à effectuer indépendamment sur les axes x et y)
    // * Inscrire enfin dans les buffers les nouvelles position et vitesse
    //   du point à l'indice courant.
    //
    // nb: il s'agit de la même chose qu'à la question précédente ; seules
    //     la lecture et l'écriture dans les buffers diffèrent.
    //
    for(int id=globalId; id<count; id+=gridSize)
    {
      //-- read --
      float px=soa_px[id];
      float py=soa_py[id];
      float vx=soa_vx[id];
      float vy=soa_vy[id];
      //-- compute --
      px+=(vx+xAccel*0.5f*dt)*dt;
      py+=(vy+yAccel*0.5f*dt)*dt;
      vx+=xAccel*dt;
      vy+=yAccel*dt;
      if(px>xLimit)
      {
        px=xLimit;
        if(vx>=0.0f)
        {
          vx=-vx;
        }
      }
      else if(px<-xLimit)
      {
        px=-xLimit;
        if(vx<=0.0f)
        {
          vx=-vx;
        }
      }
      if(py>yLimit)
      {
        py=yLimit;
        if(vy>=0.0f)
        {
          vy=-vy;
        }
      }
      else if(py<-yLimit)
      {
        py=-yLimit;
        if(vy<=0.0f)
        {
          vy=-vy;
        }
      }
      //-- write --
      soa_px[id]=px;
      soa_py[id]=py;
      soa_vx[id]=vx;
      soa_vy[id]=vy;
    }
    // ...
  }
)RAW_CUDA_CODE"};
  assertSuccess(motionProgram);

  //---- generate and build the colour GPU-program ----
  crs::CudaProgram colourProgram{device, "colour",
R"RAW_CUDA_CODE(
  //
  // ... À COMPLÉTER {3} ...
  //
  // Déclarer comme paramètres :
  // - quatre buffers de réels que nous consulterons pour connaître les
  //   positions et vitesses des tableaux
  //   ``points.px'', ``points.py'', ``points.vx'', ``points.vy'',
  // - trois buffers de réels dans lequel nous écrirons les couleurs
  //   des tableaux ``points.r'', ``points.g'', ``points.b'',
  // - un entier ``count''  désignant le nombre total de points,
  // - deux réels ``xLimit'' et ``yLimit'' désignant les limites des
  //   positions des points selon les axes x et y,
  //
  extern "C" __global__
  void
  colour(const float *soa_px,
         const float *soa_py,
         const float *soa_vx,
         const float *soa_vy,
         float *soa_r,
         float *soa_g,
         float *soa_b,
         int count,
         float xLimit,
         float yLimit)
  {
    const int gridSize=blockDim.x*gridDim.x;
    const int globalId=blockDim.x*blockIdx.x+threadIdx.x;
    //
    // ... À COMPLÉTER {4} ...
    //
    // Réaliser la boucle qui assure que l'ensemble des indices de
    // calcul traitent bien le nombre total de points (paramètre ``count'').
    // * Obtenir depuis les buffers la position et la vitesse du point
    //   à l'indice courant.
    // * Calculer les composantes rouge et verte selon la formule :
    //     composante = (position + limite) / (2.0f * limite)
    //   (considérer l'axe x pour la rouge et l'axe y pour la verte)
    // * Calculer la composante bleue selon la formule :
    //     composante = min(1.0f, sqrtf(vx*vx+vy*vy))
    //     (vx et vy représentant le vecteur vitesse du point considéré)
    // * Inscrire enfin dans les buffers la nouvelle couleur
    //   du point à l'indice courant.
    //
    // nb: il s'agit de la même chose qu'à la question précédente ; seules
    //     la lecture et l'écriture dans les buffers diffèrent.
    //
    for(int id=globalId; id<count; id+=gridSize)
    {
      //-- read --
      const float px=soa_px[id];
      const float py=soa_py[id];
      const float vx=soa_vx[id];
      const float vy=soa_vy[id];
      //-- compute --
      const float r=(px+xLimit)/(2.0f*xLimit);
      const float g=(py+yLimit)/(2.0f*yLimit);
      const float b=min(1.0f, sqrtf(vx*vx+vy*vy));
      //-- write --
      soa_r[id]=r;
      soa_g[id]=g;
      soa_b[id]=b;
    }
    // ...
  }
)RAW_CUDA_CODE"};
  assertSuccess(colourProgram);

  //---- allocate suitable GPU-buffers ----
  const int pointCount=crs::len(points.px);
  //
  // ... À COMPLÉTER {5} ...
  //
  // Créer sur le GPU les buffers nécessaires aux programmes.
  // Ils doivent être dimensionnés pour contenir les mêmes données que les
  // vecteurs de ``points''.
  //
  crs::CudaBuffer<float> gpuPx(device, pointCount);
  crs::CudaBuffer<float> gpuPy(device, pointCount);
  crs::CudaBuffer<float> gpuVx(device, pointCount);
  crs::CudaBuffer<float> gpuVy(device, pointCount);
  crs::CudaBuffer<float> gpuR(device, pointCount);
  crs::CudaBuffer<float> gpuG(device, pointCount);
  crs::CudaBuffer<float> gpuB(device, pointCount);
  // ...

  //---- create GPU-command stream ----
  crs::CudaStream stream{device};

  //---- transfer data from host to GPU ----
  const double t1=crs::gettimeofday();
  const double e1=crs::cpuEnergy();
  //
  // ... À COMPLÉTER {6} ...
  //
  // Rendre le contenu des vecteurs de ``points'' disponible dans les buffers
  // correspondants sur le GPU.
  //
  gpuPx.fromHost(stream, data(points.px));
  gpuPy.fromHost(stream, data(points.py));
  gpuVx.fromHost(stream, data(points.vx));
  gpuVy.fromHost(stream, data(points.vy));
  gpuR.fromHost(stream, data(points.r));
  gpuG.fromHost(stream, data(points.g));
  gpuB.fromHost(stream, data(points.b));
  // ...

  for(int i=0; i<stepCount; ++i)
  {
    //---- launch motion GPU-program with its prepared parameters ----
    //
    // ... À COMPLÉTER {7} ...
    //
    // Préciser à ``motionProgram'' les buffers qu'il doit utiliser ainsi que
    // les paramètres ``pointCount'', ``xLimit'', ``yLimit'',
    // ``xAccel'', ``yAccel'' et ``dt'',
    // et lancer enfin son exécution.
    //
    const void *gpuArgs[]={gpuPx.programArg(),
                           gpuPy.programArg(),
                           gpuVx.programArg(),
                           gpuVy.programArg(),
                           &pointCount,
                           &xLimit, &yLimit,
                           &xAccel, &yAccel,
                           &dt};
    motionProgram.launch(stream, blockCount, blockSize, 0, gpuArgs);
    // ...
    if(!(i%10)) // once out of ten times
    {
      //---- launch colour GPU-program with its prepared parameters ----
      //
      // ... À COMPLÉTER {8} ...
      //
      // Préciser à ``colourProgram'' les buffers qu'il doit utiliser ainsi que
      // les paramètres ``pointCount'', ``xLimit'' et ``yLimit'',
      // et lancer enfin son exécution.
      //
      const void *gpuArgs[]={gpuPx.programArg(),
                             gpuPy.programArg(),
                             gpuVx.programArg(),
                             gpuVy.programArg(),
                             gpuR.programArg(),
                             gpuG.programArg(),
                             gpuB.programArg(),
                             &pointCount,
                             &xLimit, &yLimit};
      colourProgram.launch(stream, blockCount, blockSize, 0, gpuArgs);
      // ...
    }
  }

  //---- transfer data from GPU to host ----
  //
  // ... À COMPLÉTER {9} ...
  //
  // Obtenir le contenu des vecteurs de ``points'' depuis les buffers
  // correspondants sur le GPU.
  //
  gpuPx.toHost(stream, data(points.px));
  gpuPy.toHost(stream, data(points.py));
  gpuVx.toHost(stream, data(points.vx));
  gpuVy.toHost(stream, data(points.vy));
  gpuR.toHost(stream, data(points.r));
  gpuG.toHost(stream, data(points.g));
  gpuB.toHost(stream, data(points.b));
  // ...

  //---- wait for GPU-commands to finish ----
  stream.hostSync();

  //---- display performances ----
  const double duration=crs::gettimeofday()-t1;
  const double energy=crs::cpuEnergy()-e1;
  const double gpu_energy=duration*device.power();
  crs::out("GPU initialisation: % seconds\n"
           "GPU computation:    % seconds (% Joules + % Joules on CPU)\n"
           "--> % Mp/s\n",
           t1-t0, duration, gpu_energy, energy,
           1e-6*pointCount*stepCount/duration);
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
  const int pointCount=big ? 5'000'000 : 100'000;
  const int stepCount=big ? 200 : 10'000;
  std::srand(int(std::time(nullptr)));
  const float angle=M_2PIf*float(std::rand())/float(RAND_MAX);
  const float xAccel=cosf(angle);
  const float yAccel=sinf(angle);
  const float dt=0.0001f;
  const float xLimit=1.0f;
  const float yLimit=0.5f;
  crs::out("% points, % steps\n", pointCount, stepCount);
  Point_SOA points;
  for(int i=0; i<pointCount; ++i)
  {
    points.px.emplace_back(2.0f*xLimit*(0.5f-float(std::rand())/
                                             float(RAND_MAX)));
    points.py.emplace_back(2.0f*yLimit*(0.5f-float(std::rand())/
                                             float(RAND_MAX)));
    points.vx.emplace_back(2.0f*(0.5f-float(std::rand())/
                                      float(RAND_MAX)));
    points.vy.emplace_back(2.0f*(0.5f-float(std::rand())/
                                      float(RAND_MAX)));
    points.r.emplace_back(0.0f);
    points.g.emplace_back(0.0f);
    points.b.emplace_back(0.0f);
  }

  //---- use multi-CPU if requested ----
  Point_SOA cpuPoints;
  if(useCpu)
  {
    cpuPoints=useGpu ? points : std::move(points);
    cpuSimulation(args, cpuPoints,
                  xAccel, yAccel, dt, xLimit, yLimit, stepCount);
  }

  //---- use GPU if requested ----
  Point_SOA gpuPoints;
  if(useGpu)
  {
    gpuPoints=std::move(points);
    gpuSimulation(args, gpuPoints,
                  xAccel, yAccel, dt, xLimit, yLimit, stepCount);
  }

  //---- check CPU/GPU consistency ----
  if(useCpu&&useGpu)
  {
    int mismatchCount=0;
    for(int i=0; i<pointCount; ++i)
    {
      const double dx=cpuPoints.px[i]-gpuPoints.px[i];
      const double dy=cpuPoints.py[i]-gpuPoints.py[i];
      const double epsilon=1e-3;
      if((dx*dx+dy*dy)>epsilon*epsilon)
      {
        ++mismatchCount;
      }
    }
    if(mismatchCount)
    {
      crs::out("!!! CPU/GPU mismatch for % points !!!\n", mismatchCount);
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
