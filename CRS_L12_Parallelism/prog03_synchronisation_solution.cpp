//----------------------------------------------------------------------------

#include "crsUtils.hpp"

#define USE_MUTEX  0
#define USE_ATOMIC 1

static_assert(!(USE_MUTEX&&USE_ATOMIC),
              "USE_MUTEX and USE_ATOMIC are mutually exclusive");

//---- some data to be kept around each thread ----
struct ThreadData
{
  std::thread th{};
  int result{};
};

//---- some data common to every thread ----
struct SharedData
{
  std::vector<ThreadData> td{};

#if USE_MUTEX

  volatile int count{0};
  std::mutex mtx{};

#elif USE_ATOMIC

  std::atomic_int count{0};

#else // unsynchronised

  volatile int count{0};

#endif
};

//---- the actual work done by each thread ----
void
threadTask(int index,
           SharedData &sd)
{
  //---- access thread-specific data ----
  ThreadData &td=sd.td[index];

  //---- accumulate many values in a local count and in the shared one ----
  int count=0;
  for(int iter=0; iter<10'000'000; ++iter)
  {
    const int value=iter%(2+index);
    count+=value; // accumulate in local count

#if USE_MUTEX

    //
    // ... À COMPLÉTER {3} ...
    //
    // Réaliser la même accumulation que dans le cas non synchronisé
    // mais en prenant soin de l'encadrer par une section critique
    // (méthodes ``lock()'' et ``unlock()'' du verrou ``sd.mtx'').
    //
    sd.mtx.lock();
    sd.count+=value; // accumulate in shared count (a usual integer)
    sd.mtx.unlock();
    // ...

#elif USE_ATOMIC

    //
    // ... À COMPLÉTER {4} ...
    //
    // Réaliser la même accumulation que dans le cas non synchronisé.
    // (le code C++ est identique, seul le type de la variable est différent)
    //
    sd.count+=value; // accumulate in shared count (an _atomic_ integer)
    // ...

#else // unsynchronised

    sd.count+=value; // accumulate in shared count (a usual integer)

#endif
  }

  //---- make local count available to the main program ----
  td.result=count;
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

#if USE_MUTEX
  crs::out("mutex-based version\n");
#elif USE_ATOMIC
  crs::out("atomic-based version\n");
#else // unsynchronised
  crs::out("!!! unsynchronised version !!!\n");
#endif

  //---- determine number of threads ----
  int threadCount=-1;
  if(crs::len(args)>1) // chosen on the command line
  {
    crs::extract(args[1], threadCount);
  }
  if(threadCount<=0) // or deduced from number of CPU
  {
    threadCount=std::thread::hardware_concurrency();
  }

  //---- prepare data for parallel work ----
  double t0=crs::gettimeofday();
  SharedData sd;
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
  for(int i=0; i<threadCount; ++i)
  {
    sd.td[i].th=std::thread{threadTask, i, std::ref(sd)};
  }
  // ...

  //---- wait for threads to terminate and collect results ----
  crs::out("waiting for % threads\n", threadCount);
  int localCount=0;
  //
  // ... À COMPLÉTER {2} ...
  //
  // Parcourir ``sd.td'' afin d'effectuer l'opération ``join()'' sur chacun
  // des threads qu'il désigne.
  // Après chacune de ces opérations, il est envisageable de récupérer le
  // champ ``result'' qui était associé au thread concerné puisque ce
  // dernier a terminé son traitement.
  // Afficher alors cette valeur et accumulez la (``+='') dans la variable
  // ``localCount''.
  //
  for(int i=0; i<threadCount; ++i)
  {
    sd.td[i].th.join();
    const int result=sd.td[i].result;
    crs::out("result[%]=%\n", i, result);
    localCount+=result;
  }
  // ...

  //---- check consistency ----
  int sharedCount=sd.count;
  crs::out("done in % s, localCount=%, sharedCount=%%\n",
           crs::gettimeofday()-t0, localCount, sharedCount,
           sharedCount!=localCount ? " !!!" : "");

  return 0;
}

//----------------------------------------------------------------------------
