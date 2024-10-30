//----------------------------------------------------------------------------

#include "crsUtils.hpp"

#define USE_SYNCHRO 0

//---- a trivial queue type for the purpose of this exercise ----
template<typename T,
         int N>
class TinyFifo
{
public:

  void
  enqueue(T v)
  {
    d_[w_]=std::move(v);
    w_=(w_+1)%N;
  }

  T
  dequeue()
  {
    T v{d_[r_]};
    r_=(r_+1)%N;
    return v;
  }
  
  int
  size() const
  {
    return (N+r_-w_)%N;
  }

  int
  capacity() const
  {
    return N-1;
  }

private:
  std::array<volatile T, N> d_{};
  volatile int r_{0}, w_{0};
};

template<typename T,
         int N>
inline
bool
empty(const TinyFifo<T, N> &f)
{
  return f.size()==0;
}

template<typename T,
         int N>
inline
bool
full(const TinyFifo<T, N> &f)
{
  return f.size()==f.capacity();
}

//---- some data to be kept around each thread ----
struct ThreadData
{
  std::thread th;
  int amount;
};

//---- some data common to every thread ----
struct SharedData
{
  std::vector<ThreadData> tdP{};
  std::vector<ThreadData> tdC{};
  TinyFifo<int, 5> fifo{};
  volatile bool done{false};

#if USE_SYNCHRO

  std::mutex mtx{};
  std::condition_variable_any cnd{};

#endif
};

