//----------------------------------------------------------------------------

#include "crsUtils.hpp"

//---- the actual work done by the thread ----
void
threadTask(int count,
           int &result)
{
  crs::out("starting %(%, %)\n", __func__, count, result);
  for(int i=0; i<count; ++i)
  {
    crs::out("adding % to %\n", i+1, result);
    result+=i+1;
    crs::sleep(1.0);
  }
  crs::out("leaving %(%, %)\n", __func__, count, result);
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- prepare common data ----
  int count=5;
  if(crs::len(args)>1)
  {
    count=std::stoi(args[1]);
  }
  int result=0;

  //---- start a new thread ----
  crs::out("main() launches a thread\n");
  //
  // ... À COMPLÉTER {1} ...
  //
  // Démarrer un thread qui va exécuter la fonction ``threadTask()''.
  // Pour démarrer un tel thread, nous utilisons :
  //   std::thread th( nom_de_la_fonction, paramètres_de_cette_fonction... );
  // Ceci crée dans la variable ``th'' un objet de type ``std::thread''
  // qui se charge de demander au système d'exploitation le démarrage d'un
  // nouveau thread pour qu'il exécute en parallèle la fonction indiquée en lui
  // transmettant les paramètres requis.
  // Les paramètres qui sont passés par valeur ne demandent pas de précaution
  // particulière (ils seront simplement recopiés dans le contexte du thread).
  // En revanche, les paramètres qui sont passés par référence, doivent être
  // explicitement entourés de ``std::ref( le_paramètre )'' afin que la
  // fonction exécutée ne fasse pas référence à une copie du paramètre mais
  // bien à son original.
  // Ici nous souhaitons que notre thread se contente de recevoir la valeur de
  // la variable ``count'' de la fonction ``main()'' mais qu'il soit en mesure
  // d'en modifier la variable ``result'' (voir le prototype de la fonction
  // ``threadTask()'').
  //

  // ...

  //---- keep main function busy while thread is running ----
  for(int i=0; i<5; ++i)
  {
    crs::out("main() is still running...\n");
    crs::sleep(0.5);
  }

  //---- wait for thread to terminate ----
  crs::out("main() is waiting for the thread\n");
  //
  // ... À COMPLÉTER {2} ...
  //
  // Attendre que le thread ait terminé son exécution.
  // Pour ceci, nous utilisons la méthode ``join()'' de l'objet de type
  // ``std::thread'' qui désigne notre thread.
  //

  // ...

  //---- display result computed by the thread ----
  crs::out("result=%\n", result);

  return 0;
}

//----------------------------------------------------------------------------
