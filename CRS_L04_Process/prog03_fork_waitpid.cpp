//----------------------------------------------------------------------------

#include "crsUtils.hpp"

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- display process identifiers ----
  crs::out("process % starting... (parent=%)\n",
           crs::getpid(), crs::getppid());

  //---- create a new process ----
  pid_t result=-1;
  //
  // ... À COMPLÉTER {1} ...
  //
  // Utiliser ``crs::fork()'' pour faire apparaître un nouveau processus
  // et mémoriser le résultat de cet appel dans ``result''.
  //

  // ...

  //---- display process identifiers and fork() result ----
  crs::out("process % obtained % from fork() (parent=%)\n",
           crs::getpid(), result, crs::getppid());

  //---- distinguish child process from parent process ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // Utiliser la valeur de ``result'' pour que le nouveau processus (l'enfant)
  // passe par la première branche de l'alternative alors que le processus
  // initial prendra la seconde branche.
  //
  if( true ) // ... MODIFIER CETTE CONDITION ...
  // ...
  {
    //
    // ... À COMPLÉTER {5} ...
    //
    // Provoquez un plantage du processus enfant,
    // en levant une exception par exemple :
    //   throw std::runtime_error{"Drop dead!"};
    //

    // ...

    //
    // ... À COMPLÉTER {4} ...
    //
    // Terminer le processus enfant en invoquant ``crs::exit()'' avec
    // une valeur entière de votre choix (<256).
    //

    // ...
  }
  else
  {
    //---- wait for child process ----
    crs::out("process % waiting for child process %\n",
             crs::getpid(), result);
    //
    // ... À COMPLÉTER {3} ...
    //
    // Attendre la terminaison du processus enfant à l'aide de
    // ``crs::waitpid()'' et afficher les trois informations
    // obtenues.
    //

    // auto [pid, status, signal]= ...
    // crs::out("after waitpid(%) --> pid=%, status=%, signal=% (%)\n",
    //          result, pid, status, signal, crs::strsignal(signal));

    // ...
  }

  crs::out("process % leaving... (parent=%)\n",
           crs::getpid(), crs::getppid());
  return 0;
}

//----------------------------------------------------------------------------
