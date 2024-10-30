//----------------------------------------------------------------------------

#ifndef CRSUTILS_HPP
#define CRSUTILS_HPP 1

#include "platformAdapter.h"

#if defined _WIN32
# define SIGUSR1 10 // signals are not supported on Windows
# define SIGUSR2 12 // ensure compilation succeeds 
# define SIGCHLD 17 // (will fail at run-time)
#endif

#if USE_SSL
# define OPENSSL_API_COMPAT 0x10100000L
# include <openssl/ssl.h>
# include <openssl/err.h>
#else
  struct SSL_CTX; // dummy types for prototypes
  struct SSL;
#endif

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstring>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <functional>
#include <type_traits>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace crs {

// standard array-like non-member functions
using std::data;
using std::size;
using std::empty;
using std::begin;
using std::cbegin;
using std::end;
using std::cend;

//----------------------------------------------------------------------------
// Various utilities: input/output, text manipulation...
//----------------------------------------------------------------------------

std::tuple<std::string, // protocol
           std::string, // hostname
           uint16_t,    // port number
           std::string, // resource
           std::string> // params
parseUriWithParams(const std::string &uri);

std::tuple<std::string, // protocol
           std::string, // hostname
           uint16_t,    // port number
           std::string> // resource
parseUri(const std::string &uri);

std::tuple<std::string, // protocol
           std::string, // hostname
           uint16_t>    // port number
parseProxyUri(const std::string &uri);

template<typename Container>
int // number of elements in c (as an int, whatever c::size_type is)
len(const Container &c);

template<typename Container>
int // position of e in c or -1
find(const Container &c,
     const typename Container::value_type &e);

int // position of searched in s or -1
find(const std::string &s,
     const std::string &searched);

bool // success
startsWith(const std::string &s,
           const std::string &prefix);

bool // success
endsWith(const std::string &s,
         const std::string &suffix);

std::string // s without leading/trailing separators
strip(const std::string &s);

template<typename First,
         typename ...Args>
std::ostream &
txt(std::ostream &output,
    const char *format,
    First &&first,
    Args &&...args);

template<typename ...Args>
std::string
txt(const char *format,
    Args &&...args);

template<typename ...Args>
int // written bytes
out(const char *format,
    Args &&...args);

template<typename ...Args>
int // written bytes
out(const std::string &format,
    Args &&...args);

template<typename ...Args>
int // written bytes
err(const char *format,
    Args &&...args);

template<typename ...Args>
int // written bytes
err(const std::string &format,
    Args &&...args);

template<typename ...Args>
int // extraction count
extract(std::istream &input,
        Args &&...args);

template<typename ...Args>
int // extraction count
extract(const std::string &input,
        Args &&...args);

template<typename Fnct>
int // split count
split(const std::string &str,
      const char *char_set,
      Fnct fnct,
      bool keep_empty=false,
      int max_split_count=-1);

std::vector<std::string> // split str
split(const std::string &str,
      const char *char_set,
      bool keep_empty=false,
      int max_split_count=-1);

template<typename Fnct>
int // split count
split(const std::string &str,
      Fnct fnct,
      bool keep_empty=false,
      int max_split_count=-1);

std::vector<std::string> // split str
split(const std::string &str,
      bool keep_empty=false,
      int max_split_count=-1);

template<typename Fnct>
int // split count
split_on_word(const std::string &str,
              const std::string &word,
              Fnct fnct,
              bool keep_empty=true,
              int max_split_count=-1);

std::vector<std::string> // split str
split_on_word(const std::string &str,
              const std::string &word,
              bool keep_empty=true,
              int max_split_count=-1);

template<typename Container,
         typename Separator>
std::string // joined strings
join(const Container &strings,
     Separator separator);

template<typename Container>
std::string // joined strings
join(const Container &strings);

template<typename Elem>
void
uninitialised_resize(std::vector<Elem> &v,
                     typename std::vector<Elem>::size_type sz);

