//----------------------------------------------------------------------------

#include "application.hpp"
#include "server.hpp"

std::string // reply to client
handle_http_message(Server &server,
                    Application &application,
                    const std::string &incoming)
{
  const auto lock=server.lock(); // exclusive access to application
  // ...{3}...
  // Use « process_incoming_message() » on the application in order to
  // provide it with the incoming message; obtain the current client
  // session.
  // Then, use « consume_pending_messages() » on the application in
  // order to retrieve the reply to be sent back to the client.
  return ""; // ... provide actual reply ...
}

template<typename WebSocketType> // could be either « SOCKET » or « SSL * »
void
handle_websocket(Server &server,
                 Application &application,
                 WebSocketType websocket)
{
  // ...{4}...
  // This is the main loop for the websocket dialog.
  // Each time a message is received, the server must be locked in order
  // to ensure an exclusive access to the application.
  // This incoming message is provided to the application with its
  // « process_incoming_message() » function.
  // The resulting current client session will be passed with the
  // websocket to the « register_session_websocket() » function of the
  // server to let him know that this websocket is used by this
  // specific client session.
  // Since some reply messages might be available for any client session
  // (not only the current one), we have to consider all the client
  // sessions and deliver their messages via their respective websockets.
  // In order to do so, the « foreach_session_websocket() » of the server
  // must be called with a lambda-closure similar to this one:
  /*
    [&](const auto &session, const auto &websocket)
    {
      // Obtain, with « consume_pending_messages() » on the application,
      // the message to be delivered to « session ».
      // If this message is not empty, then send it to the corresponding
      // « websocket ».
    }
  */
  // When exiting the loop, calling « forget_session_websocket() » on the
  // server makes it consider that this websocket is about to be closed.
}

template<typename SocketType> // could be either « SOCKET » or « SSL * »
void
http_dialog(SocketType dialog_socket,
            Server &server,
            Application &application)
{
  // ...{1}...
  // The HTTP dialog with the client must have these properties:
  // • it relies on « keep-alive » connections as much as possible
  //   (expect if the client requested « close »),
  // • the request « GET / » should deliver the « client.html » file,
  // • the request « GET /favicon.ico » should deliver the « enib.ico » file,
  // • any other request ends with a « 404 » error reply.
  //
  // ...{3}...
  // In order to make the application work, the HTTP dialog will have to
  // consider « POST /msg » requests.
  // The associated content must be provided to « handle_http_message() »
  // in order to deliver the result of this call to the client.
  //
  // ...{4}...
  // A websocket upgrade via « GET /channel » will  have to be considered.
  // All the dialog via this websocket will take place in
  // « handle_websocket() ».

  for(;;)
  {
    //---- receive and analyse HTTP request line by line ----
    std::string request_method, request_uri,
                connection, content_length;
    for(;;)
    {
      const auto l=crs::recvLine(dialog_socket);
      // crs::out("header: %", l);
      if(empty(l)||(l=="\n")||(l=="\r\n"))
      {
        break; // end of header
      }
      if(empty(request_method)) // first line
      {
        crs::extract(l, request_method, request_uri);
      }
      else if(crs::extract(l, "Connection:", connection)==2)
      {
        // nothing more to be done
      }
      else if(crs::extract(l, "Content-Length:", content_length)==2)
      {
        // nothing more to be done
      }
    }
    if(empty(request_method))
    {
      break; // no request
    }
    connection=crs::find(connection, "close")!=-1
               ? "close" : "keep-alive"; // assume keep-alive by default
    const auto length=empty(content_length)
                      ? 0 : std::stoi(content_length);
        //---- handle file transfer (hardcoded) ----
    if((request_method=="GET")&&
       ((request_uri=="/")||(request_uri=="/favicon.ico")))
    {
      const auto path=request_uri=="/"
                      ? "client.html" : "enib.ico";
      const auto content_type=request_uri=="/"
                              ? "text/html" : "image/vnd.microsoft.icon";
      const auto header=
        crs::txt("HTTP/1.1 200 OK\r\n"
                 "Connection: %\r\n"
                 "Content-Type: %\r\n"
                 "Content-Length: %\r\n"
                 "\r\n",
                 connection,
                 content_type,
                 crs::fileSize(path));
      crs::sendAll(dialog_socket, header);
      auto buffer=std::array<char, 0x400>{};
      const auto input=crs::openR(path);
      for(;;)
      {
        const auto r=crs::read(input, data(buffer), crs::len(buffer));
        if(r==0)
        {
          break; // EOF
        }
        crs::sendAll(dialog_socket, data(buffer), r);
      }
      crs::close(input);
      if(connection=="close")
      {
        break; // dialog stops here
      }
      else
      {
        continue; // done with reply, but keep-alive
      }
    }

        //---- any other unhandled case ----
    if(true) // the last resort!
    {
      crs::out("--> sending 404 Not Found: % %\n",
               request_method, request_uri);
      const auto content=
        crs::txt("<!DOCTYPE html>\n"
                 "<html><head>\n"
                 "<meta charset=\"utf-8\">\n"
                 "</head><body>\n"
                 "<h2>404 - Not Found</h2>\n"
                 "<p>[<a href=\"/\">home</a>]</p>\n"
                 "<hr>\n"
                 "<p><i>method:</i> <b>%</b></p>\n"
                 "<p><i>uri:</i> <b>%</b></p>\n"
                 "<hr>\n"
                 "</body></html>\n",
                 request_method,
                 request_uri);
      const auto header=
        crs::txt("HTTP/1.1 404 Not Found\r\n"
                 "Connection: %\r\n"
                 "Content-Type: text/html\r\n"
                 "Content-Length: %\r\n"
                 "\r\n",
                 connection,
                 crs::len(content));
      crs::sendAll(dialog_socket, header);
      crs::sendAll(dialog_socket, content);
      if(connection=="close")
      {
        break; // dialog stops here
      }
      else
      {
        continue; // done with reply, but keep-alive
      }
    }
  }
}

