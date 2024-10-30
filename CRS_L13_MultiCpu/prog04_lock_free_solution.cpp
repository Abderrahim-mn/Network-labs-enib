//----------------------------------------------------------------------------

#include "image.hpp"

#define ASSUMED_CACHELINE_SIZE 64

//---- some data to be kept around each thread ----
struct ThreadData
{
  std::thread th{};
  int histoR[256]{};
  int histoG[256]{};
  int histoB[256]{};
  // ensure at least two _aligned_ cache-lines between ``hot data''
  uint8_t padding[3*ASSUMED_CACHELINE_SIZE]{};
};

//---- some data common to every thread ----
struct SharedData
{
  std::vector<ThreadData> td{};
  std::vector<crs::CpuInfo> cpuInfo{};
  const Pixel *image{};
  int width{}, height{};
  Pixel *result{};
  std::atomic<int> step{0};
  std::atomic<int> inCurrentStep{0};
  std::atomic<int> inHistogram{0};
  volatile bool done{false};
};

//---- the actual work done by each thread ----
void
threadWork(int index,
           SharedData &sd)
{
  //---- access thread-specific data ----
  ThreadData &td=sd.td[index];

  //---- determine the part of the work to be done by this thread ----
  const int64_t size=sd.width*sd.height;
  const int64_t workBegin=size*index/crs::len(sd.td);
  const int64_t workEnd=size*(index+1)/crs::len(sd.td);

  //---- compute histogram ----
  ::memset(td.histoR, 0, 256*sizeof(*td.histoR));
  ::memset(td.histoG, 0, 256*sizeof(*td.histoG));
  ::memset(td.histoB, 0, 256*sizeof(*td.histoB));
  for(int64_t i=workBegin; i<workEnd; ++i)
  {
    const Pixel p=sd.image[i];
    ++td.histoR[p.r()];
    ++td.histoG[p.g()];
    ++td.histoB[p.b()];
  }

  //---- signal and wait for end of histogram ----
  //
  // ... À COMPLÉTER {8} ...
  //
  // Décrémenter le champ atomique ``inHistogram'' de la structure partagée
  // ``sd''.
  // Si le résultat de cette décrémentation n'est pas nul, alors il faut
  // réaliser une boucle d'attente active jusqu'à ce que ce même champ atomique
  // devienne nul.
  //
  if(--sd.inHistogram>0)
  {
    while(sd.inHistogram)
    {
      // busy-wait
    }
  }
  // ...

  //---- accumulate and equalise histogram ----
  const double norm=255.0/double(size);
  int histoR[256]={0};
  int histoG[256]={0};
  int histoB[256]={0};
  for(const auto &t: sd.td)
  {
    for(int prevR=0, prevG=0, prevB=0, i=0; i<256; ++i)
    {
      histoR[i]+=int(norm*(prevR+=t.histoR[i]));
      histoG[i]+=int(norm*(prevG+=t.histoG[i]));
      histoB[i]+=int(norm*(prevB+=t.histoB[i]));
    }
  }

  //---- adjust intensities ----
  for(int64_t i=workBegin; i<workEnd; ++i)
  {
    const Pixel p=sd.image[i];
    sd.result[i].r()=uint8_t(histoR[p.r()]);
    sd.result[i].g()=uint8_t(histoG[p.g()]);
    sd.result[i].b()=uint8_t(histoB[p.b()]);
  }
}

