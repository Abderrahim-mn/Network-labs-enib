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
  // (identique à prog03_txt_udp_client.cpp)
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
  // (identique à prog03_txt_udp_client.cpp)
  //
  SOCKET s = crs::socket(PF_INET, SOCK_DGRAM, 0);
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
    tmpValue = crs::hton_i32(value);
    // ...

    //---- send converted value to the specified destination/port ----
    //
    // ... À COMPLÉTER {5} ...
    //
    // Envoyer, à l'aide de ``crs::sendto()'', les ``sizeof(tmpValue)'' octets
    // qui constituent ``tmpValue'' au serveur désigné par ``ipAddress'' et
    // ``portNumber''.
    //
    crs::sendto(s, &tmpValue, sizeof(tmpValue), ipAddress, portNumber);
    // ...

    //---- receive two 32-bit integers in network byte-order ----
    int32_t tmpReply[2];
    //
    // ... À COMPLÉTER {6} ...
    //
    // Recevoir dans ``tmpReply'' les ``sizeof(tmpReply)'' octets
    // qui constituent cette variable depuis la socket UDP à l'aide de
    // ``crs::recvfrom()''.
    // Si la quantité d'octets reçue (le premier résultat de
    // ``crs::recvfrom()'') n'est pas celle attendue, afficher un message
    // d'erreur explicite et quitter avec ``crs::exit(1)''.
    //

    auto [r, fromIpAddr, fromPort]= crs::recvfrom(s, tmpReply, sizeof(tmpReply));
    if (r!=sizeof(tmpReply)) {
	    crs::err("ko % bytes receive", sizeof(tmpReply));
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
    // l'aide de ``crs::ntoh_i32()'' et afficher ces valeurs ainsi que
    // l'adresse IP et le port d'où elles proviennent.
    //
    for(int i=0;i<crs::len(reply);++i) {
	    reply[i] = crs::ntoh_i32(tmpReply[i]);
    }
     crs::out("from %:%: integers=<%, %>\n",
              crs::formatIpv4Address(fromIpAddr), fromPort,
              reply[0], reply[1]);

    // ...
  }

  //---- close UDP socket ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // Fermer la socket avec ``crs::close()''.
  //
  // (identique à prog03_txt_udp_client.cpp)
  //
  crs::close(s);
  // ...

  return 0;
}

//----------------------------------------------------------------------------
