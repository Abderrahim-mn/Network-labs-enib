//----------------------------------------------------------------------------

#include "crsCuda.hpp"

#define ASSUMED_CACHELINE_SIZE 64

struct Point
{
  float px{}, py{}; // position
  float vx{}, vy{}; // velocity
  float r{}, g{}, b{}; // colour
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
  std::vector<Point> *points{};
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
  const int64_t count=crs::len(*sd.points);
  const int64_t workBegin=count*index/crs::len(sd.td);
  const int64_t workEnd=count*(index+1)/crs::len(sd.td);

  //---- compute point motion ----
  const float xAccel=sd.xAccel, yAccel=sd.yAccel, dt=sd.dt,
              xLimit=sd.xLimit, yLimit=sd.yLimit;
  Point * RESTRICT points=data(*sd.points);
  TRY_TO_VECTORISE
  for(int64_t id=workBegin; id<workEnd; ++id)
  {
    //-- read --
    float px=points[id].px, py=points[id].py;
    float vx=points[id].vx, vy=points[id].vy;
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
    points[id].px=px;
    points[id].py=py;
    points[id].vx=vx;
    points[id].vy=vy;
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
    TRY_TO_VECTORISE
    for(int64_t id=workBegin; id<workEnd; ++id)
    {
      //-- read --
      const float px=points[id].px, py=points[id].py;
      const float vx=points[id].vx, vy=points[id].vy;
      //-- compute --
      const float r=(px+xLimit)/(2.0f*xLimit);
      const float g=(py+yLimit)/(2.0f*yLimit);
      const float b=std::min(1.0f, sqrtf(vx*vx+vy*vy));
      //-- write --
      points[id].r=r;
      points[id].g=g;
      points[id].b=b;
    }
  }
}

