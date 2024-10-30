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
  int histoR[256]{};
  int histoG[256]{};
  int histoB[256]{};
  std::mutex mtx{};
  std::condition_variable_any cnd{};
  volatile int inHistogram{0};
  volatile bool histogramEqualised{false};
};

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

  //---- signal end of histogram ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // Décrémenter ``sd.inHistogram''.
  // Si le résultat est nul, le signaler avec la variable condition ``sd.cnd''.
  // Tout ceci sous le contrôle du verrou ``sd.mtx''.
  //
  sd.mtx.lock();
  if(!--sd.inHistogram)
  {
    sd.cnd.notify_all();
  }
  sd.mtx.unlock();
  // ...

  if(index==0) // pure serial work, done by thread 0 only
  {
    //---- wait for end of histogram ----
    //
    // ... À COMPLÉTER {4} ...
    //
    // Attendre avec la variable condition ``sd.cnd'' que
    // ``sd.inHistogram'' devienne nul.
    // Tout ceci sous le contrôle du verrou ``sd.mtx''.
    //
    sd.mtx.lock();
    while(sd.inHistogram)
    {
      sd.cnd.wait(sd.mtx);
    }
    sd.mtx.unlock();
    // ...

    //---- accumulate and equalise histograms from all threads ----
    const double norm=255.0/double(size);
    ::memset(sd.histoR, 0, 256*sizeof(*td.histoR));
    ::memset(sd.histoG, 0, 256*sizeof(*td.histoG));
    ::memset(sd.histoB, 0, 256*sizeof(*td.histoB));
    for(const auto &t: sd.td)
    {
      for(int prevR=0, prevG=0, prevB=0, i=0; i<256; ++i)
      {
        sd.histoR[i]+=int(norm*(prevR+=t.histoR[i]));
        sd.histoG[i]+=int(norm*(prevG+=t.histoG[i]));
        sd.histoB[i]+=int(norm*(prevB+=t.histoB[i]));
      }
    }

    //---- signal end of equalised histogram ----
    //
    // ... À COMPLÉTER {5} ...
    //
    // Changer l'état de ``sd.histogramEqualised'' et le signaler avec la
    // variable condition ``sd.cnd''.
    // Tout ceci sous le contrôle du verrou ``sd.mtx''.
    //
    sd.mtx.lock();
    sd.histogramEqualised=true;
    sd.cnd.notify_all();
    sd.mtx.unlock();
    // ...
  }
  else // other threads have nothing more to do than waiting
  {
    //---- wait for equalised histogram ----
    //
    // ... À COMPLÉTER {6} ...
    //
    // Attendre avec la variable condition ``sd.cnd'' que
    // ``sd.histogramEqualised'' passe à ``true''.
    // Tout ceci sous le contrôle du verrou ``sd.mtx''.
    //
    sd.mtx.lock();
    while(!sd.histogramEqualised)
    {
      sd.cnd.wait(sd.mtx);
    }
    sd.mtx.unlock();
    // ...
  }

  //---- adjust intensities ----
  for(int64_t i=workBegin; i<workEnd; ++i)
  {
    const Pixel p=sd.image[i];
    sd.result[i].r()=uint8_t(sd.histoR[p.r()]);
    sd.result[i].g()=uint8_t(sd.histoG[p.g()]);
    sd.result[i].b()=uint8_t(sd.histoB[p.b()]);
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

    //---- launch parallel work ----
    //
    // ... À COMPLÉTER {1} ...
    //
    // Maintenant que les les champs ``image'', ``width'', ``height'' et
    // ``result'' de la structure partagée ``sd'' viennent d'être actualisés
    // pour cette nouvelle image de la séquence, il faut initialiser les
    // champs ``inHistogram'' et ``histogramEqualised'' de la structure
    // partagée ``sd'' pour que les barrières de synchronisation fonctionnent
    // correctement.
    // Puis, il faut initialiser le champ ``th'' de chacun des
    // ``threadCount'' éléments de ``sd.td'' avec un thread qui exécute la
    // fonction ``threadTask()''.
    // Celle-ci doit recevoir, en plus d'un indice numérotant les threads,
    // une _référence_ sur la donnée partagée ``sd'' à laquelle les threads
    // accéderont tous.
    //
    sd.inHistogram=threadCount;
    sd.histogramEqualised=false;
    for(int i=0; i<threadCount; ++i)
    {
      sd.td[i].th=std::thread{threadTask, i, std::ref(sd)};
    }
    // ...

    //---- wait for parallel work to terminate ----
    //
    // ... À COMPLÉTER {2} ...
    //
    // Parcourir ``sd.td'' afin d'effectuer l'opération ``join()'' sur chacun
    // des threads qu'il désigne.
    //
    for(int i=0; i<threadCount; ++i)
    {
      sd.td[i].th.join();
    }
    // ...
  }

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
