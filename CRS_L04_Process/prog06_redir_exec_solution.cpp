//----------------------------------------------------------------------------

#include "crsUtils.hpp"

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- create anonymous pipe ----
  auto [fifoR, fifoW]=crs::pipe();

  //---- create child process ----
  pid_t child=crs::fork();

  //---- handle child process ----
  if(child==0)
  {
    //---- close useless end of the pipe ----
    crs::close(fifoR); // child will not read from this pipe

    //---- redirect standard-output to pipe ----
    //
    // ... À COMPLÉTER {1} ...
    //
    // Réaliser une redirection pour que la sortie standard soit désormais
    // associée au côté écriture du tube.
    // * le descripteur de fichier de la sortie standard ne doit plus
    //   désigner le terminal mais le côté écriture du tube
    //     crs::dup2(fifoW, STDOUT_FILENO);
    // * le descripteur sur le côté écriture du tube est désormais inutile
    //   car celui de la sortie standard désigne dorénavant cette ressource
    //     crs::close(fifoW);
    //

    crs::dup2(fifoW, STDOUT_FILENO);
    crs::close(fifoW); // redirected so useless now

    // ...

    if(crs::len(args)>1) // a file name is provided on the command line
    {
      //---- open input file ----
      int input=crs::openR(args[1]);

      //---- redirect standard-input to input file ----
      //
      // ... À COMPLÉTER {2} ...
      //
      // Réaliser une redirection pour que l'entrée standard soit désormais
      // associée au descripteur de fichier ``input''.
      //

      crs::dup2(input, STDIN_FILENO);
      crs::close(input); // redirected so useless now

      // ...

      //---- execute decompression program in the current process ----
      //
      // ... À COMPLÉTER {3} ...
      //
      // Utiliser simplement ``crs::exec()'' avec comme ligne de commande
      // ``{ "gunzip" }'' (un tableau de chaînes ne contenant qu'un
      // seul élément : le nom du programme).
      //

      crs::exec({ "gunzip" }); // never returns!

      // ... 
    }

    //---- produce a message on standard output ----
    crs::out("this is a message from the child process\n");

    //---- leave child process ----
    crs::exit(0);
  }

  //---- close useless end of the pipe ----
  crs::close(fifoW); // parent will not write to this pipe

  //---- consume messages ----
  for(;;)
  {
    std::string msg=crs::read(fifoR, 0x100);
    if(empty(msg))
    {
      crs::out("<EOF>\n");
      break;
    }
    // ...
    crs::out("PARENT RECEIVED [[[%]]]\n", msg);
  }

  //---- close pipe ----
  crs::close(fifoR); // done with reading

  //---- wait for child process ----
  crs::waitpid(child);

  return 0;
}

//----------------------------------------------------------------------------
