//----------------------------------------------------------------------------

#include "crsUtils.hpp"

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- create anonymous pipe ----
  //
  // ... À COMPLÉTER {1} ...
  //
  // Créer un tube de communication anonyme à l'aide de ``crs::pipe()''
  // et obtenir ses extrémités ``lecture'' et ``écriture''.
  //
  auto [fifoR, fifoW]=crs::pipe();
  // ...

  //---- create child process ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // Créer un processus enfant à l'aide de ``crs::fork()''.
  //
  pid_t child=crs::fork();
  // ...

  //---- handle child process ----
  //
  // ... À COMPLÉTER {4} ...
  //
  // Faire en sorte que seul le processus enfant entre dans l'alternative.
  //
  // if( false ) // ... MODIFIER CETTE CONDITION ...
  if(child==0)
  // ...
  {
    //---- close useless end of the pipe ----
    //
    // ... À COMPLÉTER {6} ...
    //
    // Fermer, à l'aide de ``crs::close()'', le côté ``lecture'' du tube
    // puisque le processus enfant n'utilise ce tube qu'en écriture.
    //
    crs::close(fifoR); // child will not read from this pipe
    // ...

    //---- produce messages ----
    for(int i=0; i<20; ++i)
    {
      auto msg=crs::txt("message % from child to parent\n", i);
      //
      // ... À COMPLÉTER {8} ...
      //
      // Écrire le message ``msg'' dans le tube à l'aide de
      //   ``crs::writeAll()''.
      //
      crs::writeAll(fifoW, msg);
      // ...

      //---- make messages appear slowly ----
      crs::sleep(0.25);
    }

    //---- close pipe ----
    //
    // ... À COMPLÉTER {9} ...
    //
    // Fermer le côté ``écriture'' du tube puisque le processus enfant
    // a fini d'écrire.
    //
    crs::close(fifoW); // done with writing
    // ...

    //---- leave child process ----
    //
    // ... À COMPLÉTER {5} ...
    //
    // Terminer quoi qu'il arrive l'exécution du processus enfant par
    // ``std::exit()''.
    // La suite du code (après le ``if'') ne sera ainsi exécutée que par
    // le processus parent.
    //
    crs::exit(0);
    // ...
  }

  //---- close useless end of the pipe ----
  //
  // ... À COMPLÉTER {7} ...
  //
  // Fermer, à l'aide de ``crs::close()'', le côté ``écriture'' du tube
  // puisque le processus parent n'utilise ce tube qu'en lecture.
  //
  crs::close(fifoW); // parent will not write to this pipe
  // ...

  //---- consume messages ----
  for(;;)
  {
    std::string msg;
    //
    // ... À COMPLÉTER {10} ...
    //
    // Obtenir dans ``msg'' une chaîne C++ depuis le tube à l'aide de
    // ``crs::read()'' (fixons arbitrairement la capacité maximale à 0x100).
    // Si ce message est vide (fin-de-fichier), il faut quitter cette boucle
    // avec ``break;''.
    //
    msg=crs::read(fifoR, 0x100);
    if(empty(msg))
    {
      crs::out("<EOF>\n");
      break;
    }
    // ...
    crs::out("parent received <%>\n", msg);
  }

  //---- close pipe ----
  //
  // ... À COMPLÉTER {11} ...
  //
  // Fermer le côté ``lecture'' du tube puisque le processus parent
  // a détecté la fin-de-fichier et donc ne lira plus.
  //
  crs::close(fifoR); // done with reading
  // ...

  //---- wait for child process ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // Attendre la terminaison du processus enfant à l'aide de
  // ``crs::waitpid()''.
  //
  crs::waitpid(child);
  // ...

  return 0;
}

//----------------------------------------------------------------------------