void
dialog_thread(SOCKET dialog_socket,
              Server &server,
              Application &application,
	      bool use_ssl)
{
  SSL *ssl=nullptr;
  try
  {
    // ...{2}...
    // Expect a boolean parameter in order to determine whether an SSL
    // connection is needed or not.
    // In this case, calling « make_ssl() » on the server will embed
    // the provided socket in such an SSL connection.
    // Then « http_dialog() » can be called with this SSL connection
    // instead of the socket.
    if(use_ssl)
    {
      ssl=server.make_ssl(dialog_socket);
      http_dialog(ssl, server, application);
    }
    else
    {
      http_dialog(dialog_socket, server, application);
    }
  }
  catch(const std::exception &e)
  {
    crs::err("\n!!!!!!!! Exception !!!!!!!!\n%\n", e.what());
  }
  catch(...)
  {
    crs::err("\n!!! Unknown exception !!!\n");
  }
  //---- close dialog socket in any case! ----
  crs::sslClose(ssl);
  crs::close(dialog_socket);
  crs::out("client disconnected\n");
}

int
main(int argc,
     char **argv)
{
  const auto args=std::vector<std::string>{argv, argv+argc};

  //---- check command line arguments ----
  if((crs::len(args)!=2)&&(crs::len(args)!=3))
  {
    crs::err("usage: % http_port [https_port]\n", args[0]);
    crs::exit(1);
  }

  //---- extract local port numbers ----
  const auto http_port_number=uint16_t(std::stoi(args[1]));
  const auto https_port_number=uint16_t(crs::len(args)!=3
                                        ? 0 : std::stoi(args[2]));

  //--- prepare application ----
  auto application=Application{};
  auto server=Server{};

  // ...{1}...
  // Set up an HTTP server listening to « http_port_number ».
  // Each connection must start a thread running « dialog_thread() ».
  //
  // ...{2}...
  // Set up an HTTPS server listening to « https_port_number ».
  // Both the HTTP and the HTTPS servers have to be considered when
  // accepting connections.
  // A boolean parameter must be added to « dialog_thread() » in
  // order to distinguish these two kinds of connection.

  //---- create listen socket (http) ----
  const auto http_listen_socket=crs::socket(PF_INET, SOCK_STREAM, 0);
  // ... avoiding timewait problems (optional)
  crs::setReuseAddrOption(http_listen_socket, true);
  // ... bound to the specified port
  crs::bind(http_listen_socket, http_port_number);
  // ... listening to connections
  crs::listen(http_listen_socket);
  crs::out("host '%' waiting for http connections on port '%'...\n",
           crs::gethostname(), http_port_number);

  auto https_listen_socket=INVALID_SOCKET;
  if(https_port_number!=0) // second port may not have been provided
  {
    //---- create listen socket (https) ----
    https_listen_socket=crs::socket(PF_INET, SOCK_STREAM, 0);
    // ... avoiding timewait problems (optional)
    crs::setReuseAddrOption(https_listen_socket, true);
    // ... bound to the specified port
    crs::bind(https_listen_socket, https_port_number);
    // ... listening to connections
    crs::listen(https_listen_socket);
    crs::out("host '%' waiting for https connections on port '%'...\n",
             crs::gethostname(), https_port_number);
  }


  for(;;)
  {
    //---- wait for connections ----
    auto read_set=std::vector<SOCKET>{http_listen_socket};
    if(https_port_number!=0) // second port may not have been provided
    {
      read_set.emplace_back(https_listen_socket);
    }
    crs::select(read_set);
        //---- react to http connection (if available) ----
    if(crs::find(read_set, http_listen_socket)!=-1)
    {

      const auto [dialog_socket, from_ip_addr, from_port]=
        crs::acceptfrom(http_listen_socket);
      crs::out("new http connection from %:%\n",
               crs::formatIpv4Address(from_ip_addr), from_port);
      //---- start a new dialog thread ----
      auto th=std::thread{dialog_thread,
                          dialog_socket,
                          std::ref(server),
                          std::ref(application),
                          false};
      th.detach();
    }

    //---- react to https connection (if available) ----
    if(crs::find(read_set, https_listen_socket)!=-1)
    {
      const auto [dialog_socket, from_ip_addr, from_port]=
        crs::acceptfrom(https_listen_socket);
      crs::out("new https connection from %:%\n",
               crs::formatIpv4Address(from_ip_addr), from_port);
      //---- start a new dialog thread ----
      auto th=std::thread{dialog_thread,
                          dialog_socket,
                          std::ref(server),
                          std::ref(application),
                          true};
      th.detach();
    }

  }
  crs::close(http_listen_socket);
  return 0;
}

//----------------------------------------------------------------------------
