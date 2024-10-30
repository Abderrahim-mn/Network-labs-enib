//----------------------------------------------------------------------------

#include "crsUtils.hpp"

//---- the actual work done by each thread ----
void
threadTask(int index,
           std::string msg)
{
  for(int i=0; i<5; ++i)
  {
    crs::out("thread % says %\n", index, msg);
    crs::sleep(0.5+0.25*(crs::len(msg)%2));
  }
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- one thread for each command-line argument ----
  int threadCount=crs::len(args);

  //---- launch threads ----
  crs::out("launching % threads\n", threadCount);
  //
  // ... À COMPLÉTER {1} ...
  //
  // Créer un ``std::vector<std::thread>'' de taille ``threadCount''.
  // Chacun de ses éléments désignera un thread exécutant la fonction
  // ``threadTask()'' qui recevra comme paramètre son indice (de 0 à
  // ``threadCount-1'') ainsi que le mot de la ligne de commande correspondant
  // à cet indice (``args[i]'').
  //
  std::vector<std::thread> th(threadCount);
  for(int i=0; i<threadCount; ++i)
  {
    th[i]=std::thread{threadTask, i, args[i]};
  }
  // ...

  //---- wait for threads to terminate ----
  crs::out("waiting for % threads\n", threadCount);
  //
  // ... À COMPLÉTER {2} ...
  //
  // Parcourir le vecteur de threads précédant afin d'effectuer l'opération
  // ``join()'' sur chacun de ses éléments.
  //
  for(int i=0; i<threadCount; ++i)
  {
    th[i].join();
  }
  // ...

  crs::out("done\n");

  return 0;
}

//----------------------------------------------------------------------------