template<typename Elem>
void
uninitialised_resize(std::basic_string<Elem> &s,
                     typename std::basic_string<Elem>::size_type sz);

template<typename First,
         typename ...Args>
int // stored bytes
pack_bytes(void *buffer,
           int buffer_capacity,
           const First &first,
           const Args &...args);

template<typename First,
         typename ...Args>
int // read bytes
unpack_bytes(const void *buffer,
             int buffer_size,
             First &first,
             Args &...args);

double // energy consumed by CPU, in Joules
cpuEnergy();

std::string // textual description of current call stack
computeStackTrace();

//----------------------------------------------------------------------------
// Usual system-programming operations
//----------------------------------------------------------------------------

std::string // text description of error code
strerror(int error_code);

std::string // value of environment variable or "" (not set)
getenv(const std::string &name);

void
setenv(const std::string &name,
       const std::string &value={});

double // seconds since 1970/01/01 00:00:00 UTC
gettimeofday();

void
sleep(double seconds);

pid_t // current process identifier
getpid();

pid_t // parent process identifier
getppid();

pid_t // new process identifier (in parent process) or 0 (in child process)
fork();

[[noreturn]]
void
exit(int status);

std::tuple<pid_t, // pid of child process or 0 (if nonblocking)
           int,   // status or -1
           int>   // signal or -1
waitpid(pid_t child,
        bool non_blocking=false);

std::string // text description of signal number
strsignal(int signal);

void
sigaction(int signal,
          std::function<void(int)> action=nullptr);

std::tuple<int, // read file-descriptor
           int> // write file-descriptor
pipe();

std::tuple<SOCKET, // local socket for one end
           SOCKET> // local socket for the other end
socketpair(int type);

int // new file-descriptor 
dup(int fd);

int // new file-descriptor (newFd)
dup2(int old_fd,
     int new_fd);

[[noreturn]]
void
exec(const std::vector<std::string> &command_line);

template<typename T>
T * // shared memory address
mmap_shared(int element_count);

void * // shared memory address
mmap_shared(int byte_count);

template<typename T>
void
munmap(T *address,
       int element_count);

void
munmap(void *address,
       int byte_count);

struct CpuInfo
{
  int pkgId, coreId, cpuId;
};

std::vector<CpuInfo> // detected package and core of each CPU in the system
detectCpuInfo(bool enable_smt=true);

void
bindCurrentThreadToCpu(int cpu);

//----------------------------------------------------------------------------
// Usual operations on files and directories
//----------------------------------------------------------------------------

std::vector<std::string> // directory entries (except . and ..)
listDir(const std::string &path);

bool // path exists and conforms to mode
access(const std::string &path,
       int mode);

int // file size or -1 (no file)
fileSize(const std::string &path);

bool // path exists and is a file
isFile(const std::string &path);

bool // path exists and is a directory
isDir(const std::string &path);

bool // path exists and is a named pipe
isFifo(const std::string &path);

void
mkdir(const std::string &path);

void
rmdir(const std::string &path);

void
mkfifo(const std::string &path);

int // read-only file-descriptor
openR(const std::string &path);

int // write-only file-descriptor
openW(const std::string &path,
      bool append=false,
      bool exclusive=false);

int // read-write file-descriptor
openRW(const std::string &path);

int // new absolute offset
lseek(int fd,
      int offset,
      int origin);

void
close(int fd);

void
unlink(const std::string &path);

int // written bytes
write(int fd,
      const void *content,
      int content_size);

int // written bytes (content_size expected)
writeAll(int fd,
         const void *content,
         int content_size);

int // written bytes (len(msg) expected)
writeAll(int fd,
         const std::string &msg);

int // read bytes or 0 (EOF)
read(int fd,
     void *buffer,
     int buffer_capacity);

int // read bytes (buffer_capacity expected) or 0 (EOF)
readAll(int fd,
        void *buffer,
        int buffer_capacity);

std::string // read text or "" (EOF)
read(int fd,
     int capacity);

