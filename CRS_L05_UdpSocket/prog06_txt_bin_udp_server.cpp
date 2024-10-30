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
    crs::err("usage: % txt_port bin_port\n", args[0]);
    crs::exit(1);
  }

  //---- extract local port numbers ----
  uint16_t txtPortNumber=uint16_t(std::stoi(args[1]));
  uint16_t binPortNumber=uint16_t(std::stoi(args[2]));

  //---- create UDP socket (text) ----
  //
  // ... À COMPLÉTER {1} ...
  //
  // (semblable à prog02_txt_udp_server.cpp)
  //
  SOCKET s_txt = crs::socket(PF_INET, SOCK_DGRAM, 0);
  crs::bind(s_txt, txtPortNumber);
  // ...

  //---- create UDP socket (binary) ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // (semblable à prog04_bin_udp_server.cpp)
  //
  SOCKET s_bin = crs::socket(PF_INET, SOCK_DGRAM, 0);
  crs::bind(s_bin, binPortNumber);

  // ...

  crs::out("host '%' waiting for text/binary messages on ports '%/%'...\n",
           crs::gethostname(), txtPortNumber, binPortNumber);
  for(;;) // as long as dialog can go on...
  {
    //---- wait for incoming information ----
    //
    // ... À COMPLÉTER {4} ...
    //
    // Surveiller simultanément (avec ``crs::select()'') les messages arrivant
    // sur les deux sockets.
    //
    std::vector<SOCKET> rs={s_txt, s_bin};
    crs::select(rs);
    // ...

    //---- react to textual request (if available) ----
    //
    // ... À COMPLÉTER {5} ...
    //
    // Si la socket pour les échanges textuels est prête à fournir des données
    // (``voir crs::find()''), effectuer l'échange textuel requête/réponse.
    //
    // (semblable à prog02_txt_udp_server.cpp)
    //
    if( crs::find(rs, s_txt) !=-1 ) // ... MODIFIER CETTE CONDITION ...
    {
    auto [request, fromIpAddr, fromPort]=crs::recvfrom(s_txt, 0x100);
      crs::out("from %:%: <%>\n",
               crs::formatIpv4Address(fromIpAddr), fromPort, request);

      //---- prepare reply ----
      auto reply=crs::txt("server received % bytes\n", crs::len(request));

      //---- send reply to client ----
      crs::sendto(s_txt, reply, fromIpAddr, fromPort);
    }
    // ...

    //---- react to binary request (if available) ----
    //
    // ... À COMPLÉTER {6} ...
    //
    // Si la socket pour les échanges binaires est prête à fournir des données
    // (``voir crs::find()''), effectuer l'échange binaire requête/réponse.
    //
    // (semblable à prog04_bin_udp_server.cpp)
    //
    if( crs::find(rs,s_bin) != -1 ) // ... MODIFIER CETTE CONDITION ...
    {
      //---- receive a 32-bit integer in network byte-order ----
      int32_t tmpRequest;
      auto [r, fromIpAddr, fromPort]=
        crs::recvfrom(s_bin, &tmpRequest, sizeof(tmpRequest));
      if(r!=sizeof(tmpRequest))
        {
        crs::err("% bytes expected\n", sizeof(tmpRequest));
        crs::exit(1);
        }

      //---- convert to host byte-order 32-bit integer and display ----
      int32_t request=crs::ntoh_i32(tmpRequest);
      crs::out("from %:%: integer=%\n",
               crs::formatIpv4Address(fromIpAddr), fromPort, request);

      //---- prepare reply ----
      int32_t reply[2]={2*request, request*request};

      //---- convert to network byte-order 32-bit integers ----
      int32_t tmpReply[2];
      tmpReply[0]=crs::hton_i32(reply[0]);
      tmpReply[1]=crs::hton_i32(reply[1]);

      //---- send converted reply to client ----
      crs::sendto(s_bin, tmpReply, sizeof(tmpReply),
                  fromIpAddr, fromPort);

    }
    // ...
  }

  //---- close UDP sockets ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // (semblable à prog02_txt_udp_server.cpp et prog04_bin_udp_server.cpp)
  //
  crs::close(s_txt);
  crs::close(s_bin);
  // ...

  return 0;
}

//----------------------------------------------------------------------------