//---- the actual work done by each producer thread ----
void
producerTask(int index,
             SharedData &sd)
{
  //---- access thread-specific data ----
  ThreadData &td=sd.tdP[index];

  //---- produce and enqueue many values ----
  int amount=0;
  for(int iter=0; iter<100'000; ++iter)
  {
    const int value=iter%(2+index);

#if USE_SYNCHRO

    //
    // ... À COMPLÉTER {6} ...
    //
    // Réaliser la même insertion dans la file d'attente (en s'assurant qu'elle
    // ne soit pas pleine) que dans le cas non synchronisé, mais en prenant
    // soin :
    // * de rendre l'attente bloquante (méthode ``wait()'' de la variable
    //   condition ``sd.cnd''),
    // * de signaler le changement d'état de la file d'attente (méthode
    //   ``notify_all()'' de la variable condition ``sd.cnd''),
    // * d'encadrer le tout par une section critique (méthodes ``lock()'' et 
    //   ``unlock()'' du verrou ``sd.mtx'').
    //

    // ...

#else // unsynchronised

    while(full(sd.fifo))    // wait for some room left
    {
                            // nothing more to be done
    }
    sd.fifo.enqueue(value); // then we can enqueue something

#endif

    amount+=value;
    // crs::out("Producer % (value=%)\n", index, value);
  }

  //---- make produced amount available to the main program ----
  td.amount=amount;
}

//---- the actual work done by each consumer thread ----
void
consumerTask(int index,
             SharedData &sd)
{
  //---- access thread-specific data ----
  ThreadData &td=sd.tdC[index];

  //---- dequeue and consume many values ----
  int amount=0;
  for(;;)
  {
    int value=-1;

#if USE_SYNCHRO

    //
    // ... À COMPLÉTER {7} ...
    //
    // Réaliser la même extraction depuis la file d'attente (en s'assurant
    // qu'elle ne soit pas vide et qu'il ne faille pas quitter) que dans le cas
    // non synchronisé, mais en prenant soin :
    // * de rendre l'attente bloquante (méthode ``wait()'' de la variable
    //   condition ``sd.cnd''),
    // * de signaler le changement d'état de la file d'attente (méthode
    //   ``notify_all()'' de la variable condition ``sd.cnd''),
    // * d'encadrer le tout par une section critique (méthodes ``lock()'' et
    //   ``unlock()'' du verrou ``sd.mtx'').
    // Attention, le verrou doit être libéré dans tous les cas, même lorsqu'on
    // doit quitter !
    //

    // ...

#else // unsynchronised

    while(!sd.done&&empty(sd.fifo)) // wait for end or something queued
    {
                                    // nothing more to be done
    }
    if(sd.done)                     // leave when end of work is signaled
    {
      break;
    }
    value=sd.fifo.dequeue();         // then there is something to dequeue

#endif

    amount+=value;
    // crs::out("Consumer % (value=%)\n", index, value);
  }

  //---- make consumed amount available to the main program ----
  td.amount=amount;
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

#if !USE_SYNCHRO
  crs::out("!!! unsynchronised version !!!\n");
#endif

  //---- determine number of threads ----
  int producerCount=-1;
  if(crs::len(args)>1) // chosen on the command line
  {
    crs::extract(args[1], producerCount);
  }
  if(producerCount<=0) // or deduced from number of CPU
  {
    producerCount=std::thread::hardware_concurrency();
  }
  int consumerCount=-1;
  if(crs::len(args)>2) // chosen on the command line
  {
    crs::extract(args[2], consumerCount);
  }
  if(consumerCount<=0) // or half the number of producers
  {
    consumerCount=producerCount-producerCount/2;
  }

  //---- prepare data for parallel work ----
  double t0=crs::gettimeofday();
  SharedData sd;
  sd.tdP.resize(producerCount);
  sd.tdC.resize(consumerCount);

  //---- launch producer threads ----
  crs::out("launching % producers\n", producerCount);
  //
  // ... À COMPLÉTER {1} ...
  //
  // Pour chacun des ``producerCount'' éléments de ``sd.tdP'', initialiser le
  // champ ``th'' avec un thread qui exécute la fonction ``producerTask()''.
  // Celle-ci doit recevoir, en plus d'un indice numérotant les threads,
  // une _référence_ sur la donnée partagée ``sd'' à laquelle les threads
  // accéderont tous.
  //

  // ...

  //---- launch consumer threads ----
  crs::out("launching % consumers\n", consumerCount);
  //
  // ... À COMPLÉTER {2} ...
  //
  // Pour chacun des ``consumerCount'' éléments de ``sd.tdC'', initialiser le
  // champ ``th'' avec un thread qui exécute la fonction ``consumerTask()''.
  // Celle-ci doit recevoir, en plus d'un indice numérotant les threads,
  // une _référence_ sur la donnée partagée ``sd'' à laquelle les threads
  // accéderont tous.
  //

  // ...

  //---- wait for producers to terminate and collect produced amount ----
  crs::out("waiting for % producers\n", producerCount);
  int produced=0;
  //
  // ... À COMPLÉTER {3} ...
  //
  // Parcourir ``sd.tdP'' afin d'effectuer l'opération ``join()'' sur chacun
  // des threads qu'il désigne.
  // Après chacune de ces opérations, il est envisageable de récupérer le
  // champ ``amount'' qui était associé au thread concerné puisque ce
  // dernier a terminé son traitement.
  // Afficher alors cette valeur et accumulez la (``+='') dans la variable
  // ``produced''.
  //

  // ...

  //---- signal (the consumers) for the end of work ----
#if USE_SYNCHRO

  //
  // ... À COMPLÉTER {5} ...
  //
  // Indiquer la fin du travail (en s'assurant qu'il n'y a plus rien à
  // consommer) comme dans le cas non synchronisé, mais en prenant
  // soin :
  // * de rendre l'attente bloquante (méthode ``wait()'' de la variable
  //   condition ``sd.cnd''),
  // * de signaler le changement d'état de l'indicateur de fin (méthode
  //   ``notify_all()'' de la variable condition ``sd.cnd''),
  // * d'encadrer le tout par une section critique (méthodes ``lock()'' et
  //   ``unlock()'' du verrou ``sd.mtx'').
  //

  // ...

#else // unsynchronised

  while(!empty(sd.fifo)) // wait until everything is consumed
  {
                         // nothing more to be done
  }
  sd.done=true;          // then signal end of work

#endif

  //---- wait for consumers to terminate and collect consumed amount ----
  crs::out("waiting for % consumers\n", consumerCount);
  int consumed=0;
  //
  // ... À COMPLÉTER {4} ...
  //
  // Parcourir ``sd.tdC'' afin d'effectuer l'opération ``join()'' sur chacun
  // des threads qu'il désigne.
  // Après chacune de ces opérations, il est envisageable de récupérer le
  // champ ``amount'' qui était associé au thread concerné puisque ce
  // dernier a terminé son traitement.
  // Afficher alors cette valeur et accumulez la (``+='') dans la variable
  // ``consumed''.
  //

  // ...

  //---- check consistency ----
  crs::out("done in % s, produced=%, consumed=%%\n",
           crs::gettimeofday()-t0, produced, consumed,
           produced!=consumed ? " !!!" : "");

  return 0;
}

//----------------------------------------------------------------------------
