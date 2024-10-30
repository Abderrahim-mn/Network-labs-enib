//----------------------------------------------------------------------------

#include "crsUtils.hpp"

int
main(int argc,
     char **argv)
{
  //---- convert C-style command-line to C++ std::vector<std::string> ----
  std::vector<std::string> args{argv, argv+argc};

  //---- iterate through command-line arguments ----
  for(int i=0; i<crs::len(args); ++i)
  {
    //---- display every argument ----
    crs::writeAll(STDOUT_FILENO, crs::txt(
                  "args[%]=<%>\n", i, args[i]));

    //---- try to extract an integer value ----
    int value;
    if(crs::extract(args[i], value)==1)
    {
      crs::writeAll(STDOUT_FILENO, crs::txt(
                    "  --> %*%=%\n", value, value, value*value));
    }
  }

  for(;;)
  {
    //---- read next line on standard-input ----
    crs::writeAll(STDOUT_FILENO, "waiting for user input...\n");
    auto msg=crs::readLine(STDIN_FILENO); // [Control]+[d] --> EOF
    if(empty(msg))
    {
      crs::writeAll(STDOUT_FILENO, "<EOF>\n");
      break;
    }

    //---- interpret user input ----
    //
    // ... À COMPLÉTER ...
    //
    // Si la saisie correspond à une valeur entière, réaliser et afficher un
    // calcul simple avec cet entier,
    // s'il s'agit de la chaîne "bye\n", quitter la boucle de saisie avec
    // l'instruction ``break;'',
    // sinon, afficher la ligne saisie ainsi que sa longueur.
    //

    // ...
  }

  return 0;
}

//----------------------------------------------------------------------------
