//----------------------------------------------------------------------------

#include "crsUtils.hpp"

void
changeData(int *data,
           int increment)
{
  if(data)
  {
    *data+=increment;
  }
}

void
showData(const std::string &msg,
         int *data)
{
  if(data)
  {
    crs::out("% --> %% @%\n",
             msg, std::setw(4), *data, data);
  }
}

int staticData; // global variable --> static storage

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- create various kinds of data ----
  extern int staticData; // useless...
  int automaticData; // local variable --> automatic storage (by default)
  int *dynamicData=static_cast<int *>(malloc(sizeof(int)));
  int *sharedData=nullptr;
  //
  // ... À COMPLÉTER {1} ...
  //
  // Utiliser ``crs::mmap_shared()'' pour obtenir dans ``sharedData'' un
  // segment de mémoire partagée pouvant contenir un seul ``int''.
  //
  sharedData=crs::mmap_shared<int>(1);
  // ...

  //---- set initial values (ones) ----
  staticData=1;
  automaticData=2;
  *dynamicData=3;
  if(sharedData)
  {
    *sharedData=4;
  }

  //---- display initial values ----
  showData("initial, static   ", &staticData);
  showData("initial, automatic", &automaticData);
  showData("initial, dynamic  ", dynamicData);
  showData("initial, shared   ", sharedData);

  //---- create a new process ----
  pid_t child=crs::fork();

  if(child==0) // child process
  {
    //---- change values (tens) ----
    changeData(&staticData,    10);
    changeData(&automaticData, 20);
    changeData(dynamicData,    30);
    changeData(sharedData,     40);
    crs::out("tens changed by child process\n");

    //---- let some time for the hundreds to be changed ----
    crs::sleep(1);

    //---- display values ----
    showData("child,   static   ", &staticData);
    showData("child,   automatic", &automaticData);
    showData("child,   dynamic  ", dynamicData);
    showData("child,   shared   ", sharedData);
  }
  else // parent process
  {
    //---- change values (hundreds) ----
    changeData(&staticData,    100);
    changeData(&automaticData, 200);
    changeData(dynamicData,    300);
    changeData(sharedData,     400);
    crs::out("hundreds changed by parent process\n");

    //---- let some time for the tens to be changed ----
    crs::sleep(1);

    //---- display values ----
    showData("parent,  static   ", &staticData);
    showData("parent,  automatic", &automaticData);
    showData("parent,  dynamic  ", dynamicData);
    showData("parent,  shared   ", sharedData);

    //---- wait for child process ----
    crs::waitpid(child);
  }

  //---- release resources ----
  // nothing to be done for staticData
  // nothing to be done for automaticData
  free(dynamicData);
  if(sharedData)
  {
    //
    // ... À COMPLÉTER {2} ...
    //
    // Libérer le segment de mémoire partagé à l'aide de ``crs::munmap()''.
    //
    crs::munmap(sharedData, 1);
    // ...
  }

  return 0;
}

//----------------------------------------------------------------------------