std::string // read text or "" (EOF)
readAll(int fd,
        int capacity);

std::string // read text line or "" (EOF)
readLine(int fd);

//----------------------------------------------------------------------------
// Usual operations on IPv4 sockets
//----------------------------------------------------------------------------

uint32_t // IPv4 address of dotted-decimal text or 0
parseIpv4Address(const std::string &address);

std::string // dotted-decimal text of IPv4 address
formatIpv4Address(uint32_t address);

std::string
gethostname();

uint32_t // IPv4 address of host name
gethostbyname(const std::string &hostname);

SOCKET
socket(int domain,
       int type,
       int protocol);

void
close(SOCKET s);

void
shutdown(SOCKET s,
         int how);

void
setReuseAddrOption(SOCKET s,
                   bool on);

void
setTcpNodelayOption(SOCKET s,
                    bool on);

void
setBroadcastOption(SOCKET s,
                   bool on);

void
bind(SOCKET s,
     uint32_t address,
     uint16_t port);

void
bind(SOCKET s,
     uint16_t port);

std::tuple<uint32_t, // IPv4 address
           uint16_t> // port number
getsockname(SOCKET s);

std::tuple<uint32_t, // IPv4 address
           uint16_t> // port number
getpeername(SOCKET s);

void
listen(SOCKET s,
       int backlog=10);

std::tuple<SOCKET,   // dialog socket
           uint32_t, // IPv4 address
           uint16_t> // port number
acceptfrom(SOCKET listen_socket);

SOCKET // dialog socket
accept(SOCKET listen_socket);

void
connect(SOCKET s,
        uint32_t address,
        uint16_t port);

int // sent bytes
send(SOCKET s,
     const void *content,
     int content_size);

int // sent bytes (content_size expected)
sendAll(SOCKET s,
        const void *content,
        int content_size);

int // sent bytes (len(msg) expected)
sendAll(SOCKET s,
        const std::string &msg);

int // sent bytes
sendto(SOCKET s,
       const void *content,
       int content_size,
       uint32_t address,
       uint16_t port);

int // sent bytes
sendto(SOCKET s,
       const std::string &msg,
       uint32_t address,
       uint16_t port);

int // received bytes or 0 (EOF)
recv(SOCKET s,
     void *buffer,
     int buffer_capacity);

int // received bytes (buffer_capacity expected) or 0 (EOF)
recvAll(SOCKET s,
        void *buffer,
        int buffer_capacity);

std::string // received text or "" (EOF)
recv(SOCKET s,
     int capacity);

std::string // received text or "" (EOF)
recvAll(SOCKET s,
        int capacity);

std::string // received text line or "" (EOF)
recvLine(SOCKET s);

std::tuple<int,      // received bytes or 0 (EOF)
           uint32_t, // IPv4 address
           uint16_t> // port number
recvfrom(SOCKET s,
         void *buffer,
         int buffer_capacity);

std::tuple<std::string, // received text or "" (EOF)
           uint32_t,    // IPv4 address
           uint16_t>    // port number
recvfrom(SOCKET s,
         int capacity);

int // number of sockets in ready-state
select(std::vector<SOCKET> &inout_read_set,
       std::vector<SOCKET> &inout_write_set,
       double timeout=-1.0);

int // number of sockets in ready-state
select(std::vector<SOCKET> &inout_read_set,
       double timeout=-1.0);

int16_t // value converted to network byte-order
hton_i16(int16_t host_value);

int16_t // value converted to host byte-order
ntoh_i16(int16_t network_value);

uint16_t // value converted to network byte-order
hton_ui16(uint16_t host_value);

uint16_t // value converted to host byte-order
ntoh_ui16(uint16_t network_value);

int32_t // value converted to network byte-order
hton_i32(int32_t host_value);

int32_t // value converted to host byte-order
ntoh_i32(int32_t network_value);

uint32_t // value converted to network byte-order
hton_ui32(uint32_t host_value);

uint32_t // value converted to host byte-order
ntoh_ui32(uint32_t network_value);

