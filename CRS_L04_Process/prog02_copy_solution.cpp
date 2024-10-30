//----------------------------------------------------------------------------

#include "crsUtils.hpp"

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- check command line arguments ----
  std::string inputFileName, outputFileName;
  for(int i=1; i<crs::len(args); ++i)
  {
    if((args[i]=="-i")&&(i+1<crs::len(args)))
    {
      inputFileName=args[++i];
    }
    else if((args[i]=="-o")&&(i+1<crs::len(args)))
    {
      outputFileName=args[++i];
    }
    else
    {
      crs::writeAll(STDERR_FILENO, crs::txt(
                    "usage: % [-i input] [-o output]\n", args[0]));
      crs::exit(1);
    }
  }

  //---- choose input file-descriptor ----
  int input=STDIN_FILENO; // use standard-input by default
  if(!empty(inputFileName))
  {
    crs::writeAll(STDERR_FILENO, crs::txt(
                  "using '%' as input\n", inputFileName));
    //
    // ... À COMPLÉTER {1} ...
    //
    // Ouvrir en lecture, à l'aide de ``crs::openR()'', le fichier désigné
    // par ``inputFileName''.
    // Le résultat (un descripteur de fichier) doit écraser la variable
    // ``input'' pour remplacer l'usage de l'entrée standard.
    // 
    input=crs::openR(inputFileName);
    // ...
  }

  //---- choose output file-descriptor ----
  int output=STDOUT_FILENO; // use standard-output by default
  if(!empty(outputFileName))
  {
    crs::writeAll(STDERR_FILENO, crs::txt(
                  "using '%' as output\n", outputFileName));
    //
    // ... À COMPLÉTER {2} ...
    //
    // Ouvrir en écriture, à l'aide de ``crs::openW()'', le fichier désigné
    // par ``outputFileName''.
    // Le résultat (un descripteur de fichier) doit écraser la variable
    // ``output'' pour remplacer l'usage de la sortie standard.
    // 
    output=crs::openW(outputFileName);
    // ...
  }

  //---- copy from input to output file-descriptor ----
  char buffer[0x400];
  for(;;)
  {
    //
    // ... À COMPLÉTER {5} ...
    //
    // Obtenir ``r'' octets depuis le ``input'' vers ``buffer'' à l'aide
    // de ``crs::read()''.
    // (nb : il s'agit de données brûtes, pas forcément de texte)
    // Si ``r'' est nul (fin-de-fichier), il faut quitter cette boucle avec
    // ``break;''.
    // Utiliser ``crs::writeAll()'' pour envoyer vers ``output'' les ``r''
    // octets précédemment obtenus dans ``buffer''.
    //
    int r=crs::read(input, buffer, sizeof(buffer));
    crs::writeAll(STDERR_FILENO, crs::txt("% bytes obtained\n", r));
    if(r==0)
    {
      break; // EOF
    }
    crs::writeAll(output, buffer, r);
    // ...
  }

  //---- close output file (if any) ----
  if(!empty(outputFileName))
  {
    crs::writeAll(STDERR_FILENO, crs::txt(
                  "closing '%' output\n", outputFileName));
    //
    // ... À COMPLÉTER {3} ...
    //
    // Fermer le descripteur de fichier ``output'' avec ``crs::close()''.
    //
    crs::close(output);
    // ...
  }

  //---- close input file (if any) ----
  if(!empty(inputFileName))
  {
    crs::writeAll(STDERR_FILENO, crs::txt(
                  "closing '%' input\n", inputFileName));
    //
    // ... À COMPLÉTER {4} ...
    //
    // Fermer le descripteur de fichier ``input'' avec ``crs::close()''.
    //
    crs::close(input);
    // ...
  }

  return 0;
}

//----------------------------------------------------------------------------