//---- multi-CPU-based simulation of many animated points ----
void
cpuSimulation(const std::vector<std::string> &cmdLine,
              std::vector<Point> &points,
              float xAccel,
              float yAccel,
              float dt,
              float xLimit,
              float yLimit,
              int stepCount)
{
  crs::out("~~~~ %() ~~~~\n", __func__);
  const double t0=crs::gettimeofday();
  const int pointCount=crs::len(points);

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
              std::vector<Point> &points,
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
  struct Point
  {
    float px, py; // position
    float vx, vy; // velocity
    float r, g, b; // colour
  };

  //
  // ... À COMPLÉTER {1} ...
  //
  // Déclarer comme paramètres :
  // - un buffer de points que nous consulterons et modifierons pour
  //   ajuster les positions et vitesses du tableau ``points'',
  // - un entier ``count''  désignant le nombre total de points,
  // - deux réels ``xLimit'' et ``yLimit'' désignant les limites des
  //   positions des points selon les axes x et y,
  // - deux réels ``xAccel'' et ``yAccel'' désignant le vecteur
  //   accélération que subissent les points,
  // - un réel ``dt'' désignant le pas de temps de l'intégration numérique.
  //
  extern "C" __global__
  void
  motion( /* ...
             ... */ )
  {
    const int gridSize=blockDim.x*gridDim.x;
    const int globalId=blockDim.x*blockIdx.x+threadIdx.x;
    //
    // ... À COMPLÉTER {2} ...
    //
    // Réaliser la boucle qui assure que l'ensemble des indices de
    // calcul traitent bien le nombre total de points (paramètre ``count'').
    // * Obtenir depuis le buffer la position et la vitesse du point
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
    // * Inscrire enfin dans le buffer les nouvelles position et vitesse
    //   du point à l'indice courant.
    //

    // ...
  }
)RAW_CUDA_CODE"};
  assertSuccess(motionProgram);

  //---- generate and build the colour GPU-program ----
  crs::CudaProgram colourProgram{device, "colour",
R"RAW_CUDA_CODE(
  struct Point
  {
    float px, py; // position
    float vx, vy; // velocity
    float r, g, b; // colour
  };

  //
  // ... À COMPLÉTER {3} ...
  //
  // Déclarer comme paramètres :
  // - un buffer de points que nous consulterons et modifierons pour
  //   ajuster les couleurs du tableau ``points'',
  // - un entier ``count''  désignant le nombre total de points,
  // - deux réels ``xLimit'' et ``yLimit'' désignant les limites des
  //   positions des points selon les axes x et y,
  //
  extern "C" __global__
  void
  colour( /* ...
             ... */ )
  {
    const int gridSize=blockDim.x*gridDim.x;
    const int globalId=blockDim.x*blockIdx.x+threadIdx.x;
    //
    // ... À COMPLÉTER {4} ...
    //
    // Réaliser la boucle qui assure que l'ensemble des indices de
    // calcul traitent bien le nombre total de points (paramètre ``count'').
    // * Obtenir depuis le buffer la position et la vitesse du point
    //   à l'indice courant.
    // * Calculer les composantes rouge et verte selon la formule :
    //     composante = (position + limite) / (2.0f * limite)
    //   (considérer l'axe x pour la rouge et l'axe y pour la verte)
    // * Calculer la composante bleue selon la formule :
    //     composante = min(1.0f, sqrtf(vx*vx+vy*vy))
    //     (vx et vy représentant le vecteur vitesse du point considéré)
    // * Inscrire enfin dans le buffer la nouvelle couleur
    //   du point à l'indice courant.
    //

    // ...
  }
)RAW_CUDA_CODE"};
  assertSuccess(colourProgram);

  //---- allocate suitable GPU-buffer ----
  const int pointCount=crs::len(points);
  //
  // ... À COMPLÉTER {5} ...
  //
  // Créer sur le GPU le buffer nécessaire aux programmes.
  // Il doit être dimensionné pour contenir les mêmes données que le
  // vecteur ``points''.
  //

  // ...

  //---- create GPU-command stream ----
  crs::CudaStream stream{device};

  //---- transfer data from host to GPU ----
  const double t1=crs::gettimeofday();
  const double e1=crs::cpuEnergy();
  //
  // ... À COMPLÉTER {6} ...
  //
  // Rendre le contenu de ``points'' disponible dans le buffer
  // correspondant sur le GPU.
  //

  // ...

  for(int i=0; i<stepCount; ++i)
  {
    //---- launch motion GPU-program with its prepared parameters ----
    //
    // ... À COMPLÉTER {7} ...
    //
    // Préciser à ``motionProgram'' le buffer qu'il doit utiliser ainsi que
    // les paramètres ``pointCount'', ``xLimit'', ``yLimit'',
    // ``xAccel'', ``yAccel'' et ``dt'',
    // et lancer enfin son exécution.
    //

    // ...
    if(!(i%10)) // once out of ten times
    {
      //---- launch colour GPU-program with its prepared parameters ----
      //
      // ... À COMPLÉTER {8} ...
      //
      // Préciser à ``colourProgram'' le buffer qu'il doit utiliser ainsi que
      // les paramètres ``pointCount'', ``xLimit'' et ``yLimit'',
      // et lancer enfin son exécution.
      //

      // ...
    }
  }

  //---- transfer data from GPU to host ----
  //
  // ... À COMPLÉTER {9} ...
  //
  // Obtenir le contenu de ``points'' depuis le buffer
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
  std::vector<Point> points;
  for(int i=0; i<pointCount; ++i)
  {
    points.emplace_back(Point{2.0f*xLimit*(0.5f-float(std::rand())/
                                                float(RAND_MAX)),
                              2.0f*yLimit*(0.5f-float(std::rand())/
                                                float(RAND_MAX)),
                              2.0f*(0.5f-float(std::rand())/
                                         float(RAND_MAX)),
                              2.0f*(0.5f-float(std::rand())/
                                         float(RAND_MAX)),
                              0.0f, 0.0f, 0.0f});
  }

  //---- use multi-CPU if requested ----
  std::vector<Point> cpuPoints;
  if(useCpu)
  {
    cpuPoints=useGpu ? points : std::move(points);
    cpuSimulation(args, cpuPoints,
                  xAccel, yAccel, dt, xLimit, yLimit, stepCount);
  }

  //---- use GPU if requested ----
  std::vector<Point> gpuPoints;
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
      const double dx=cpuPoints[i].px-gpuPoints[i].px;
      const double dy=cpuPoints[i].py-gpuPoints[i].py;
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
