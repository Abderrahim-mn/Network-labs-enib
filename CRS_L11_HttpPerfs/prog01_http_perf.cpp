//----------------------------------------------------------------------------

#include "crsUtils.hpp"

struct ClientState
{
  int id{-1};
  int blockCount{0};
  double t0{-1.0};
  double e0{-1.0};
};

static std::vector<std::unique_ptr<ClientState>> states_;
static std::mutex stateMtx_{};

ClientState &
getClientState(int clientId)
{
  std::lock_guard<std::mutex> lock{stateMtx_};
  for(auto &elem: states_)
  {
    if(elem->id==clientId)
    {
      return *elem;
    }
  }
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
  crs::out("--> beginning application with client %\n", clientId);
  auto state=std::make_unique<ClientState>();
  state->id=clientId;
  state->t0=crs::gettimeofday();
  state->e0=crs::cpuEnergy();
  return *states_.emplace_back(std::move(state));
}

void
destroyClientState(int clientId)
{
  std::lock_guard<std::mutex> lock{stateMtx_};
  for(auto &elem: states_)
  {
    if(elem->id==clientId)
    {
      const double energy=crs::cpuEnergy()-elem->e0;
      const double duration=crs::gettimeofday()-elem->t0;
      crs::out("--> ending application with client %\n", clientId);
      crs::out("2x% blocks in % s (2x% block/s, % Joules)\n",
               elem->blockCount, duration, elem->blockCount/duration, energy);
      elem=std::move(states_.back());
      states_.pop_back();
      return;
    }
  }
  throw std::runtime_error{"unknown client state "+std::to_string(clientId)};
}

//----------------------------------------------------------------------------

void
dialogThread(SOCKET dialogSocket)
{
  try
  {
    //---- ensure small packets are sent straight away ----
    crs::setTcpNodelayOption(dialogSocket, true);

    //---- prepare storage for a block of integer values ----
    constexpr int blockSize=256;
    auto storage=std::make_unique<int32_t []>(blockSize);
    int32_t *block=storage.get();
    const int blockBytes=int(blockSize*sizeof(int32_t));

    //---- reuse connection as much as possible ----
    for(;;)
    {
      //---- receive and analyse HTTP request line by line ----
      std::string requestMethod, requestUri, connection, contentLength;
      //
      // ... À COMPLÉTER ...
      //
      // Lire et analyser complètement l'en-tête de la requête HTTP afin d'en
      // extraire les informations suivantes lorsqu'elles sont disponibles :
      // - première ligne --> variables ``requestMethod'' et  ``requestUri''
      // - ligne d'option ``Connection:'' --> variable ``connection''
      // - ligne d'option ``Content-Length:'' --> variable ``contentLength''
      //

      // ...
      if(empty(requestMethod))
      {
        break; // no request
      }
      connection=crs::find(connection, "close")!=-1
                 ? "close" : "keep-alive"; // assume keep-alive by default
      const int length=empty(contentLength) ? 0 : std::stoi(contentLength);

      //---- handle ``POST /block?id=XXXX'' request ----
      if(int clientId;
         (requestMethod=="POST")&&
         (crs::extract(requestUri, "/block?id=", clientId)==2))
      {
        //---- retrieve previous/initial state ----
        ClientState &client=getClientState(clientId);

        //---- receive next block (consider byte-order) ----
        if(length!=blockBytes)
        {
          throw std::runtime_error{"bad length for block"};
        }
        //
        // ... À COMPLÉTER ...
        //
        // Recevoir les données de la requête POST.
        // Il s'agit de ``blockBytes'' octets qui seront placés à
        // l'adresse ``block''.
        // En cas d'impossibilité, mettre fin à la communication.
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
        ++client.blockCount; // one more block has just been processed

        //---- send back changed block (consider byte-order) ----
        //
        // ... À COMPLÉTER ...
        //
        // Rédiger et envoyer l'en-tête de réponse HTTP qui indique que
        // ``blockBytes'' octets de type ``application/octet-stream''
        // seront effectivement fournis.
        // Envoyer après cet en-tête, comme promis, les ``blockBytes''
        // octets situés à l'adresse ``block''.
        // Suite à cette réponse, la gestion de la connexion devra respecter
        // l'option ``connection'' (``close'' ou ``keep-alive'').
        // nb : les ``blockSize'' entiers de 32 bits préparés dans ``block''
        //      sont dans l'ordre hôte ; ils ne sont donc pas directement
        //      transmissibles.
        //

        // ...
      }

      //---- handle ``GET /stop?id=XXXX'' request ----
      if(int clientId;
         (requestMethod=="GET")&&
         (crs::extract(requestUri, "/stop?id=", clientId)==2))
      {
        //---- detroy client state ----
        destroyClientState(clientId);

        //---- prepare and send empty reply ----
        //
        // ... À COMPLÉTER ...
        //
        // Rédiger et envoyer l'en-tête de réponse HTTP qui indique que
        // ``0'' octets de type ``application/octet-stream''
        // seront effectivement fournis.
        // Comme indiqué dans l'en-tête, aucun contenu ne le suivra.
        // Suite à cette réponse, la gestion de la connexion devra respecter
        // l'option ``connection'' (``close'' ou ``keep-alive'').
        //

        // ...
      }

      //---- handle ``GET /'' request ----
      if((requestMethod=="GET")&&(requestUri=="/"))
      {
        const std::string path="http_perf.html";
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
