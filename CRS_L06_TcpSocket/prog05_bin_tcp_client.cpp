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
  // (identique à prog02_txt_tcp_client.cpp)
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
  // (identique à prog02_txt_tcp_client.cpp)
  //
  SOCKET clientSocket=crs::socket(PF_INET, SOCK_STREAM, 0);
  // ... connected to the specified destination/port
  crs::connect(clientSocket, ipAddress, portNumber);
  // ...

  for(;;) // as long as dialog can go on...
  {
    //---- read next line on standard-input ----
    crs::out("host '%' waiting for user input... (integer)\n",
             crs::gethostname());
    auto msg=crs::readLine(STDIN_FILENO); // [Control]+[d] --> EOF
    if(empty(msg))
    {
      crs::out("<EOF>\n");
      break;
    }

    //---- extract 32-bit integer ----
    int32_t value;
    if(crs::extract(msg, value)!=1)
    {
      crs::err("input does not look like an integer\n");
      continue;
    }

    //---- convert to network byte-order 32-bit integer ----
    int32_t tmpValue;
    //
    // ... À COMPLÉTER {4} ...
    //
    // Placer dans ``tmpValue'' la conversion de ``value'' de l'ordre hôte
    // vers l'ordre réseau à l'aide de ``crs::hton_i32()''.
    //
    tmpValue=crs::hton_i32(value);
    // ...

    //---- send converted value to server ----
    //
    // ... À COMPLÉTER {5} ...
    //
    // Envoyer, à l'aide de ``crs::sendAll()'', les ``sizeof(tmpValue)''
    // octets qui constituent ``tmpValue'' au serveur à travers la
    // connexion TCP qui nous relie.
    //
    crs::sendAll(clientSocket, &tmpValue, sizeof(tmpValue));
    // ...

    //---- receive two 32-bit integers in network byte-order ----
    int32_t tmpReply[2];
    //
    // ... À COMPLÉTER {6} ...
    //
    // Recevoir dans ``tmpReply'' les ``sizeof(tmpReply)'' octets
    // qui constituent cette variable depuis la connexion TCP à l'aide de
    // ``crs::recvAll()''.
    // Si la quantité d'octets reçue (le résultat de ``crs::recvAll()'')
    // est nulle, cela signifie que la fin-de-fichier (EOF) est atteinte
    // (le serveur a fermé la connexion) ; il suffit de quitter la boucle
    // de dialogue avec ``break;'', sinon, si la quantité d'octets reçue
    // n'est pas celle attendue, afficher un message d'erreur explicite et
    // quitter avec ``crs::exit(1)''.
    //
    int r=crs::recvAll(clientSocket, tmpReply, sizeof(tmpReply));
    if(r==0)
    {
      crs::out("<EOF>\n");
      break;
    }
    if(r!=sizeof(tmpReply))
    {
      crs::err("% bytes expected\n", sizeof(tmpReply));
      crs::exit(1);
    }
    // ...

    //---- convert to host byte-order 32-bit integers and display----
    int32_t reply[2];
    //
    // ... À COMPLÉTER {7} ...
    //
    // Placer dans chaque élément de ``reply'' la conversion de l'élément
    // de ``tmpReply'' correspondant de l'ordre réseau vers l'ordre hôte, à
    // l'aide de ``crs::ntoh_i32()'' et afficher ces valeurs.
    //
    reply[0]=crs::ntoh_i32(tmpReply[0]);
    reply[1]=crs::ntoh_i32(tmpReply[1]);
    crs::out("received integers <%, %>\n", reply[0], reply[1]);
    // ...
  }

  //---- close client socket ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // Fermer la socket avec ``crs::close()''.
  //
  // (identique à prog02_txt_tcp_client.cpp)
  //
  crs::close(clientSocket);
  // ...

  return 0;
}

//----------------------------------------------------------------------------
