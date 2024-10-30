//----------------------------------------------------------------------------

#include "crsUtils.hpp"

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- check command line arguments ----
  if(crs::len(args)!=3)
  {
    crs::err("usage: % destination port\n", args[0]);
    crs::exit(1);
  }

  //---- extract destination IP address ----
  uint32_t ipAddress=0;
  //
  // ... À COMPLÉTER {1} ...
  //
  // Déterminer dans ``ipAddress'', à l'aide de ``crs::gethostbyname()'',
  // l'adresse IP de ``args[1]''.
  //
  ipAddress = crs::gethostbyname(args[1]);
  //...

  //---- extract destination port number ----
  uint16_t portNumber=uint16_t(std::stoi(args[2]));

  //---- create UDP socket ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // Créer avec ``crs::socket()'' une socket UDP.
  //
  SOCKET s = crs::socket(PF_INET, SOCK_DGRAM, 0);

  // ...

  for(;;) // as long as dialog can go on...
  {
    //---- read next line on standard-input ----
    crs::out("host '%' waiting for user input... (text line)\n",
             crs::gethostname());
    auto msg=crs::readLine(STDIN_FILENO); // [Control]+[d] --> EOF
    if(empty(msg))
    {
      crs::out("<EOF>\n");
      break;
    }

    //---- send message to the specified destination/port ----
    //
    // ... À COMPLÉTER {4} ...
    //
    // Envoyer, à l'aide de ``crs::sendto()'', le message textuel ``msg'' au
    // serveur désigné par ``ipAddress'' et ``portNumber''.
    //
    crs::sendto(s, msg, ipAddress, portNumber);
    // ...

    //---- receive and display reply and source address/port ----
    //
    // ... À COMPLÉTER {5} ...
    //
    // Recevoir la réponse textuelle depuis la socket UDP avec
    // ``crs::recvfrom()'' et l'afficher ainsi que l'adresse IP et le port
    // d'où elle provient (normalement ce doit être le serveur sollicité).
    //

    auto [reply, fromIpAddr, fromPort] = crs::recvfrom(s, 0x100);
    crs::out("from %:%: <%>\n",
              crs::formatIpv4Address(fromIpAddr), fromPort, reply);

    // ...
  }

  //---- close UDP socket ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // Fermer la socket avec ``crs::close()''.
  //

  crs::close(s);
  // ...

  return 0;
}

//----------------------------------------------------------------------------