int64_t // value converted to network byte-order
hton_i64(int64_t host_value);

int64_t // value converted to host byte-order
ntoh_i64(int64_t network_value);

uint64_t // value converted to network byte-order
hton_ui64(uint64_t host_value);

uint64_t // value converted to host byte-order
ntoh_ui64(uint64_t network_value);

real32_t // value converted to network byte-order
hton_r32(real32_t host_value);

real32_t // value converted to host byte-order
ntoh_r32(real32_t network_value);

real64_t // value converted to network byte-order
hton_r64(real64_t host_value);

real64_t // value converted to host byte-order
ntoh_r64(real64_t network_value);

//----------------------------------------------------------------------------
// Usual operations on SSL
//----------------------------------------------------------------------------

SSL_CTX *
sslInit(const std::string &ca_cert_path={},
        const std::string &cert_path={},
        const std::string &key_path={});

void
sslDestroy(SSL_CTX *ctx);

SSL *
sslConnect(SOCKET s,
           SSL_CTX *ctx,
           const std::string &hostname={});

SSL *
sslAccept(SOCKET s,
          SSL_CTX *ctx);

bool // some bytes are immediately available
sslPending(SSL *ssl);

void
sslClose(SSL *ssl);

int // sent bytes
send(SSL *ssl,
     const void *content,
     int content_size);

int // sent bytes (content_size expected)
sendAll(SSL *ssl,
        const void *content,
        int content_size);

int // sent bytes (len(msg) expected)
sendAll(SSL *ssl,
        const std::string &msg);

int // received bytes or 0 (EOF)
recv(SSL *ssl,
     void *buffer,
     int buffer_capacity);

int // received bytes (buffer_capacity expected) or 0 (EOF)
recvAll(SSL *ssl,
        void *buffer,
        int buffer_capacity);

std::string // received text or "" (EOF)
recv(SSL *ssl,
     int capacity);

std::string // received text or "" (EOF)
recvAll(SSL *ssl,
        int capacity);

std::string // received text line or "" (EOF)
recvLine(SSL *ssl);

//----------------------------------------------------------------------------
// WebSocket operations (sockets and SSL)
//----------------------------------------------------------------------------

std::string
wsHandshake(std::string key);

enum WsOpcode {WS_NONE=-1, WS_CONT_=0, WS_TXT=1, WS_BIN=2,
               WS_CLOSE=8, WS_PING=9, WS_PONG=10};

void
wsSend(SOCKET s,
       const void *content,
       int content_size,
       WsOpcode opcode=WS_BIN);

void
wsSend(SOCKET s,
       const std::string &msg,
       WsOpcode opcode=WS_TXT);

void
wsSendClose(SOCKET s);

std::tuple<WsOpcode, // opcode
           int>      // received bytes or 0 (EOF)
wsRecv(SOCKET s,
       void *buffer,
       int buffer_capacity);

std::tuple<WsOpcode,    // opcode
           std::string> // received text or "" (EOF)
wsRecv(SOCKET s);

void
wsSend(SSL *ssl,
       const void *content,
       int content_size,
       WsOpcode opcode=WS_BIN);

void
wsSend(SSL *ssl,
       const std::string &msg,
       WsOpcode opcode=WS_TXT);

void
wsSendClose(SSL *ssl);

std::tuple<WsOpcode, // opcode
           int>      // received bytes or 0 (EOF)
wsRecv(SSL *ssl,
       void *buffer,
       int buffer_capacity);

std::tuple<WsOpcode,    // opcode
           std::string> // received text or "" (EOF)
wsRecv(SSL *ssl);

//----------------------------------------------------------------------------
// Inline implementation details (don't look below!)
//----------------------------------------------------------------------------

template<typename Container>
inline
int // number of elements in c (as an int, whatever c::size_type is)
len(const Container &c)
{
  return int(size(c));
}