//---- the background thread task ----
void
threadTask(int index,
           SharedData &sd)
{
  //---- prevent this thread from migrating to another CPU ----
  crs::bindCurrentThreadToCpu(sd.cpuInfo[index].cpuId);

  for(int lastStep=0;;)
  {
    //---- wait for a new step ----
    //
    // ... À COMPLÉTER {6} ...
    //
    // Réaliser une boucle d'attente active jusqu'à ce que le champ atomique
    // ``step'' de la structure partagée ``sd'' ait une valeur différente de
    // celle de la variable locale ``lastStep''.
    // Mettre alors à jour la variable locale ``lastStep''.
    // Si après cette boucle le champ ``done'' de la structure partagée ``sd''
    // vaut ``true'' il faut alors quitter la boucle de traitement (``break'').
    //
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
    // ...

    //---- perform actual work for this step ----
    threadWork(index, sd);

    //---- signal end of this step ----
    //
    // ... À COMPLÉTER {7} ...
    //
    // Décrémenter le champ atomique ``inCurrentStep'' de la structure partagée
    // ``sd''.
    //
    --sd.inCurrentStep;
    // ...
  }
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- prepare images and storage for result ----
  ImageSequence seq{args};
  std::vector<Pixel> storage;
  const double t0=crs::gettimeofday();
  const double e0=crs::cpuEnergy();

  //---- prepare data for parallel work ----
  SharedData sd;
  const bool enableSmt=crs::find(args, "smt")!=-1;
  sd.cpuInfo=crs::detectCpuInfo(enableSmt);
  for(const auto &info: sd.cpuInfo)
  {
    crs::out("pkg %, core %, cpu %\n", info.pkgId, info.coreId, info.cpuId);
  }
  const int threadCount=crs::len(sd.cpuInfo);
  sd.td.resize(threadCount);

  //---- prevent main thread from migrating to another CPU ----
  crs::bindCurrentThreadToCpu(sd.cpuInfo[0].cpuId);

  //---- launch background threads once for all ----
  //
  // ... À COMPLÉTER {1} ...
  //
  // Pour chacun des ``threadCount'' éléments de ``sd.td''
  // _SAUF_ _LE_ _PREMIER_ (commencer à l'indice ``1''),
  // initialiser le champ ``th'' avec un thread qui exécute la fonction
  // ``threadTask()''.
  // Celle-ci doit recevoir, en plus d'un indice numérotant les threads,
  // une _référence_ sur la donnée partagée ``sd'' à laquelle les threads
  // accéderont tous.
  //
  for(int i=1; i<threadCount; ++i) // skip thread 0 (main)
  {
    sd.td[i].th=std::thread{threadTask, i, std::ref(sd)};
  }
  // ...

  //---- process every image in the sequence ----
  for(int i=0; i<3000; ++i)
  {
    std::tie(sd.image, sd.width, sd.height)=seq.next();

    //---- ensure storage is large enough for result ----
    const int size=sd.width*sd.height;
    if(size>crs::len(storage))
    {
      storage.resize(size);
    }
    sd.result=data(storage);

    //---- start a new step ----
    //
    // ... À COMPLÉTER {3} ...
    //
    // Maintenant que les les champs ``image'', ``width'', ``height'' et
    // ``result'' de la structure partagée ``sd'' viennent d'être actualisés
    // pour cette nouvelle image de la séquence, il faut initialiser les
    // champs atomiques ``inCurrentStep'' et ``inHistogram'' de la
    // structure partagée ``sd'' pour que les barrières de synchronisation
    // fonctionnent correctement.
    // Puis, en tout dernier, incrémenter le champ atomique ``step'' de la
    // structure partagée ``sd''.
    //
    sd.inCurrentStep=threadCount;
    sd.inHistogram=threadCount;
    ++sd.step; // threads synchronise on this last change
    // ...

    //---- work as thread 0 ----
    threadWork(0, sd);

    //---- wait for this step to end ----
    //
    // ... À COMPLÉTER {4} ...
    //
    // Décrémenter le champ atomique ``inCurrentStep'' de la structure partagée
    // ``sd'' et réaliser une boucle d'attente active jusqu'à ce que ce champ
    // devienne nul.
    //
    --sd.inCurrentStep;
    while(sd.inCurrentStep)
    {
      // busy-wait
    }
    // ...
  }

  //---- signal end of work ----
  //
  // ... À COMPLÉTER {5} ...
  //
  // Passer à ``true'' le champ ``done'' de la structure partagée ``sd''.
  // Puis, en tout dernier, incrémenter le champ atomique ``step'' de la
  // structure partagée ``sd''.
  //
  sd.done=true;
  ++sd.step;
  // ...

  //---- wait for background threads to terminate ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // Parcourir ``sd.td'' afin d'effectuer l'opération ``join()'' sur chacun
  // des threads qu'il désigne
  // _SAUF_ _LE_ _PREMIER_ (commencer à l'indice ``1''),
  //
  for(int i=1; i<threadCount; ++i) // skip thread 0 (main)
  {
    sd.td[i].th.join();
  }
  // ...

  //---- display performances ----
  const double duration=crs::gettimeofday()-t0;
  const double energy=crs::cpuEnergy()-e0;
  crs::out("% images in % seconds (% per second, % Joules)\n",
           seq.current(), duration, seq.current()/duration, energy);

  //---- save last result (to check correctness) ----
  if(sd.result)
  {
    saveImage(sd.result, sd.width, sd.height, "output_last.ppm");
  }

  return 0;
}

//----------------------------------------------------------------------------
