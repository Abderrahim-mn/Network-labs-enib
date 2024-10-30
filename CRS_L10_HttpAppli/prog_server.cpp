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
  // server to let it know that this websocket is used by this
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
  //   (except if the client requested « close »),
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
}

void
dialog_thread(SOCKET dialog_socket,
              Server &server,
              Application &application)
{
  SSL *ssl=nullptr;
  try
  {
    http_dialog(dialog_socket, server, application);
    // ...{2}...
    // Expect a boolean parameter in order to determine whether an SSL
    // connection is needed or not.
    // In this case, calling « make_ssl() » on the server will embed
    // the provided socket in such an SSL connection.
    // Then « http_dialog() » can be called with this SSL connection
    // instead of the socket.
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

  return 0;
}

//----------------------------------------------------------------------------
