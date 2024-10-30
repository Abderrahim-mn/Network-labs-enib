//----------------------------------------------------------------------------

#include "crsUtils.hpp"

#define USE_LOCAL_COPY 0
#define USE_PADDING    0

static_assert(!(USE_PADDING&&USE_LOCAL_COPY),
              "USE_PADDING and USE_LOCAL_COPY are mutually exclusive");

#define ASSUMED_CACHELINE_SIZE 64

//---- some data to be kept around each thread ----
struct ThreadData
{
  std::thread th{};
  std::array<int, 8> workSpace{};

#if USE_PADDING

  // ensure at least two _aligned_ cache-lines between ``hot data''
  uint8_t padding[3*ASSUMED_CACHELINE_SIZE];

#endif
};

//---- some data common to every thread ----
struct SharedData
{
  std::vector<ThreadData> td{};
  std::vector<crs::CpuInfo> cpuInfo{};
  int64_t workAmount{};
};

//---- the actual work done by each thread ----
void
threadTask(int index,
           SharedData &sd)
{
  ThreadData &td=sd.td[index];

  //---- prevent this thread from migrating to another CPU ----
  crs::bindCurrentThreadToCpu(sd.cpuInfo[index].cpuId);

  //---- determine the part of the work to be done by this thread ----
  const int64_t count=sd.workAmount;
  const int64_t workBegin=count*index/crs::len(sd.td);
  const int64_t workEnd=count*(index+1)/crs::len(sd.td);

  //---- perform the actual work ----
  const int workSpaceSize=crs::len(td.workSpace);
#if USE_LOCAL_COPY

  //
  // ... À COMPLÉTER {3} ...
  //
  // Réaliser la même boucle de calcul que dans le cas ``in-place'', mais en
  // prenant soin :
  // * d'effectuer une copie préalable (``='') du champ ``td.workSpace'' dans
  //   une variable locale,
  // * de faire les calculs sur cette variable locale,
  // * de recopier (``='') cette variable locale vers le champ
  //   ``td.workSpace'' une fois la boucle de calcul terminée.
  //

  // ...

#else // in-place

  for(int64_t w=workBegin; w<workEnd; ++w)
  {
    ++td.workSpace[w%workSpaceSize];
  }

#endif
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

#if USE_LOCAL_COPY
  crs::out("copy-based version\n");
#elif USE_PADDING
  crs::out("padded version\n");
#else // compact
  crs::out("compact version\n");
#endif

  //---- prepare data for parallel work ----
  const double t0=crs::gettimeofday();
  const double e0=crs::cpuEnergy();
  SharedData sd;
  sd.workAmount=10'000'000'000;
  const bool enableSmt=crs::find(args, "smt")!=-1;
  sd.cpuInfo=crs::detectCpuInfo(enableSmt);
  for(const auto &info: sd.cpuInfo)
  {
    crs::out("pkg %, core %, cpu %\n", info.pkgId, info.coreId, info.cpuId);
  }
  const int threadCount=crs::len(sd.cpuInfo);
  sd.td.resize(threadCount);

  //---- launch threads ----
  crs::out("launching % threads\n", threadCount);
  //
  // ... À COMPLÉTER {1} ...
  //
  // Pour chacun des ``threadCount'' éléments de ``sd.td'', initialiser le
  // champ ``th'' avec un thread qui exécute la fonction ``threadTask()''.
  // Celle-ci doit recevoir, en plus d'un indice numérotant les threads,
  // une _référence_ sur la donnée partagée ``sd'' à laquelle les threads
  // accéderont tous.
  //

  // ...

  //---- wait for threads to terminate ----
  crs::out("waiting for % threads\n", threadCount);
  int64_t result=0;
  //
  // ... À COMPLÉTER {2} ...
  //
  // Parcourir ``sd.td'' afin d'effectuer l'opération ``join()'' sur chacun
  // des threads qu'il désigne.
  // Après chacune de ces opérations, il est envisageable d'accumuler (``+='')
  // dans ``result'' tous les éléments du champ ``workSpace'' qui était associé
  // au thread concerné puisque ce dernier a terminé son traitement.
  //

  // ...

  //---- check consistency ----
  if(result!=sd.workAmount)
  {
    crs::out("!!! computation mismatch: %!=% !!!\n", result, sd.workAmount);
  }

  //---- display performances ----
  const double duration=crs::gettimeofday()-t0;
  const double energy=crs::cpuEnergy()-e0;
  crs::out("% iterations in % seconds (% it/s per cpu, % Joules)\n",
           result, duration,
           double(result)/duration/crs::len(sd.cpuInfo), energy);

  return 0;
}

//----------------------------------------------------------------------------
