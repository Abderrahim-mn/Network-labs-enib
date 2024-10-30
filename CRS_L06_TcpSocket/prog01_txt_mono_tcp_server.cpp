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

  //---- create listen socket ----
  //
  // ... À COMPLÉTER {1} ...
  //
  // Créer avec ``crs::socket()'' une socket TCP, et utiliser ``crs::bind()''
  // pour qu'elle soit associée au port ``portNumber'' de la machine.
  // Il s'agit d'une socket d'écoute ; ceci sera spécifié par l'appel à
  // ``crs::listen()''.
  //
  SOCKET listenSocket=crs::socket(PF_INET, SOCK_STREAM, 0);
  // ... avoiding timewait problems (optional)
  crs::setReuseAddrOption(listenSocket, true);
  // ... bound to the specified port
  crs::bind(listenSocket, portNumber);
  // ... listening to connections
  crs::listen(listenSocket);
  // ...

  for(;;)
  {
    //---- accept and display new connection ----
    crs::out("host '%' waiting for a new connection on port '%'...\n",
             crs::gethostname(), portNumber);
    //
    // ... À COMPLÉTER {3} ...
    //
    // Accepter, à l'aide de ``crs::acceptfrom()'', la prochaine connexion
    // sur la socket d'écoute.
    // Cette opération fait apparaître une socket de dialogue TCP.
    // Afficher les coordonnées du client qui est à l'origine de cette
    // connexion.
    //
    auto [dialogSocket, fromIpAddr, fromPort]=crs::acceptfrom(listenSocket);
    crs::out("new connection from %:%\n",
             crs::formatIpv4Address(fromIpAddr), fromPort);
    // ...

    for(;;) // as long as dialog can go on...
    {
      //---- receive and display request from client ----
      crs::out("host '%' waiting for a TCP message from client...\n",
               crs::gethostname());
      std::string request;
      //
      // ... À COMPLÉTER {5} ...
      //
      // Recevoir dans ``request'' du texte depuis la socket de dialogue TCP
      // avec ``crs::recv()''.
      // Si le texte reçu est vide, cela signifie que la fin-de-fichier est
      // atteinte (le client a fermé la connexion) ; il suffit de quitter la
      // boucle de dialogue avec ``break;'', sinon afficher le texte reçu.
      //
      request=crs::recv(dialogSocket, 0x100);
      if(empty(request))
      {
        crs::out("<EOF>\n");
        break;
      }
      crs::out("received <%>\n", request);
      // ...

      //---- prepare reply ----
      auto reply=crs::txt("server received % bytes\n", crs::len(request));

      //---- send reply to client ----
      //
      // ... À COMPLÉTER {6} ...
      //
      // Envoyer avec ``crs::sendAll()'', la réponse ``reply'' au client à
      // travers la socket de dialogue TCP qui nous y relie.
      //
      crs::sendAll(dialogSocket, reply);
      // ...
    }

    //---- close dialog socket ----
    //
    // ... À COMPLÉTER {4} ...
    //
    // Fermer la socket de dialogue avec ``crs::close()''.
    //
    crs::close(dialogSocket);
    // ...

    crs::out("client disconnected\n");
  }

  //---- close listen socket ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // Fermer la socket d'écoute avec ``crs::close()''.
  // Même si cette portion de code n'est jamais atteinte ici (dans ce programme
  // simpliste), il faut toujours se poser la question de la fermeture !
  //
  crs::close(listenSocket);
  // ...

  return 0;
}

//----------------------------------------------------------------------------
