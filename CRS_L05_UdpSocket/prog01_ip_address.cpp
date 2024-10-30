//----------------------------------------------------------------------------

#include "crsUtils.hpp"

void
showIpAddress(const std::string &hostName)
{
  try
  {
    //---- extract IP address from host name ----
    uint32_t ipAddress=0;
    //
    // ... À COMPLÉTER {2} ...
    //
    // Déterminer dans ``ipAddress'', à l'aide de ``crs::gethostbyname()'',
    // l'adresse IP de ``hostname''.
    //

    //...
    ipAddress = crs::gethostbyname(hostName);
    //---- display dotted-decimal form of IP address ----
    crs::out("'%' host has IP address '%'\n",
             hostName, crs::formatIpv4Address(ipAddress));
  }
  catch(const std::exception &)
  {
    // ``crs::gethostbyname()'' throws an exception on failure!
    crs::out("unknown IP address for host '%'\n", hostName);
  }
}

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- determine current host name ----
  std::string hostName;
  //
  // ... À COMPLÉTER {1} ...
  //
  // Déterminer dans ``hostname'', à l'aide de ``crs::gethostname()'',
  // le nom de domaine qui a été affecté à la machine courante.
  //
  hostName = crs::gethostname();
  // ...
  crs::out("current host name is '%'\n", hostName);

  //---- display IP addresses ---
  showIpAddress(hostName);
  for(int i=1; i<crs::len(args); ++i)
  {
    showIpAddress(args[i]);
  }

  return 0;
}

//----------------------------------------------------------------------------
