//----------------------------------------------------------------------------

#ifndef SERVER_HPP
#define SERVER_HPP

#include "crsUtils.hpp"

class Server
{
public:

  Server() =default;

  SSL *
  make_ssl(SOCKET socket)
  {
    return crs::sslAccept(socket, ssl_ctx_);
  }

  [[nodiscard]] std::lock_guard<std::mutex>
  lock()
  {
    return std::lock_guard<std::mutex>{mtx_};
  }

  template<typename WebSocketType>
  void
  register_session_websocket(int session,
                             WebSocketType websocket)
  {
    const auto it=std::find_if(
      begin(session_websockets_), end(session_websockets_),
      [&](const auto &sws)
      {
        return sws.session==session;
      });
    auto &sws=it!=end(session_websockets_)
             ? *it : session_websockets_.emplace_back();
    if constexpr(std::is_same_v<SOCKET, WebSocketType>)
    {
      sws=SessionWebSocket{session, websocket, nullptr};
    }
    else
    {
      sws=SessionWebSocket{session, INVALID_SOCKET, websocket};
    }
  }

  void
  forget_session_websocket(int session)
  {
    const auto it=std::find_if(
      cbegin(session_websockets_), cend(session_websockets_),
      [&](const auto &sws)
      {
        return sws.session==session;
      });
    if(it!=cend(session_websockets_))
    {
      session_websockets_.erase(it);
    }
  }

  template<typename SessionWebSocketFnct>
  void
  foreach_session_websocket(SessionWebSocketFnct fnct) const
  {
    for(const auto &sws: session_websockets_)
    {
      if(sws.ssl!=nullptr)
      {
        fnct(sws.session, sws.ssl);
      }
      else if(sws.socket!=INVALID_SOCKET)
      {
        fnct(sws.session, sws.socket);
      }
    }
  }

  // prevent from copying/moving (internal resource management)
  // nb: this is redundant with the std::mutex member
  Server(const Server &) =delete;
  Server & operator=(const Server &) =delete;
  Server(Server &&) =delete;
  Server& operator=(Server &&) =delete;
  ~Server()
  {
#if USE_SSL
    crs::sslDestroy(ssl_ctx_);
#endif
  }

private:

  struct SessionWebSocket
  {
    int session{};
    SOCKET socket{INVALID_SOCKET};
    SSL *ssl{};
  };

  std::mutex mtx_{};
  std::vector<SessionWebSocket> session_websockets_{};
#if USE_SSL
  SSL_CTX *ssl_ctx_{crs::sslInit({}, "cert.pem", "key.pem")};
  /*
    The self-signed certificate "cert.pem" and its private key "key.pem"
    were obtained with the following command:
      openssl req -x509 -newkey rsa:2048 -nodes -days 3650 \
              -keyout key.pem -out cert.pem \
              -subj '/C=FR/ST=Bretagne/L=Brest/O=ENIB/OU=CRS/CN=localhost'
  */
#else
  SSL_CTX *ssl_ctx_{};
#endif
};

#endif // SERVER_HPP

//----------------------------------------------------------------------------
