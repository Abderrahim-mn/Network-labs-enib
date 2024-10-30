//----------------------------------------------------------------------------

#include "crsUtils.hpp"

void
dialogThread(SOCKET dialogSocket)
{
  try
  {
    //---- ensure small packets are sent straight away ----
    crs::setTcpNodelayOption(dialogSocket, true);

    //---- reuse connection as much as possible ----
    for(;;)
    {
      //---- receive and analyse HTTP request line by line ----
      std::string requestMethod, requestUri, connection, upgrade, wsKey;
      //
      // ... À COMPLÉTER ...
      //
      // Lire et analyser complètement l'en-tête de la requête HTTP afin d'en
      // extraire les informations suivantes lorsqu'elles sont disponibles :
      // - première ligne --> variables ``requestMethod'' et  ``requestUri''
      // - ligne d'option ``Connection:'' --> variable ``connection''
      // - ligne d'option ``Upgrade:'' --> variable ``upgrade''
      // - ligne d'option ``Sec-WebSocket-Key:'' --> variable ``wsKey''
      //

      // ...
      if(empty(requestMethod))
      {
        break; // no request
      }
      connection=crs::find(connection, "close")!=-1
                 ? "close" : "keep-alive"; // assume keep-alive by default

      //---- handle upgrade request ----
      if(upgrade=="websocket")
      {
        crs::out("--> upgrading to websocket\n");

        //---- prepare storage for a block of integer values ----
        constexpr int blockSize=256;
        auto storage=std::make_unique<int32_t []>(blockSize);
        int32_t *block=storage.get();
        const int blockBytes=int(blockSize*sizeof(int32_t));

        //---- prepare initial state ----
#if !defined NDEBUG
        crs::err("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                 "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        crs::err("  Built for debug purpose at first "
                 "(measurements will not be relevant).\n");
        crs::err("  For an actual experiment, rebuild with: "
                 "   make rebuild opt=1\n");
        crs::err("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"
                 "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
#endif
        int blockCount=0;
        const double t0=crs::gettimeofday();
        const double e0=crs::cpuEnergy();

        //---- prepare and send reply header ----
        //
        // ... À COMPLÉTER ...
        //
        // Rédiger et envoyer l'en-tête de réponse HTTP qui accepte
        // l'usage de la websocket identifiée par ``wsKey''.
        //

        // ...

        //---- as long as some blocks are provided by the client ----
        for(;;)
        {
          //---- receive next block (consider byte-order) ----
          //
          // ... À COMPLÉTER ...
          //
          // Recevoir depuis le client un message websocket binaire dont les
          // ``blockBytes'' octets seront placés à l'adresse ``block''.
          // En cas d'impossibilité (tous les blocs ont été reçus), quitter
          // avec ``break;'' la boucle de traitement des blocs.
          // nb : les ``blockSize'' entiers de 32 bits qui sont alors obtenus
          //      dans ``block'' sont dans l'ordre réseau ; ils ne sont donc
          //      pas directement exploitables.
          //

          // ...

          //---- change values in block ----
          for(int i=0; i<blockSize; ++i)
          {
            block[i]+=i;
          }
          ++blockCount; // one more block has just been processed

          //---- send back changed block (consider byte-order) ----
          //
          // ... À COMPLÉTER ...
          //
          // Envoyer au client un message websocket binaire contenant les
          // ``blockBytes'' octets situés à l'adresse ``block''.
          // nb : les ``blockSize'' entiers de 32 bits préparés dans ``block''
          //      sont dans l'ordre hôte ; ils ne sont donc pas directement
          //      transmissibles.
          //

          // ...
        }

        //---- display performances in final state ----
        const double energy=crs::cpuEnergy()-e0;
        const double duration=crs::gettimeofday()-t0;
        crs::out("2x% blocks in % s (2x% block/s, % Joules)\n",
                 blockCount, duration, blockCount/duration, energy);

        break; // websocket dialog stops here
      }

      //---- handle ``GET /'' request ----
      if((requestMethod=="GET")&&(requestUri=="/"))
      {
        const std::string path="ws_perf.html";
        crs::out("--> sending file: %\n", path);

        //
        // ... À COMPLÉTER ...
        //
        // Produire une réponse HTTP qui fournisse au client le contenu du
        // fichier dont le nom est donné par ``path'' (supposé existant).
        // Suite à cette réponse, la gestion de la connexion devra respecter
        // l'option ``connection'' (``close'' ou ``keep-alive'').
        //

        // ...
      }

      //---- any other unhandled case ----
      if(true) // the last resort!
      {
        crs::out("--> sending 404 Not Found: % %\n",
                 requestMethod, requestUri);

        //
        // ... À COMPLÉTER ...
        //
        // Produire une réponse HTTP qui indique au client que la requête
        // formulée ne correspond à aucune ressource prévue.
        // Suite à cette réponse, la gestion de la connexion devra respecter
        // l'option ``connection'' (``close'' ou ``keep-alive'').
        //

        // ...
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
  // ... À COMPLÉTER ...
  //
  // Réaliser un serveur HTTP qui soit accessible sur le port ``portNumber''.
  //
  // Chaque nouvelle connexion d'un client doit être traitée dans un thread
  // exécutant la fonction ``dialogThread()'' définie plus haut.
  //

  // ...

  return 0;
}

//----------------------------------------------------------------------------