template<typename Container>
inline
int // position of e in c or -1
find(const Container &c,
     const typename Container::value_type &e)
{
  const auto it=std::find(cbegin(c), cend(c), e);
  return it==cend(c) ? -1 : int(distance(cbegin(c), it));
}

inline
int // position of searched in s or -1
find(const std::string &s,
     const std::string &searched)
{
  const auto pos=s.find(searched);
  return pos==std::string::npos ? -1 : int(pos);
}

inline
bool // success
startsWith(const std::string &s,
           const std::string &prefix)
{
  return (size(s)>=size(prefix))&&
         !s.compare(0, size(prefix), prefix);
}

inline
bool // success
endsWith(const std::string &s,
         const std::string &suffix)
{
  return (size(s)>=size(suffix))&&
         !s.compare(size(s)-size(suffix), size(suffix), suffix);
}

inline
std::string // s without leading/trailing separators
strip(const std::string &s)
{
  const auto notSpace=
    [&](char c)
    {
      return !::isspace(c);
    };
  const auto b=std::find_if(cbegin(s), cend(s), notSpace);
  const auto e=std::find_if(crbegin(s), crend(s), notSpace).base();
  return std::string{b, std::max(b, e)};
}

inline
std::ostream &
txt(std::ostream &output,
    const char *format)
{
  return output << format;
}

template<typename First,
         typename ...Args>
inline
std::ostream &
txt(std::ostream &output,
    const char *format,
    First &&first,
    Args &&...args)
{
  while(*format)
  {
    if(*format=='%')
    {
      return txt(output << std::forward<First>(first),
                 ++format, std::forward<Args>(args)...);
    }
    output << *format++;
  }
  return output;
}

template<typename ...Args>
inline
std::string
txt(const char *format,
    Args &&...args)
{
  auto output=std::ostringstream{};
  txt(output, format, std::forward<Args>(args)...);
  return output.str();
}

template<typename ...Args>
inline
int // written bytes
out(const char *format,
    Args &&...args)
{
  return writeAll(STDOUT_FILENO, txt(format, std::forward<Args>(args)...));
}

template<typename ...Args>
inline
int // written bytes
out(const std::string &format,
    Args &&...args)
{
  return out(data(format), std::forward<Args>(args)...);
}

template<typename ...Args>
inline
int // written bytes
err(const char *format,
    Args &&...args)
{
  return writeAll(STDERR_FILENO, txt(format, std::forward<Args>(args)...));
}

template<typename ...Args>
inline
int // written bytes
err(const std::string &format,
    Args &&...args)
{
  return err(data(format), std::forward<Args>(args)...);
}

inline
void
extract_arg_(std::istream &input,
             int &count,
             const char &literalChar)
{
  if(!input.fail())
  {
    auto buffer=char{};
    input >> buffer;
    if(buffer!=literalChar)
    {
      input.setstate(input.rdstate()|std::ios::failbit);
    }
  }
  if(!input.fail())
  {
    ++count;
  }
}

template<int N>
void
extract_arg_(std::istream &input,
             int &count,
             const char(&literalString)[N])
{
  if(!input.fail())
  {
    auto buffer=std::array<char, N-1>{};
    input >> buffer[0]; // only first char to skip leading separators
    if(N>2)
    {
      input.read(&buffer[1], N-2); // remaining of literal string
    }
    if(::strncmp(&buffer[0], literalString, N-1))
    {
      input.setstate(input.rdstate()|std::ios::failbit);
    }
  }
  if(!input.fail())
  {
    ++count;
  }
}

template<typename T>
inline
void
extract_arg_(std::istream &input,
             int &count,
             T &arg)
{
  if(!input.fail())
  {
    input >> arg;
  }
  if(!input.fail())
  {
    ++count;
  }
}

struct variadic_pass_
{
  template<typename ...T>
  variadic_pass_(T...)
  {
  }
};

template<typename ...Args>
inline
int // extraction count
extract(std::istream &input,
        Args &&...args)
{
  auto count=0;
  variadic_pass_{(extract_arg_(input, count,
                               std::forward<Args>(args)), 1)...};
  return count;
}

