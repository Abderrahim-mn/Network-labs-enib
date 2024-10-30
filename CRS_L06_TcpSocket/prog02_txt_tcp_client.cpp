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
  ipAddress=crs::gethostbyname(args[1]);
  // ...

  //---- extract destination port number ----
  uint16_t portNumber=uint16_t(std::stoi(args[2]));

  //---- create client socket ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // Créer avec ``crs::socket()'' une socket TCP.
  // Utiliser ``crs::connect()'' pour la connecter à l'adresse IP et au port
  // du serveur visé.
  //
  SOCKET clientSocket=crs::socket(PF_INET, SOCK_STREAM, 0);
  // ... connected to the specified destination/port
  crs::connect(clientSocket, ipAddress, portNumber);
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

    //---- send message to server ----
    //
    // ... À COMPLÉTER {4} ...
    //
    // Envoyer, à l'aide de ``crs::sendAll()'', le message textuel ``msg''
    // au serveur à travers la connexion TCP qui nous y relie.
    //
    crs::sendAll(clientSocket, msg);
    // ...

    //---- receive and display reply ----
    std::string reply;
    //
    // ... À COMPLÉTER {5} ...
    //
    // Recevoir dans ``reply'' la réponse (textuelle) du serveur depuis la
    // connexion TCP qui nous y relie.
    // Si le texte reçu est vide, cela signifie que la fin-de-fichier (EOF)
    // est atteinte (le client a fermé la connexion) ; il suffit de quitter
    // la boucle de dialogue avec ``break;'', sinon afficher le texte reçu.
    reply=crs::recv(clientSocket, 0x100);
    if(empty(reply))
    {
      crs::out("<EOF>\n");
      break;
    }
    crs::out("received <%>\n", reply);
    // ...
  }

  //---- close client socket ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // Fermer la socket avec ``crs::close()''.
  //
  crs::close(clientSocket);
  // ...

  return 0;
}

//----------------------------------------------------------------------------
