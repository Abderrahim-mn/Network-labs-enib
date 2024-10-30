//----------------------------------------------------------------------------

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "crsUtils.hpp"

class Application
{
public:

  Application() =default;

  int // current client session
  process_incoming_message(const std::string &incoming)
  {
    const auto msg=crs::split(incoming, "|", true, 2);
    auto session=0;
    if(size(msg)>0)
    {
      crs::extract(msg[0], session);
    }
    auto *client=client_with_session_(session);
    const auto cmd=size(msg)>1 ? msg[1] : std::string{};
    if(cmd=="session")
    {
      post_(crs::txt("SET|session|%", client->session), client);
    }
    else if(cmd=="quit")
    {
      remove_client_(*client);
      client=nullptr;
    }
    else if(cmd=="ping")
    {
      // just obtain pending messages
    }
    else if(cmd=="name")
    {
      const auto name=size(msg)>2 ? msg[2]: std::string{};
      change_name_(*client, name);
    }
    else if(cmd=="say")
    {
      const auto msg=crs::split(incoming, "|", true, 3);
      const auto to=size(msg)>2 ? msg[2] :std::string{};
      const auto text=size(msg)>3 ? msg[3] :std::string{};
      say_something_(*client, to, text);
    }
    else
    {
      post_(crs::txt("ERR|unexpected message %", incoming), client);
    }
    auto people=std::string{};
    for(const auto &c: clients_)
    {
      if(!empty(c.name))
      {
        if(!empty(people))
        {
          people+=' ';
        }
        people+=c.name;
      }
    }
    post_(crs::txt("SET|people|%", people));
    return client==nullptr ? 0 : client->session;
  }

  std::string // messages for this client session
  consume_pending_messages(int client_session)
  {
    auto messages=std::string{};
    for(auto &c: clients_)
    {
      if(c.session==client_session)
      {
        messages=crs::join(c.pending_messages, '\n');
        c.pending_messages.clear();
        break;
      }
    }
    return messages;
  }

private:

  struct Client
  {
    int session{};
    std::string name{};
    std::vector<std::string> pending_messages{};
    double last_time{};
  };

  Client *
  client_with_session_(int session)
  {
    const auto now=crs::gettimeofday();
    for(auto i=crs::len(clients_); i--;)
    {
      auto &c=clients_[i];
      if(c.session==session)
      {
        c.last_time=now;
      }
      else if(c.last_time+5.0<now)
      {
        remove_client_(c);
      }
    }
    Client *client=nullptr;
    for(auto &c: clients_)
    {
      if(c.session==session)
      {
        client=&c;
        break;
      }
    }
    if(client==nullptr)
    {
      client=&clients_.emplace_back();
      client->session=++last_session_;
      client->last_time=now;
    }
    return client;
  }

  Client *
  client_with_name_(const std::string &name)
  {
    for(auto &c: clients_)
    {
      if(c.name==name)
      {
        return &c;
      }
    }
    return nullptr;
  }

  void
  post_(const std::string &message,
        Client *client=nullptr)
  {
    if(client==nullptr)
    {
      for(auto &c: clients_)
      {
        c.pending_messages.emplace_back(message);
      }
    }
    else
    {
      client->pending_messages.emplace_back(message);
    }
  }

  void
  remove_client_(const Client &client)
  {
    if(!empty(client.name))
    {
      post_(crs::txt("INFO|goodbye %", client.name));
    }
    const auto idx=&client-data(clients_);
    clients_.erase(cbegin(clients_)+idx);
  }

  void
  change_name_(Client &client,
               std::string name)
  {
    if(empty(name))
    {
      post_(crs::txt("ERR|your name cannot be empty"), &client);
    }
    else if(crs::find(name, ' ')!=-1)
    {
      post_(crs::txt("ERR|your name cannot contain spaces"), &client);
    }
    else if(crs::find(name, '|')!=-1)
    {
      post_(crs::txt("ERR|your name cannot contain `&#124;`"), &client);
    }
    else
    {
      const auto it=std::find_if(cbegin(clients_), cend(clients_),
        [&](const auto &c)
        {
          return (c.name==name)&&(&c!=&client);
        });
      if(it!=cend(clients_))
      {
        post_(crs::txt("ERR|name % already exists", name), &client);
      }
      else if(client.name!=name)
      {
        if(!empty(client.name))
        {
          post_(crs::txt("INFO|% becomes %", client.name, name));
        }
        else
        {
          post_(crs::txt("INFO|welcome %", name));
        }
        client.name=name;
      }
    }
    post_(crs::txt("SET|name|%", client.name), &client);
  }

  void
  say_something_(Client &client,
                 const std::string &to,
                 const std::string &text)
  {
    auto to_split=crs::split(crs::strip(to));
    auto dest=std::vector<Client *>{};
    if(empty(to_split))
    {
      for(auto &c: clients_)
      {
        if(!empty(c.name))
        {
          dest.emplace_back(&c);
        }
      }
    }
    else
    {
      for(const auto &name: to_split)
      {
        if(auto *c=client_with_name_(name); c!=nullptr)
        {
          dest.emplace_back(c);
        }
        else
        {
          post_(crs::txt("ERR|% is unknown", name), &client);
        }
      }
    }
    auto dest_names=std::string{};
    for(const auto &c: dest)
    {
      if(!empty(c->name))
      {
        if(!empty(dest_names))
        {
          dest_names+=' ';
        }
        dest_names+=c->name;
      }
    }
    const auto msg=crs::txt("MSG|%|%|%", client.name, dest_names, text);
    if(!empty(dest)&&(crs::find(dest, &client)==-1))
    {
      dest.emplace_back(&client);
    }
    for(auto &c: dest)
    {
      post_(msg, c);
    }
  }

  std::vector<Client> clients_{};
  int last_session_{};
};

#endif // APPLICATION_HPP

//----------------------------------------------------------------------------