template<typename ...Args>
inline
int // extraction count
extract(const std::string &input,
        Args &&...args)
{
  auto inputStream=std::istringstream{input};
  return extract(inputStream, std::forward<Args>(args)...);
}

template<typename Fnct>
inline
int // split count
split(const std::string &str,
      const char *char_set,
      Fnct fnct,
      bool keep_empty,
      int max_split_count)
{
  using size_type = std::string::size_type;
  auto last_pos=size_type{};
  auto split_count=0;
  auto stop=false;
  do
  {
    auto pos=split_count==max_split_count
             ? std::string::npos
             : str.find_first_of(char_set, last_pos);
    if(pos==std::string::npos)
    {
      pos=size(str);
      stop=true;
    }
    if(keep_empty||(pos!=last_pos))
    {
      ++split_count;
      fnct(int(last_pos), int(pos));
    }
    last_pos=pos+1;
  } while(!stop);
  return split_count;
}

inline
std::vector<std::string> // split str
split(const std::string &str,
      const char *char_set,
      bool keep_empty,
      int max_split_count)
{
  auto result=std::vector<std::string>{};
  split(str, char_set,
    [&](const auto &b, const auto &e)
    {
      result.emplace_back(data(str)+b, e-b);
    },
    keep_empty, max_split_count);
  return result;
}

template<typename Fnct>
inline
int // split count
split(const std::string &str,
      Fnct fnct,
      bool keep_empty,
      int max_split_count)
{
  return split(str, " \t\r\n", fnct, keep_empty, max_split_count);
}

inline
std::vector<std::string> // split str
split(const std::string &str,
      bool keep_empty,
      int max_split_count)
{
  auto result=std::vector<std::string>{};
  split(str,
    [&](const auto &b, const auto &e)
    {
      result.emplace_back(data(str)+b, e-b);
    },
    keep_empty, max_split_count);
  return result;
}

template<typename Fnct>
inline
int // split count
split_on_word(const std::string &str,
              const std::string &word,
              Fnct fnct,
              bool keep_empty,
              int max_split_count)
{
  using size_type = std::string::size_type;
  const auto skip=size(word);
  auto last_pos=size_type{};
  auto split_count=0;
  auto stop=false;
  do
  {
    auto pos=split_count==max_split_count
             ? std::string::npos
             : str.find(word, last_pos);
    if(pos==std::string::npos)
    {
      pos=size(str);
      stop=true;
    }
    if(keep_empty||(pos!=last_pos))
    {
      ++split_count;
      fnct(int(last_pos), int(pos));
    }
    last_pos=pos+skip;
  } while(!stop);
  return split_count;
}

inline
std::vector<std::string> // split str
split_on_word(const std::string &str,
              const std::string &word,
              bool keep_empty,
              int max_split_count)
{
  auto result=std::vector<std::string>{};
  split_on_word(str, word,
    [&](const auto &b, const auto &e)
    {
      result.emplace_back(data(str)+b, e-b);
    },
    keep_empty, max_split_count);
  return result;
}

template<typename T,
         typename=void>
struct has_std_to_string
  : std::false_type {};
template<typename T>
struct has_std_to_string<T,
  std::void_t<decltype(std::to_string(std::declval<T>()))>>
  : std::true_type {};
template<typename T,
         typename=void>
struct has_ADL_to_string
  : std::false_type {};
template<typename T>
struct has_ADL_to_string<T,
  std::void_t<decltype(to_string(std::declval<T>()))>>
  : std::true_type {};
template<typename T>
inline constexpr bool has_to_string_v =
  std::disjunction_v<has_std_to_string<T>,
                     has_ADL_to_string<T>>;
template<typename T>
inline constexpr bool always_false_v = std::false_type::value;

template<typename Container,
         typename Separator>
