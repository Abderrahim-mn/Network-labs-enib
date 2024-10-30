//----------------------------------------------------------------------------

#include "crsUtils.hpp"

void
dialogThread(SOCKET dialogSocket)
{
  try
  {
    //---- reuse connection as much as possible ----
    for(;;)
    {
      //---- receive and analyse HTTP request line by line ----
      std::string requestMethod, requestUri,
                  connection, contentLength,
                  upgrade, wsKey;
      //
      // ... À COMPLÉTER {B-1} ...
      //
      // Effectuer une boucle de lecture de l'en-tête complet de la requête.
      // Chaque ligne ``l'' est obtenue avec ``crs::recvLine()'' depuis la
      // socket.
      // * Si une ligne est vide ou n'est constituée que de "\n" ou "\r\n",
      //   c'est que l'en-tête est fini ; il faut quitter cette boucle de
      //   lecture pour passer à la suite.
      // * Si ``requestMethod'' est vide, c'est qu'il s'agit de la première
      //   ligne de l'en-tête ; il faut renseigner ``requestMethod'' et
      //   ``requestUri'' avec les deux premiers mots de cette ligne en
      //   utilisant ``crs::extract(l, requestMethod, requestUri)''.
      // * Sinon, nous pouvons tenter de renseigner ``connection'' en
      //   utilisant ``crs::extract(l, "Connection:", connection)''.
      // * De la même façon, nous pouvons tenter de renseigner
      //   ``contentLength'' si la ligne obtenue commence par
      //   ``Content-Length:''.
      // * Ou encore, nous pouvons tenter de renseigner ``upgrade''
      //   si la ligne obtenue commence par ``Upgrade:''.
      // * Et enfin, nous pouvons tenter de renseigner ``wsKey''
      //   si la ligne obtenue commence par ``Sec-WebSocket-Key:''.
      // nb: tous les champs extraits ne serviront pas dans l'immédiat
      //     mais seront utiles dans des étapes ultérieures du sujet.
      //
      for(;;)
      {
        auto l=crs::recvLine(dialogSocket);
        crs::out("header: %", l);
        if(empty(l)||(l=="\n")||(l=="\r\n"))
        {
          break; // end of header
        }
        if(empty(requestMethod)) // first line
        {
          crs::extract(l, requestMethod, requestUri);
        }
        else if(crs::extract(l, "Connection:", connection)==2)
        {
          // nothing more to be done
        }
        else if(crs::extract(l, "Content-Length:", contentLength)==2)
        {
          // nothing more to be done
        }
        else if(crs::extract(l, "Upgrade:", upgrade)==2)
        {
          // nothing more to be done
        }
        else if(crs::extract(l, "Sec-WebSocket-Key:", wsKey)==2)
        {
          // nothing more to be done
        }
      }
      // ...
      if(empty(requestMethod))
      {
        break; // no request
      }
      connection=crs::find(connection, "close")!=-1
                 ? "close" : "keep-alive"; // assume keep-alive by default
      const int length=empty(contentLength) ? 0 : std::stoi(contentLength);

      //---- handle ``POST /txt'' request ----
      if((requestMethod=="POST")&&(requestUri=="/txt"))
      {
        //---- receive and display text ----
        //
        // ... À COMPLÉTER {E-1} ...
        //
        // Extraire exactement ``length'' octets (la quantité indiquée par
        // l'option ``Content-Length:'' de la requête) qui suivent l'en-tête
        // de la requête afin de constituer une chaîne de caractères.
        // Si la chaîne obtenue n'a pas la longueur ``length'' promise,
        // il faut alors mettre fin au dialogue.
        // Nous afficherons la chaîne reçue afin de rendre explicite le
        // fonctionnement de notre application simpliste.
        //
        auto txt=crs::recvAll(dialogSocket, length);
        if(crs::len(txt)!=length)
        {
          throw std::runtime_error{"cannot receive text content"};
        }
        crs::out("http_txt: <%>\n", txt);
        // ...

        //---- prepare and send back some text as reply ----
        //
        // ... À COMPLÉTER {E-2} ...
        //
        // Fabriquer une chaîne de caractère qui constituera la réponse
        // du serveur.
        // Un en-tête de réponse HTTP devra alors être rédigé ; il reprendra
        // les options habituelles, et en particulier ici :
        // - ``Content-Type:'' indiquera ``text/plain'' (du simple texte),
        // - ``Content-Length:'' indiquera la longueur de la chaîne
        //   fabriquée précédemment.
        // Il s'agit ensuite d'envoyer cet en-tête puis la chaîne en question.
        //
        auto content=crs::txt("server received {%}", txt);
        auto header=crs::txt("HTTP/1.1 200 OK\r\n"
                             "Connection: %\r\n"
                             "Content-Type: text/plain\r\n"
                             "Content-Length: %\r\n"
                             "\r\n",
                             connection,
                             crs::len(content));
        crs::sendAll(dialogSocket, header);
        crs::sendAll(dialogSocket, content);
        //...

        if(connection=="close")
        {
          break; // dialog stops here
        }
        else
        {
          continue; // done with reply, but keep-alive
        }
      }

      //---- handle ``POST /bin'' request ----
      if((requestMethod=="POST")&&(requestUri=="/bin"))
      {
        //---- receive binary data ----
        int32_t values[0x100];
        if(length>int(sizeof(values)))
        {
          throw std::runtime_error{"insufficient storage for binary data"};
        }
        //
        // ... À COMPLÉTER {E-3} ...
        //
        // Extraire exactement ``length'' octets (la quantité indiquée par
        // l'option ``Content-Length:'' de la requête) qui suivent l'en-tête
        // de la requête afin renseigner le tableau ``values''.
        // Si le nombre d'octets obtenus ne vaut pas ``length'' comme promis,
        // il faut alors mettre fin au dialogue.
        //
        
	// ...

        //---- display and change values (consider byte-order) ----
        const int valueCount=length/int(sizeof(values[0]));
        for(int i=0; i<valueCount; ++i)
        {
          values[i]=crs::ntoh_i32(values[i]);
        }
        std::string msg;
        for(int i=0; i<valueCount; ++i)
        {
          msg+=crs::txt(" %", values[i]);
        }
        crs::out("http_bin:%\n", msg);
        for(int i=0; i<valueCount; ++i)
        {
          values[i]+=i+1;
        }
        for(int i=0; i<valueCount; ++i)
        {
          values[i]=crs::hton_i32(values[i]);
        }

        //---- send back changed binary data as reply ----
        //
        // ... À COMPLÉTER {E-4} ...
        //
        // Un en-tête de réponse HTTP devra être rédigé pour annoncer l'envoi
        // du tableau modifié comme réponse ; il reprendra les options
        // habituelles, et en particulier ici :
        // - ``Content-Type:'' indiquera ``application/octet-stream''
        //   (des données brutes),
        // - ``Content-Length:'' indiquera la valeur de ``length'' (nous
        //   n'avons pas changé ici la taille du tableau reçu, juste ses
        //   valeurs).
        // Il s'agit ensuite d'envoyer cet en-tête puis ``length'' octets
        // du tableau ``values''.
        //
        
	// ...

        if(connection=="close")
        {
          break; // dialog stops here
        }
        else
        {
          continue; // done with reply, but keep-alive
        }
      }

      //---- handle upgrade to websocket request ----
      if((requestMethod=="GET")&&(requestUri=="/")&&(upgrade=="websocket"))
      {
        crs::out("--> upgrading to websocket\n");

        //---- prepare and send reply header ----
        //
        // ... À COMPLÉTER {F-1} ...
        //
        // Un en-tête de réponse HTTP devra être rédigé et envoyé qu client
        // pour confirmer au client le changement de protocole.
        // Le code de réponse est ``101'' accompagné du texte
        // ``Switching Protocols''.
        // L'option ``Connection:'' doit indiquer ``Upgrade''.
        // L'option ``Upgrade:'' doit indiquer ``websocket''.
        // L'option ``Sec-WebSocket-Accept:'' doit indiquer la valeur
        // de ``crs::wsHandshake(wsKey)'' (transformation de la clef
        // reçue lors de la requête).
        //
        
	// ...

        //---- as long as the dialog with the client lasts ----
        char buffer[0x400];
        int msgCount=0;
        for(;;)
        {
          //---- wait for some incoming information at most during 0.5 s ----
          std::vector<SOCKET> readSet={dialogSocket};
          crs::select(readSet, 0.5);

          //---- react to incoming information (if available) ----
          if(crs::find(readSet, dialogSocket)!=-1)
          {
            //---- acquire message ----
            //
            // ... À COMPLÉTER {F-2} ...
            //
            // Utiliser la fonction ``crs::wsRecv()'' à trois arguments
            // pour recevoir un message websocket d'une longueur maximale
            // de ``sizeof(buffer)'' octets qui sera placé dans ``buffer''.
            // Ceci doit fournir deux résultats : ``opcode'' et ``length''.
            // Si la longueur ``length'' obtenue est nulle il faut mettre
            // fin à l'échange avec l'instruction ``break;''.
            //
             auto [opcode, length]=
               std::tuple{crs::WS_NONE, 0}; // ... À REMPLACER ...
            
	    // ...

            if(opcode==crs::WS_TXT) //---- handle textual message ----
            {
              //
              // ... À COMPLÉTER {F-3} ...
              //
              // Afficher le message textuel obtenu dans ``buffer''.
              // S'il est équivalent à "quit", nous supposons que le client
              // demande la fin de l'application ; dans ce cas, nous
              // envoyons un message de fermeture avec ``crs::wsSendClose()''
              // puis nous mettons fin à l'échange avec ``break;''.
              //
              // ...
	      //
            }
            else if(opcode==crs::WS_BIN) //---- handle binary message ----
            {
              auto values=reinterpret_cast<int32_t *>(buffer);
              int valueCount=length/int(sizeof(*values));
              //
              // ... À COMPLÉTER {F-4} ...
              //
              // Afficher les ``valueCount'' valeurs entières désignées
              // par ``values''.
              // Attention, ce sont des données binaires telles que reçues
              // depuis le réseau.
              //
              // ...
            }
            else if(opcode==crs::WS_PING) //---- handle ping ----
            {
              crs::out("ws_ping\n");
              //
              // ... À COMPLÉTER {F-5} ...
              //
              // La réception d'un message ``WS_PING'' doit donner lieu à
              // l'envoi, avec ``crs::wsSend()'' à quatre arguments, d'un
              // message ``WS_PONG'' répétant simplement les ``length''
              // octets reçus dans ``buffer''.
              //
              // ...
            }
            else if(opcode==crs::WS_CLOSE) //---- handle client close ----
            {
              crs::out("ws_close\n");
              //
              // ... À COMPLÉTER {F-6} ...
              //
              // La réception d'un message ``WS_CLOSE'' doit donner lieu à
              // l'envoi, avec ``crs::wsSend()'' à quatre arguments, d'un
              // message ``WS_CLOSE'' répétant simplement les ``length''
              // octets reçus dans ``buffer''.
              // Il faut ensuite mettre fin à l'échange avec ``break;''.
              //
              // ...
            }
          }

          //---- spontaneously produce new messages ----
          if(msgCount++%2==0)
          {
            //---- send text message to client ----
            auto msg=crs::txt("message % from server", msgCount);
            //
            // ... À COMPLÉTER {F-7} ...
            //
            // Envoyer, avec ``crs::wsSend()'' à deux arguments, le message
            // textuel ``msg''.
            //
            // ...
          }
          else
          {
            //---- send binary data to client ----
            std::array<int32_t, 10> values;
            for(int i=0; i<crs::len(values); ++i)
            {
              values[i]=100*i+msgCount;
            }
            //
            // ... À COMPLÉTER {F-8} ...
            //
            // Envoyer, avec ``crs::wsSend()'' à trois arguments, les
            // ``sizeof(values)'' octets situés à l'adresse ``data(values)''.
            // Attention, ce sont des données binaires représentant des
            // entiers de 32 bits tels que mémorisés sur l'hôte.
            //
            // ...
          }
        }

        break; // websocket dialog stops here
      }

      //---- deduce filesystem path from request URI ----
      auto path="TopDir"+crs::split(requestUri, "?").front(); // remove ?...
      if(crs::isFile(path+"/index.html"))
      {
        path+="/index.html"; // use index.html found in directories
      }

      //---- handle CGI process execution ----
      if(crs::isFile(path)&&                        // ensure it is a file
         crs::startsWith(requestUri, "/cgi-bin/")&& // standing in /cgi-bin/
         crs::access(path, R_OK|X_OK))              // and which is executable
      {
        crs::out("--> executing CGI file: %\n", path);
        connection="close"; // no easy way to determine content length
        //
        // ... À COMPLÉTER {G-1} ...
        //
        // Rédiger (avec ``crs::txt()'') le __début__de l'en-tête de la réponse
        // HTTP (200 OK) en y insérant ``connection'' ; il ne faut surtout
        // pas produire la ligne vide marquant la fin de l'en-tête car c'est le
        // processus CGI qui s'en chargera après avoir complété l'en-tête.
        // Utiliser ensuite ``crs::sendAll()'' pour l'envoyer vers la socket.
        //
        // ...

        //
        // ... À COMPLÉTER {G-2} ...
        //
        // Créer un processus enfant, et se contenter de l'attendre.
        // Le processus enfant doit renseigner avec ``crs::setenv()''
        // les variables d'environnement "REQUEST_METHOD", "REQUEST_URI"
        // et "CONTENT_LENGTH" depuis les valeurs des variables ayant des
        // noms similaires dans notre programme.
        // Il doit ensuite effectuer une redirection des entrées/sorties
        // (avec ``crs::dup2()'') dans la socket de dialogue.
        // Il ne lui reste plus qu'à provoquer son recouvrement par le
        // programme CGI choisi (avec crs::exec()'').
        //
        // ...
        if(connection=="close")
        {
          break; // dialog stops here
        }
        else
        {
          continue; // done with reply, but keep-alive
        }
      }

      //---- handle file transfer ----
      if((requestMethod=="GET")&&
         crs::isFile(path)&&      // ensure it is a file
         crs::access(path, R_OK)) // which is readable
      {
        crs::out("--> sending file: %\n", path);

        //---- deduce content type from path extension (ugly!) ----
        std::string contentType;
        if(crs::endsWith(path, ".html"))
        {
          contentType="text/html";
        }
        else if(crs::endsWith(path, ".png"))
        {
          contentType="image/png";
        }
        else if(crs::endsWith(path, ".ico"))
        {
          contentType="image/vnd.microsoft.icon";
        }
        else
        {
          contentType="unknown/unknown";
        }

        //
        // ... À COMPLÉTER {C-1} ...
        //
        // Rédiger (avec ``crs::txt()'') l'en-tête complet de la réponse HTTP
        // (200 OK) en y inserrant ``connection'', ``contentType'' et la
        // taille du fichier à transférer (avec ``crs::fileSize()'').
        // Utiliser ensuite ``crs::sendAll()'' pour l'envoyer vers la socket.
        //
        auto header=crs::txt("HTTP/1.1 200 OK\r\n"
                             "Connection: %\r\n"
                             "Content-Type: %\r\n"
                             "Content-Length: %\r\n"
                             "\r\n",
                             connection,
                             contentType,
                             crs::fileSize(path));
        crs::sendAll(dialogSocket, header);
        // ...

        char buffer[0x400];
        //
        // ... À COMPLÉTER {C-2} ...
        //
        // Ouvrir le fichier avec ``crs::openR()'', puis effectuer en boucle :
        // * Obtenir ``r'' octets depuis le fichier vers ``buffer'' à l'aide
        //   de ``crs::read()''.
        //   (nb : il s'agit de données brûtes, pas forcément de texte)
        // * Si ``r'' est nul (fin-de-fichier), il faut quitter cette boucle.
        // * Utiliser ``crs::sendAll()'' pour envoyer vers la socket les ``r''
        //   octets précédemment obtenus dans ``buffer''.
        // Après cette boucle, il ne reste plus qu'à fermer le fichier avec
        // ``crs::close()''.
        //
        int input=crs::openR(path);
        for(;;)
        {
          int r=crs::read(input, buffer, sizeof(buffer));
          if(r==0)
          {
            break; // EOF
          }
          crs::sendAll(dialogSocket, buffer, r);
        }
        crs::close(input);
        // ...
        if(connection=="close")
        {
          break; // dialog stops here
        }
        else
        {
          continue; // done with reply, but keep-alive
        }
      }

      //---- handle directory content ----
      if((requestMethod=="GET")&&
         crs::isDir(path)&&       // ensure it is a directory
         crs::access(path, R_OK)) // which is readable
      {
        crs::out("--> sending directory content: %\n", path);

        // prepare content first to provide the header with its length
        auto content=crs::txt("<!DOCTYPE html>\n"
                              "<html><head>\n"
                              "<meta charset=\"utf-8\">\n"
                              "</head><body>\n"
                              "<h2>Directory %</h2>\n"
                              "<p>[<a href=\"/\">home</a>]</p>\n"
                              "<hr>\n"
                              "<p><ul>\n"
                              "<li>[<a href=\"../\">../</a>]\n",
                              requestUri);
        for(auto &f: crs::listDir(path))
        {
          if(crs::isDir(path+"/"+f))
          {
            f+='/';
          }
          content+=crs::txt("<li><a href=\"%\">%</a></li>\n", f, f);
        }
        content+="</ul></p>\n"
                 "<hr>\n"
                 "</body></html>\n";

        //
        // ... À COMPLÉTER {D} ...
        //
        // Rédiger (avec ``crs::txt()'') l'en-tête complet de la réponse
        // HTTP (200 OK) en y insérant ``connection'' et la taille du contenu
        // préalablement préparé.
        // Utiliser ensuite ``crs::sendAll()'' pour l'envoyer vers la socket
        // puis faire de même avec le contenu.
        //
        auto header=crs::txt("HTTP/1.1 200 OK\r\n"
                             "Connection: %\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: %\r\n"
                             "\r\n",
                             connection,
                             crs::len(content));
        crs::sendAll(dialogSocket, header);
        crs::sendAll(dialogSocket, content);
        //...
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
                 requestMethod, requestUri);

        // prepare content first to provide the header with its length
        auto content=crs::txt("<!DOCTYPE html>\n"
                              "<html><head>\n"
                              "<meta charset=\"utf-8\">\n"
                              "</head><body>\n"
                              "<h2>404 - Not Found</h2>\n"
                              "<p>[<a href=\"/\">home</a>]</p>\n"
                              "<hr>\n"
                              "<p><i>method:</i> <b>%</b></p>\n"
                              "<p><i>uri:</i> <b>%</b></p>\n"
                              "<p><i>path:</i> <b>%</b></p>\n"
                              "<hr>\n"
                              "</body></html>\n",
                              requestMethod,
                              requestUri,
                              path);

        //
        // ... À COMPLÉTER {B-2} ...
        //
        // Rédiger (avec ``crs::txt()'') l'en-tête complet de la réponse
        // HTTP (404 Not Found) en y inserrant ``connection'' et la taille du
        // contenu préalablement préparé.
        // Utiliser ensuite ``crs::sendAll()'' pour l'envoyer vers la socket
        // puis faire de même avec le contenu.
        //
        auto header=crs::txt("HTTP/1.1 404 Not Found\r\n"
                             "Connection: %\r\n"
                             "Content-Type: text/html\r\n"
                             "Content-Length: %\r\n"
                             "\r\n",
                             connection,
                             crs::len(content));
        crs::sendAll(dialogSocket, header);
        crs::sendAll(dialogSocket, content);
        // ...
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
  crs::out("client disconnected\n");
}

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

  crs::out("host '%' waiting for connections on port '%'...\n",
           crs::gethostname(), portNumber);

  //
  // ... À COMPLÉTER {A} ...
  //
  // Réaliser un serveur TCP qui soit accessible sur le port ``portNumber''.
  //
  // Chaque nouvelle connexion d'un client doit être traitée dans un thread
  // exécutant la fonction ``dialogThread()'' définie plus haut.
  //

  //---- create listen socket ----
  SOCKET listenSocket=crs::socket(PF_INET, SOCK_STREAM, 0);
  // ... avoiding timewait problems (optional)
  crs::setReuseAddrOption(listenSocket, true);
  // ... bound to the specified port
  crs::bind(listenSocket, portNumber);
  // ... listening to connections
  crs::listen(listenSocket);

  for(;;)
  {
    //---- accept and display new connection ----
    auto [dialogSocket, fromIpAddr, fromPort]=crs::acceptfrom(listenSocket);
    crs::out("new connection from %:%\n",
             crs::formatIpv4Address(fromIpAddr), fromPort);

    //---- start a new dialog thread ----
    std::thread th{dialogThread, dialogSocket};
    th.detach();
  }

  //---- close listen socket ----
  crs::close(listenSocket); // never reached
  // ...

  return 0;
}

//----------------------------------------------------------------------------
