//----------------------------------------------------------------------------

#include "crsUtils.hpp"

void
txtDialogThread(SOCKET dialogSocket)
{
  try
  {
    for(;;) // as long as dialog can go on...
    {
      //
      // ... À COMPLÉTER {7} ...
      //
      // Reprendre ici ce qui a été réalisé dans la fonction ``dialogThread()''
      // de prog03_txt_tcp_server.cpp
      //
      //---- receive and display request from client ----
      auto request=crs::recv(dialogSocket, 0x100);
      if(empty(request))
      {
        crs::out("<EOF>\n");
        break;
      }
      crs::out("received <%>\n", request);

      //---- prepare reply ----
      auto reply=crs::txt("server received % bytes\n", crs::len(request));

      //---- send reply to client ----
      crs::sendAll(dialogSocket, reply);
      // ...
    }
  }
  catch(const std::exception &e)
  {
    crs::err("\n!!! Exception: % !!!\n", e.what());
  }
  catch(...)
  {
    crs::err("\n!!! Unknown exception !!!\n");
  }

  //---- close dialog socket in any case! ----
  crs::close(dialogSocket);

  crs::out("text client disconnected\n");
}

void
binDialogThread(SOCKET dialogSocket)
{
  try
  {
    for(;;) // as long as dialog can go on...
    {
      //
      // ... À COMPLÉTER {8} ...
      //
      // Reprendre ici ce qui a été réalisé dans la fonction ``dialogThread()''
      // de prog04_bin_tcp_server.cpp
      //
      //---- receive a 32-bit integer in network byte-order ----
      int32_t tmpRequest;
      int r=crs::recvAll(dialogSocket, &tmpRequest, sizeof(tmpRequest));
      if(r==0)
      {
        crs::out("<EOF>\n");
        break;
      }
      if(r!=sizeof(tmpRequest))
      {
        crs::err("% bytes expected\n", sizeof(tmpRequest));
        crs::exit(1);
      }

      //---- convert to host byte-order 32-bit integer and display ----
      int32_t request=crs::ntoh_i32(tmpRequest);
      crs::out("received integer %\n", request);

      //---- prepare reply ----
      int32_t reply[2]={2*request, request*request};

      //---- convert to network byte-order 32-bit integers ----
      int32_t tmpReply[2];
      tmpReply[0]=crs::hton_i32(reply[0]);
      tmpReply[1]=crs::hton_i32(reply[1]);

      //---- send converted reply to client ----
      crs::sendAll(dialogSocket, tmpReply, sizeof(tmpReply));
      // ...
    }
  }
  catch(const std::exception &e)
  {
    crs::err("\n!!! Exception: % !!!\n", e.what());
  }
  catch(...)
  {
    crs::err("\n!!! Unknown exception !!!\n");
  }

  //---- close dialog socket in any case! ----
  crs::close(dialogSocket);

  crs::out("binary client disconnected\n");
}

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

  //---- create listen socket (text) ----
  //
  // ... À COMPLÉTER {1} ...
  //
  // (semblable à prog03_txt_tcp_server.cpp)
  //
  SOCKET txtListenSocket=crs::socket(PF_INET, SOCK_STREAM, 0);
  // ... avoiding timewait problems (optional)
  crs::setReuseAddrOption(txtListenSocket, true);
  // ... bound to the specified port
  crs::bind(txtListenSocket, txtPortNumber);
  // ... listening to connections
  crs::listen(txtListenSocket);
  // ...

  //---- create listen socket (binary) ----
  //
  // ... À COMPLÉTER {2} ...
  //
  // (semblable à prog04_bin_tcp_server.cpp)
  //
  SOCKET binListenSocket=crs::socket(PF_INET, SOCK_STREAM, 0);
  // ... avoiding timewait problems (optional)
  crs::setReuseAddrOption(binListenSocket, true);
  // ... bound to the specified port
  crs::bind(binListenSocket, binPortNumber);
  // ... listening to connections
  crs::listen(binListenSocket);
  // ...

  crs::out("host '%' waiting for text/binary connections on ports '%/%'...\n",
           crs::gethostname(), txtPortNumber, binPortNumber);
  for(;;)
  {
    //---- wait for connections ----
    //
    // ... À COMPLÉTER {4} ...
    //
    // Surveiller simultanément (avec ``crs::select()'') les demandes de
    // connexion sur les deux sockets d'écoute.
    //
    std::vector<SOCKET> readSet={txtListenSocket, binListenSocket};
    crs::select(readSet);
    // ...

    //---- react to text connection (if available) ----
    //
    // ... À COMPLÉTER {5} ...
    //
    // Si la socket d'écoute pour les échanges textuels a reçu une nouvelle
    // demande de connexion (``voir crs::find()''), accepter cette connexion
    // et démarrer un thread qui exécutera le dialogue avec le client dans
    // la fonction ``txtDialogThread()''.
    //
    // (semblable à prog03_txt_tcp_server.cpp)
    //
    // if( false ) // ... MODIFIER CETTE CONDITION ...
    if(crs::find(readSet, txtListenSocket)!=-1)
    {
      //---- accept and display new connection ----
      auto [dialogSocket, fromIpAddr, fromPort]=
        crs::acceptfrom(txtListenSocket);
      crs::out("new text connection from %:%\n",
               crs::formatIpv4Address(fromIpAddr), fromPort);
      //---- start a new dialog thread ----
      std::thread th{txtDialogThread, dialogSocket};
      th.detach();
    }
    // ...

    //---- react to binary connection (if available) ----
    //
    // ... À COMPLÉTER {6} ...
    //
    // Si la socket d'écoute pour les échanges binaires a reçu une nouvelle
    // demande de connexion (``voir crs::find()''), accepter cette connexion
    // et démarrer un thread qui exécutera le dialogue avec le client dans
    // la fonction ``binDialogThread()''.
    //
    // (semblable à prog04_bin_tcp_server.cpp)
    //
    // if( false ) // ... MODIFIER CETTE CONDITION ...
    if(crs::find(readSet, binListenSocket)!=-1)
    {
      //---- accept and display new connection ----
      auto [dialogSocket, fromIpAddr, fromPort]=
        crs::acceptfrom(binListenSocket);
      crs::out("new binary connection from %:%\n",
               crs::formatIpv4Address(fromIpAddr), fromPort);
      //---- start a new dialog thread ----
      std::thread th{binDialogThread, dialogSocket};
      th.detach();
    }
    // ...
  }

  //---- close listen sockets ----
  //
  // ... À COMPLÉTER {3} ...
  //
  // (semblable à prog03_txt_tcp_server.cpp et prog04_bin_tcp_server.cpp)
  //
  crs::close(txtListenSocket);
  crs::close(binListenSocket);
  // ...

  return 0;
}

//----------------------------------------------------------------------------
