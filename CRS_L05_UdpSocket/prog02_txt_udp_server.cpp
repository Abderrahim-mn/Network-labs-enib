//----------------------------------------------------------------------------

#include "crsUtils.hpp"

int
main(int argc,
     char **argv)
{
  std::vector<std::string> args{argv, argv+argc};

  //---- check command line arguments ----
  if(crs::len(args)!=2)
  {
    crs::err("usage: % port\n", args[0]);
    crs::exit(1);
  }

  //---- extract local port number ----
  uint16_t portNumber=uint16_t(std::stoi(args[1]));

  //---- create UDP socket ----
  //
  // ... À COMPLÉTER {1} ...
  //
  // Créer avec ``crs::socket()'' une socket UDP, et utiliser ``crs::bind()'' 
  // pour qu'elle soit associée au port ``portNumber'' de la machine.
  //
  SOCKET s = crs::socket(PF_INET, SOCK_DGRAM, 0);
  crs::bind(s, portNumber);
  // ...

  for(;;) // as long as dialog can go on...
  {
    //---- receive and display request and source address/port ----
    crs::out("host '%' waiting for an UDP message on port '%'...\n",
             crs::gethostname(), portNumber);
    //
    // ... À COMPLÉTER {3} ...
    //
    // Recevoir du texte depuis la socket UDP avec ``crs::recvfrom()''
    // et l'afficher ainsi que l'adresse IP et le port d'où il provient.
    //

    auto [request, fromIpAddr, fromPort] = crs::recvfrom(s, 0x100);
    crs::out("from %:%: <%>\n",
            crs::formatIpv4Address(fromIpAddr), fromPort, request);

    // ...

    //---- prepare and send reply to client ----
    //
    // ... À COMPLÉTER {4} ...
    //
    // Envoyer avec ``crs::sendto()'', la réponse ``reply'' au client
    // qui nous a sollicité.
    //

    auto reply=crs::txt("server received % bytes\n", crs::len(request));
    crs::sendto(s, reply, fromIpAddr, fromPort);
    // ...
  }

  //---- close UDP socket ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // Fermer la socket avec ``crs::close()''.
  // Même si cette portion de code n'est jamais atteinte ici (dans ce programme
  // simpliste), il faut toujours se poser la question de la fermeture !
  //

  // ...
  crs::close(s);
  return 0;
}

//----------------------------------------------------------------------------