inline
std::string // joined strings
join(const Container &strings,
     Separator separator)
{
  auto result=std::string{};
  if constexpr(std::is_invocable_r_v<void, Separator, std::string &>)
  {
    auto first=true;
    for(const auto &s: strings)
    {
      if(!first)
      {
        separator(result);
      }
      first=false;
      result+=s;
    }
  }
  else if constexpr(std::is_same_v<Separator, char>)
  {
    result=join(strings,
      [&](auto &result)
      {
        result+=separator;
      });
  }
  else if constexpr(std::is_convertible_v<Separator, const char *>)
  {
    const char *sep=separator;
    result=sep&&sep[0] ?
      join(strings,
        [&](auto &result)
        {
          result+=sep;
        }) :
      join(strings,
        [&](auto &)
        {
        });
  }
  else if constexpr(std::is_convertible_v<Separator, const std::string &>)
  {
    const std::string &sep=separator;
    result=join(strings, data(sep));
  }
  else if constexpr(has_to_string_v<Separator>)
  {
    using std::to_string;
    result=join(strings, to_string(separator));
  }
  else
  {
    static_assert(always_false_v<Separator>, "bad separator");
    (void)separator;
  }
  return result;
}

template<typename Container>
inline
std::string // joined strings
join(const Container &strings)
{
  return join(strings, nullptr);
}

template<typename Elem>
inline
void
uninitialised_resize(std::vector<Elem> &v,
                     typename std::vector<Elem>::size_type sz)
{
  static_assert(std::is_pod_v<Elem>,
                "plain-old-data elements expected");
  struct PodElem
  {
    Elem uninitialised;
    PodElem() { }; // disable zero-initialisation
  };
  v.reserve(sz);
  auto &raw=reinterpret_cast<std::vector<PodElem> &>(v);
  raw.resize(sz);
}

template<typename Elem>
inline
void
uninitialised_resize(std::basic_string<Elem> &s,
                     typename std::basic_string<Elem>::size_type sz)
{
  static_assert(std::is_pod_v<Elem>,
                "plain-old-data elements expected");
#if defined __APPLE__ && defined __clang__
  // FIXME: apple's standard library complains
  s.resize(sz);
#else
  struct PodElem
  {
    Elem uninitialised;
    PodElem() { }; // disable zero-initialisation
  };
  s.reserve(sz);
  auto &raw=reinterpret_cast<std::basic_string<PodElem> &>(s);
  raw.resize(sz);
#endif
}

template<typename First,
         typename ...Args>
inline
int // stored bytes
pack_bytes(void *buffer,
           int buffer_capacity,
           const First &first,
           const Args &...args)
{
  constexpr auto sz=int(sizeof(First));
  if(buffer_capacity<sz)
  {
    return 0;
  }
  std::memcpy(buffer, &first, sz);
  if constexpr(sizeof...(Args)>0)
  {
    auto *addr=static_cast<std::uint8_t *>(buffer);
    return sz+pack_bytes(addr+sz, buffer_capacity-sz, args...);
  }
  else
  {
    return sz;
  }
}

template<typename First,
         typename ...Args>
inline
int // read bytes
unpack_bytes(const void *buffer,
             int buffer_size,
             First &first,
             Args &...args)
{
  constexpr auto sz=int(sizeof(First));
  if(buffer_size<sz)
  {
    return 0;
  }
  std::memcpy(&first, buffer, sz);
  if constexpr(sizeof...(Args)>0)
  {
    const auto *addr=static_cast<const std::uint8_t *>(buffer);
    return sz+unpack_bytes(addr+sz, buffer_size-sz, args...);
  }
  else
  {
    return sz;
  }
}

template<typename T>
inline
T * // shared memory address
mmap_shared(int element_count)
{
  return static_cast<T *>(mmap_shared(int(element_count*sizeof(T))));
}

template<typename T>
inline
void
munmap(T *address,
       int element_count)
{
  munmap(static_cast<void *>(address), int(element_count*sizeof(T)));
}

} // namespace crs

#endif // CRSUTILS_HPP

//----------------------------------------------------------------------------
