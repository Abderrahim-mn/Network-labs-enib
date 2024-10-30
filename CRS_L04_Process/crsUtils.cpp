//----------------------------------------------------------------------------

#include "crsUtils.hpp"
#include <regex>

#if defined _WIN32
# include <Dbghelp.h>
#else
# include <execinfo.h>
#endif

#if defined __APPLE__
# include <libproc.h>
#endif

namespace crs {

#define THROW_SYSTEM_FAILURE(error_code)                             \
        do                                                           \
        {                                                            \
          const auto msg=std::string{strerror((error_code))};         \
          throw std::runtime_error{txt("%:%:%() failure --- %\n%",   \
                                       __FILE__, __LINE__, __func__, \
                                       msg, computeStackTrace())};   \
        } while(0)

#define THROW_NOT_AVAILABLE(msg)                                     \
        do                                                           \
        {                                                            \
          throw std::runtime_error{txt("%:%:%() not available%\n%",  \
                                       __FILE__, __LINE__, __func__, \
                                       msg, computeStackTrace())};   \
        } while(0)

#define SSL_ERROR_MSG(fnct) \
        (txt("%: %", #fnct, ::ERR_error_string(::ERR_get_error(), nullptr)))

#define THROW_SSL_ERROR(fnct)                                        \
        do                                                           \
        {                                                            \
          throw std::runtime_error{txt("%:%:%() %\n%",               \
                                       __FILE__, __LINE__, __func__, \
                                       SSL_ERROR_MSG(fnct),          \
                                       computeStackTrace())};        \
        } while(0)

#if !defined _WIN32
  static std::map<int, std::function<void(int)>> sigaction_data_{};
  static std::atomic_flag sigaction_lock_=ATOMIC_FLAG_INIT;
#endif

[[maybe_unused]] static bool static_initialisation_=([]()
  {
#if defined _WIN32
    ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
    ::_setmode(STDIN_FILENO, _O_BINARY);
    ::_setmode(STDOUT_FILENO, _O_BINARY);
    ::_setmode(STDERR_FILENO, _O_BINARY);
    auto wsa_data=WSADATA{};
    ::WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
#if defined _MSC_VER // MSVC++ does not show anything
    std::set_terminate([]()
      {
        err("Uncaught exception!\n");
        // FIXME: fails in MSVC++
        // try { throw; }
        // catch(const std::exception &e) { err("%\n",e.what()); }
        std::abort();
      });
#endif
#if !defined _WIN32
    sigaction(SIGPIPE, [](int){}); // avoid spurious termination on IO failure
#endif
    return true;
  })();

#if defined _WIN32
  // allow generic read()/write() functions to use Windows sockets
  int read(SOCKET s, void *b, int c) { return recv(s, b, c); }
  int write(SOCKET s, const void *c, int sz) { return send(s, c, sz); }
#endif
// allow generic read()/write() functions to use SSL connections
int read(SSL *s, void *b, int c) { return recv(s, b, c); }
int write(SSL *s, const void *c, int sz) { return send(s, c, sz); }

template<typename Dst>
inline
int // written bytes (content_size expected)
writeAll_(Dst dst,
          const void *content,
          int content_size)
{
  const auto *ptr=reinterpret_cast<const char *>(content);
  auto remaining=content_size;
  while(remaining)
  {
    const auto r=write(dst, ptr, remaining);
    if(!r)
    {
      break; // EOF
    }
    ptr+=r;
    remaining-=r;
  }
  return content_size-remaining;
}

template<typename Dst>
inline
int // written bytes (len(msg) expected)
writeAll_(Dst dst,
          const std::string &msg)
{
  return writeAll_(dst, data(msg), len(msg));
}

template<typename Src>
inline
int // read bytes (buffer_capacity expected) or 0 (EOF)
readAll_(Src src,
         void *buffer,
         int buffer_capacity)
{
  auto *ptr=reinterpret_cast<char *>(buffer);
  auto remaining=buffer_capacity;
  while(remaining)
  {
    const auto r=read(src, ptr, remaining);
    if(!r)
    {
      break; // EOF
    }
    ptr+=r;
    remaining-=r;
  }
  return buffer_capacity-remaining;
}

template<typename Src>
inline
std::string // read text or "" (EOF)
read_(Src src,
      int capacity)
{
  auto result=std::string{};
  uninitialised_resize(result, capacity);
  result.resize(read(src, data(result), capacity));
  return result;
}

template<typename Src>
inline
std::string // read text or "" (EOF)
readAll_(Src src,
         int capacity)
{
  auto result=std::string{};
  uninitialised_resize(result, capacity);
  result.resize(readAll_(src, data(result), capacity));
  return result;
}

template<typename Src>
inline
std::string // read text line or "" (EOF)
readLine_(Src src)
{
  auto result=std::string{};
  auto c=char{};
  while(read(src, &c, 1)==1)
  {
    result+=c;
    if(c=='\n')
    {
      break; // end of line
    }
  }
  return result;
}

//----------------------------------------------------------------------------

std::tuple<std::string, // protocol
           std::string, // hostname
           uint16_t,    // port number
           std::string, // resource
           std::string> // params
parseUriWithParams(const std::string &uri)
{
  auto protocol=std::string{};
  auto hostname=std::string{};
  auto port_number=uint16_t{};
  auto resource=std::string{};
  auto params=std::string{};
  const auto re=std::regex{
    //1                2       3 4         5      6   7
    "^([a-zA-Z0-9]+)://([^:/]+)(:([0-9]+))?([^?]*)([?](.*))?$",
    std::regex::extended};
  auto m=std::smatch{};
  if(std::regex_search(uri, m, re))
  {
    protocol=m.str(1);
    port_number=protocol=="http" ? 80 :
                protocol=="https" ? 443 : 0;
    if((m[3].matched&&(extract(m.str(4), port_number)!=1))||!port_number)
    {
      protocol.clear();
      port_number=0;
    }
    else
    {
      hostname=m.str(2);
      resource=m.str(5);
      if(empty(resource))
      {
        resource="/";
      }
      if(m[6].matched)
      {
        params=m.str(7);
      }
    }
  }
  return {std::move(protocol),
          std::move(hostname),
          std::move(port_number),
          std::move(resource),
          std::move(params)};
}

std::tuple<std::string, // protocol
           std::string, // hostname
           uint16_t,    // port number
           std::string> // resource
parseUri(const std::string &uri)
{
  auto [protocol, hostname, port_number,
        resource, params]=parseUriWithParams(uri);
  if(!empty(params))
  {
    resource+="?"+params;
  }
  return {std::move(protocol),
          std::move(hostname),
          std::move(port_number),
          std::move(resource)};
}

std::tuple<std::string, // protocol
           std::string, // hostname
           uint16_t>    // port number
parseProxyUri(const std::string &uri)
{
  auto [protocol, hostname, port_number, resource]=parseUri(uri);
  if((resource!="/")&&!empty(resource))
  {
    protocol.clear();
    hostname.clear();
    port_number=0;
  }
  return {std::move(protocol),
          std::move(hostname),
          std::move(port_number)};
}

double // energy consumed by CPU, in Joules
cpuEnergy()
{
  struct Info
  {
    std::string file;
    int64_t range;
    int64_t init;
  };
  static auto infos=std::vector<Info>{};
  static auto available=true;
  if(!available)
  {
    return 0.0;
  }
  const auto read_text=[&](const auto &path)
  {
    const auto fd=openR(path);
    const auto text=readAll(fd, 0x100);
    close(fd);
    return text;
  };
  const auto read_value=[&](const auto &path)
  {
    auto value=int64_t{-1};
    crs::extract(read_text(path), value);
    return value;
  };
  if(empty(infos)) // first call
  {
    constexpr auto rapl_dir="/sys/class/powercap/intel-rapl/";
    if(isDir(rapl_dir))
    {
      auto unreadable=std::vector<std::string>{};
      for(const auto &e: listDir(rapl_dir))
      {
        if(!startsWith(e, "intel-rapl:"))
        {
          continue;
        }
        const auto name_file=rapl_dir+e+"/name";
        const auto max_range_file=rapl_dir+e+"/max_energy_range_uj";
        const auto energy_file=rapl_dir+e+"/energy_uj";
        if(!isFile(name_file)||!isFile(max_range_file)||!isFile(energy_file))
        {
          continue;
        }
        auto name=std::string{};
        try
        {
          name=read_text(name_file);
        }
        catch(...)
        {
          unreadable.emplace_back(name_file);
        }
        const auto is_package=startsWith(name, "package");
        if(!empty(name)&&!is_package)
        {
          continue;
        }
        auto max_range_value=int64_t{-1};
        try
        {
          max_range_value=read_value(max_range_file);
        }
        catch(...)
        {
          unreadable.emplace_back(max_range_file);
        }
        auto energy_value=int64_t{-1};
        try
        {
          energy_value=read_value(energy_file);
        }
        catch(...)
        {
          unreadable.emplace_back(energy_file);
        }
        if(is_package&&(max_range_value>0)&&(energy_value>=0))
        {
          infos.emplace_back(Info{energy_file, max_range_value, energy_value});
        }
      }
      if(!empty(unreadable))
      {
        crs::err("warning: access-rights required for energy estimation,\n");
        crs::err("         these commands (as root) might help:\n");
        for(const auto &u: unreadable)
        {
          crs::err("--> chmod +r '%'\n", u);
        }
        infos.clear();
      }
    }
    if(empty(infos))
    {
      available=false;
      return 0.0;
    }
  }
  auto energy=int64_t{0};
  for(const auto &info: infos)
  {
    const auto value=read_value(info.file);
    energy+=(value+info.range-info.init)%info.range;
  }
  return double(energy)*1e-6; // micro-joules to joules
}

std::string // textual description of current call stack
computeStackTrace()
{
  // nb: don't use crs:: system calls in order to prevent from
  //     recursive callstack retrieval on errors 
  auto result=std::string{};
  constexpr auto max_stack_size=0x100;
  auto stack=std::array<void *, max_stack_size>{};
#if defined _WIN32
  const auto h_process=::GetCurrentProcess();
  if(::SymInitialize(h_process, nullptr, TRUE))
  {
    const auto stack_depth=::CaptureStackBackTrace(0, max_stack_size,
                                                   data(stack), nullptr);
    constexpr auto buffer_size=sizeof(SYMBOL_INFO)+MAX_SYM_NAME*sizeof(TCHAR);
    auto buffer=std::array<char, buffer_size>{};
    auto symbol=reinterpret_cast<SYMBOL_INFO *>(data(buffer));
    symbol->SizeOfStruct=sizeof(SYMBOL_INFO);
    symbol->MaxNameLen=MAX_SYM_NAME;
    IMAGEHLP_LINE64 line={};
    line.SizeOfStruct=sizeof(IMAGEHLP_LINE64);
    for(auto top_level=0, level=0; level<stack_depth; ++level)
    {
      // FIXME: this works only with PDB files.
      //        Visual-Studio /debug linker switch produces such files
      //        unfortunately, mingw-w64 does not.
      auto disp=DWORD{};
      if(::SymFromAddr(h_process, DWORD64(stack[level]), nullptr, symbol)&&
         ::SymGetLineFromAddr64(h_process,
                                DWORD64(stack[level]), &disp, &line))
      {
        if(!top_level&&std::strstr(symbol->Name, __func__))
        {
          top_level=level+1;
        }
        if(top_level&&(level>=top_level))
        {
          result+='[';
          result+=std::to_string(level-top_level);
          result+="] ";
          result+=symbol->Name;
          result+=" at ";
          result+=line.FileName;
          result+=':';
          result+=std::to_string(line.LineNumber);
          result+='\n';
        }
        if(!std::strcmp(symbol->Name, "main"))
        {
          break;
        }
      }
    }
  }
#else
  const auto stack_depth=::backtrace(data(stack), max_stack_size);
  // FIXME: a solution could hardly be uglier than this one!
  //        (retrieving information from an external program)
  const auto use_command=
    [&, &this_fnct=__func__](auto &str_args)
    {
      for(auto i=0; i<stack_depth; ++i)
      {
        auto oss=std::ostringstream{};
        oss << stack[i];
        str_args.emplace_back(oss.str());
      }
      auto args=std::vector<const char *>{size(str_args)+1};
      std::transform(begin(str_args), end(str_args), begin(args),
        [&](auto &s)
        {
          return data(s);
        });
      auto fifo=std::array<int, 2>{-1, -1};
      const auto unused=::pipe(data(fifo));
      (void)unused; // some compilers complain about ignored return value
      const auto child=::fork();
      if(child==0)
      {
        ::close(fifo[0]);
        ::close(STDIN_FILENO);
        // ::close(STDERR_FILENO);
        ::dup2(fifo[1], STDOUT_FILENO);
        ::close(fifo[1]);
        ::execvp(args[0], const_cast<char **>(data(args)));
        ::exit(1);
      }
      ::close(fifo[1]);
      auto current_line=std::string{};
      auto lines=std::vector<std::string>{};
      for(;;)
      {
        auto c=char{};
        auto r=int{};
        RESTART_SYSCALL(r, int(::read(fifo[0], &c, 1)));
        if(r<1)
        {
          break;
        }
        if(c!='\n')
        {
          current_line+=c;
        }
        else if(!empty(current_line))
        {
          lines.emplace_back(std::move(current_line));
          current_line.clear();
        }
      }
      ::close(fifo[0]);
      ::waitpid(child, nullptr, 0);
      auto this_level=-1, main_level=-1;
      for(const auto &line: lines)
      {
        if((this_level==-1)&&std::strstr(data(line), this_fnct))
        {
          this_level=int(&line-&lines.front());
        }
        else if((main_level==-1)&&
                 (!std::strncmp(data(line), "main ", 5)||
                  !std::strncmp(data(line), "main(", 5)))
        {
          main_level=int(&line-&lines.front());
        }
      }
      if((main_level!=-1)&&(this_level<main_level))
      {
        lines.erase(cbegin(lines)+main_level+1, cend(lines));
      }
      if(this_level!=-1)
      {
        lines.erase(cbegin(lines), cbegin(lines)+this_level+1);
      }
      auto result=std::string{};
      for(const auto &line: lines)
      {
        result+='[';
        result+=std::to_string(&line-&lines.front());
        result+="] ";
        result+=line;
        result+='\n';
      }
      return result;
    };
#if defined __APPLE__
  auto args=std::vector<std::string>{"atos",
                                     "-p", std::to_string(::getpid())};
  result=use_command(args);
#else
  auto args=std::vector<std::string>{"eu-addr2line",
                                     "-f", "-C", "-s", "--pretty-print",
                                     "-p", std::to_string(::getpid())};
  result=use_command(args);
#endif
  if(empty(result)) // fallback
  {
    // FIXME: this cannot work since stack addresses need to be converted
    //        to executable addresses as atos and eu-addr2line do
#if 0
    // see https://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
    auto executable=std::string{};
    char buffer[PATH_MAX]={'\0'};
#if defined _WIN32
    if(::GetModuleFileName(nullptr, buffer, sizeof(buffer)))
#elif defined __APPLE__
    if(::proc_pidpath(::getpid(), buffer, sizeof(buffer))>0)
#else
    if(::readlink("/proc/self/exe", buffer, sizeof(buffer))>0)
#endif
    {
      executable=buffer;
    }
    auto args=std::vector<std::string>{"addr2line",
                                       "-f", "-C", "-s", "--pretty-print",
                                       "-e", executable};
    result=use_command(args);
#endif
  }
#endif
  if(empty(result))
  {
    result+="!!! cannot retrieve stack-trace details\n";
#if defined _MSC_VER
    result+="!!! link with the '/debug' switch to generate a .pdb file\n";
#elif defined _WIN32
    result+="!!! ('gcc' does not generate .pdb files, 'cv2pdb' may help)";
#elif defined __APPLE__
    result+="!!! ('atos' command gave nothing)";
#else
    result+="!!! ('eu-addr2line' command gave nothing)";
#endif
  }
  return result;
}

//----------------------------------------------------------------------------

std::string // text description of error code
strerror(int error_code)
{
  auto result=std::string{};
#if defined _WIN32
  auto *err=(char *)nullptr;
  if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                     (LPCVOID)0, error_code,
                     0, // MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                     (LPTSTR)&err, 4096, nullptr))
  {
    for(auto i=::strlen(err); i--; )
    {
      if((err[i]!='\r')&&(err[i]!='\n'))
      {
        break;
      }
      err[i]='\0';
    }
    result+=err;
    ::LocalFree((HLOCAL)err);
  }
  else
  {
    result="???";
  }
#else
  result=::strerror(error_code);
#endif
  result+=txt(" (%)", error_code);
  return result;
}

std::string // value of environment variable or "" (not set)
getenv(const std::string &name)
{
  const auto *v=::getenv(data(name));
  return std::string{v ? v : ""};
}

void
setenv(const std::string &name,
       const std::string &value)
{
#if defined _WIN32
  (void)name; // avoid ``unused parameter'' warning
  (void)value;
  THROW_NOT_AVAILABLE(" on Windows");
#else
  auto r=int{};
  if(empty(value))
  {
    RESTART_SYSCALL(r, ::unsetenv(data(name)));
  }
  else
  {
    RESTART_SYSCALL(r, ::setenv(data(name), data(value), 1));
  }
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#endif
}

double // seconds since 1970/01/01 00:00:00 UTC
gettimeofday()
{
#if defined _MSC_VER
  auto ft=FILETIME{};
  ::GetSystemTimeAsFileTime(&ft);
  const auto t=(int64_t(ft.dwHighDateTime)<<32)+int64_t(ft.dwLowDateTime);
  return 1e-6*((t/10LL)-11644473600000000LL); // 100ns to 1us, 1601 to 1970
#else
  struct timeval tv;
  auto r=int{};
  RESTART_SYSCALL(r, ::gettimeofday(&tv, nullptr));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return double(tv.tv_sec)+1e-6*double(tv.tv_usec);
#endif
}

void
sleep(double seconds)
{
#if defined _MSC_VER
  ::Sleep(DWORD(1e3*seconds));
#else
  struct timespec ts;
  ts.tv_sec=time_t(seconds);
  ts.tv_nsec=int(1e9*(seconds-double(ts.tv_sec)));
  auto r=int{};
  RESTART_SYSCALL(r, ::nanosleep(&ts,&ts));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#endif
}

pid_t // current process identifier
getpid()
{
#if defined _WIN32
  THROW_NOT_AVAILABLE(" on Windows");
  return -1; // never reached
#else
  auto p=pid_t{};
  RESTART_SYSCALL(p, ::getpid());
  if(p==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return p;
#endif
}

pid_t // parent process identifier
getppid()
{
#if defined _WIN32
  THROW_NOT_AVAILABLE(" on Windows");
  return -1; // never reached
#else
  auto p=pid_t{};
  RESTART_SYSCALL(p, ::getppid());
  if(p==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return p;
#endif
}

pid_t // new process identifier (in parent process) or 0 (in child process)
fork()
{
#if defined _WIN32
  THROW_NOT_AVAILABLE(" on Windows");
  return -1; // never reached
#else
  auto p=pid_t{};
  RESTART_SYSCALL(p, ::fork());
  if(p==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return p;
#endif
}

[[noreturn]]
void
exit(int status)
{
  if((status<0)||(status>255))
  {
    throw std::runtime_error{txt("%:%:%() status % not in [0;255]\n%",
                                 __FILE__, __LINE__, __func__,
                                 status, computeStackTrace())};
  }
  ::exit(status);
}

std::tuple<pid_t, // pid of child process or 0 (if non_blocking)
           int,   // status or -1
           int>   // signal or -1
waitpid(pid_t child,
        bool non_blocking)
{
  auto p=pid_t{};
  auto status=-1;
  auto signal=-1;
#if defined _WIN32
  (void)child; // avoid ``unused parameter'' warning
  (void)non_blocking;
  THROW_NOT_AVAILABLE(" on Windows");
#else
  auto wstatus=int{};
  RESTART_SYSCALL(p, ::waitpid(child, &wstatus, non_blocking ? WNOHANG : 0));
  if((p==-1)&&!non_blocking)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  if(p>0)
  {
    if(WIFEXITED(wstatus))
    {
      status=WEXITSTATUS(wstatus);
    }
    if(WIFSIGNALED(wstatus))
    {
      signal=WTERMSIG(wstatus);
    }
  }
  else
  {
    p=0;
  }
#endif
  return {std::move(p),
          std::move(status),
          std::move(signal)};
}

std::string
strsignal(int signal)
{
#if defined _WIN32
  (void)signal; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE(" on Windows");
  return {}; // never reached
#else
  const auto *s=signal==-1 ? "none" : ::strsignal(signal);
  return std::string{s ? s : "unknown"};
#endif
}

#if !defined _WIN32
static
void
sigaction_handler_(int signal)
{
  auto action=std::function<void(int)>{};
  while(sigaction_lock_.test_and_set(std::memory_order_acquire)) { }
  if(auto it=sigaction_data_.find(signal); it!=end(sigaction_data_))
  {
    action=it->second;
  }
  sigaction_lock_.clear(std::memory_order_release);
  if(action)
  {
    action(signal);
  }
}
#endif

void
sigaction(int signal,
          std::function<void(int)> action)
{
#if defined _WIN32
  (void)signal; // avoid ``unused parameter'' warning
  (void)action;
  THROW_NOT_AVAILABLE(" on Windows");
#else
  struct sigaction sa;
  ::memset(&sa, 0, sizeof(sa));
  sa.sa_handler=SIG_DFL;
  if(action)
  {
    while(sigaction_lock_.test_and_set(std::memory_order_acquire)) { }
    sigaction_data_[signal]=action;
    sigaction_lock_.clear(std::memory_order_release);
    sa.sa_handler=sigaction_handler_;
  }
  auto r=int{};
  RESTART_SYSCALL(r, ::sigaction(signal, &sa, nullptr));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#endif
}

std::tuple<int, // read file-descriptor
           int> // write file-descriptor
pipe()
{
  auto fd=std::array<int, 2>{-1, -1};
#if defined _WIN32
  THROW_NOT_AVAILABLE(" on Windows");
#else
  auto r=int{};
  RESTART_SYSCALL(r, ::pipe(data(fd)));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#endif
  return {std::move(fd[0]),
          std::move(fd[1])};
}

std::tuple<SOCKET, // local socket for one end
           SOCKET> // local socket for the other end
socketpair(int type)
{
  auto fd=std::array<SOCKET, 2>{INVALID_SOCKET, INVALID_SOCKET};
#if defined _WIN32
  (void)type; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE(" on Windows");
#else
  auto r=int{};
  RESTART_SYSCALL(r, ::socketpair(PF_LOCAL, type, 0, data(fd)));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#endif
  return {std::move(fd[0]),
          std::move(fd[1])};
}

int // new file-descriptor
dup(int fd)
{
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, ::_dup(fd));
#else
  RESTART_SYSCALL(r, ::dup(fd));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return r;
}

int // new file-descriptor (new_fd)
dup2(int old_fd,
     int new_fd)
{
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, ::_dup2(old_fd, new_fd));
#else
  RESTART_SYSCALL(r, ::dup2(old_fd, new_fd));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return r;
}

[[noreturn]]
void
exec(const std::vector<std::string> &command_line)
{
  auto args=std::vector<const char *>{};
  for(const auto &elem: command_line)
  {
    args.emplace_back(data(elem));
  }
  args.emplace_back(nullptr);
#if defined _MSC_VER
  ::_execvp(args[0], const_cast<char **>(data(args)));
#else
  ::execvp(args[0], const_cast<char **>(data(args)));
#endif
  THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
}

void * // shared memory address
mmap_shared(int byte_count)
{
#if defined _WIN32
  (void)byte_count; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE(" on Windows");
  return nullptr; // never reached
#else
  auto *p=::mmap(nullptr, byte_count, PROT_READ|PROT_WRITE,
                 MAP_ANONYMOUS|MAP_SHARED, -1, 0);
  if(p==MAP_FAILED)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return p;
#endif
}

void 
munmap(void *address,
       int byte_count)
{
#if defined _WIN32
  (void)address; // avoid ``unused parameter'' warning
  (void)byte_count;
  THROW_NOT_AVAILABLE(" on Windows");
#else
  auto r=int{};
  RESTART_SYSCALL(r, ::munmap(address, byte_count));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#endif
}

std::vector<CpuInfo> // detected package and core of each CPU in the system
detectCpuInfo(bool enable_smt)
{
  auto result=std::vector<CpuInfo>{};
  const auto cpu_count=int(std::thread::hardware_concurrency());
#if defined __linux__
  for(auto cpu_id=0; cpu_id<cpu_count; ++cpu_id)
  {
    const auto pfx=txt("/sys/devices/system/cpu/cpu%", cpu_id);
    auto core_id=-1, pkg_id=-1;
    std::ifstream{txt("%/topology/core_id", pfx)} >> core_id;
    std::ifstream{txt("%/topology/physical_package_id", pfx)} >> pkg_id;
    if((core_id<0)||(pkg_id<0))
    {
      continue;
    }
    if(enable_smt||
       std::none_of(cbegin(result), cend(result),
         [&](const auto &c)
         {
           return (c.pkgId==pkg_id)&&(c.coreId==core_id);
         }))
    {
      result.emplace_back(CpuInfo{pkg_id, core_id, cpu_id});
    }
  }
#else
  (void)enable_smt; // avoid ``unused parameter'' warning
#endif
  if(empty(result))
  {
    // nothing found, assume one package and one core per CPU
    for(auto cpu_id=0; cpu_id<cpu_count; ++cpu_id)
    {
      result.emplace_back(CpuInfo{0, cpu_id, cpu_id});
    }
  }
  return result;
}

void
bindCurrentThreadToCpu(int cpu_id)
{
#if defined __linux__
  auto cpuset=cpu_set_t{};
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id, &cpuset);
  const auto r=::pthread_setaffinity_np(::pthread_self(),
                                        sizeof(cpuset), &cpuset);
  if(r!=0)
  {
    THROW_SYSTEM_FAILURE(r);
  }
#elif defined _WIN32
  const auto r=::SetThreadAffinityMask(::GetCurrentThread(),
                                       DWORD_PTR(1ULL<<cpu_id));
  if(!r)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#else
  (void)cpu_id; // avoid ``unused parameter'' warning
#endif
}

//----------------------------------------------------------------------------

std::vector<std::string> // directory entries (except . and ..)
listDir(const std::string &path)
{
  auto result=std::vector<std::string>{};
#if defined _MSC_VER
  auto pattern=path;
  if(empty(pattern))
  {
    pattern='.';
  }
  if(pattern.back()!='\\')
  {
    pattern+='\\';
  }
  pattern+="*.*";
  auto find_data=WIN32_FIND_DATA{};
  auto find_handle=::FindFirstFile(data(pattern), &find_data);
  if(find_handle==INVALID_HANDLE_VALUE)
  {
    const auto err=SYSCALL_ERRNO;
    if(err!=ERROR_FILE_NOT_FOUND)
    {
      THROW_SYSTEM_FAILURE(err);
    }
  }
  else
  {
    do
    {
      if(::strcmp(find_data.cFileName, ".")&&
         ::strcmp(find_data.cFileName, ".."))
      {
        result.emplace_back(find_data.cFileName);
      }
    } while(::FindNextFile(find_handle, &find_data));
    ::FindClose(find_handle);
  }
#else
  auto *d=::opendir(data(path));
  if(!d)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  for(;;)
  {
    const auto *e=::readdir(d);
    if(!e)
    {
      break; // end of directory
    }
    if(::strcmp(e->d_name, ".")&&::strcmp(e->d_name, ".."))
    {
      result.emplace_back(e->d_name);
    }
  }
  auto r=int{};
  RESTART_SYSCALL(r, ::closedir(d));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#endif
  return result;
}

bool // path exists and conforms to mode
access(const std::string &path,
       int mode)
{
  auto r=int{};
#if defined _WIN32
  if(mode&X_OK)
  {
    mode=(mode&~X_OK)|R_OK; // substitute X with R
  }
#endif
#if defined _MSC_VER
  RESTART_SYSCALL(r, ::_access(data(path), mode));
#else
  RESTART_SYSCALL(r, ::access(data(path), mode));
#endif
  return r!=-1;
}

int // file size or -1 (no file)
fileSize(const std::string &path)
{
  auto r=int{};
#if defined _MSC_VER
  struct _stat st;
  RESTART_SYSCALL(r, ::_stat(data(path), &st));
#else
  struct stat st;
  RESTART_SYSCALL(r, ::stat(data(path), &st));
#endif
  return (r!=-1) ? int(st.st_size) : -1;
}

bool // path exists and is a file
isFile(const std::string &path)
{
#if defined _MSC_VER
  struct _stat st;
  auto r=int{};
  RESTART_SYSCALL(r, ::_stat(data(path), &st));
  return (r!=-1)&&(st.st_mode&_S_IFREG);
#else
  struct stat st;
  auto r=int{};
  RESTART_SYSCALL(r, ::stat(data(path), &st));
  return (r!=-1)&&((st.st_mode&S_IFMT)==S_IFREG);
#endif
}

bool // path exists and is a directory
isDir(const std::string &path)
{
#if defined _MSC_VER
  struct _stat st;
  auto r=int{};
  RESTART_SYSCALL(r, ::_stat(data(path), &st));
  return (r!=-1)&&(st.st_mode&_S_IFDIR);
#else
  struct stat st;
  auto r=int{};
# if defined _WIN32 // FIXME: ugly bug with trailing '/' or '\\' in mingw!
  if(!empty(path)&&((path.back()=='/')||(path.back()=='\\')))
  {
    auto tmp=std::string{path, 0, size(path)-1};
    while(!empty(tmp)&&((tmp.back()=='/')||(tmp.back()=='\\')))
    {
      tmp.pop_back();
    }
    if(empty(tmp)||tmp.back()==':')
    {
      tmp.push_back(path.back());
    }
    RESTART_SYSCALL(r, ::stat(data(tmp), &st));
  }
  else // execute the normal syscall
#endif
  RESTART_SYSCALL(r, ::stat(data(path), &st));
  return (r!=-1)&&((st.st_mode&S_IFMT)==S_IFDIR);
#endif
}

bool // path exists and is a named pipe
isFifo(const std::string &path)
{
#if defined _WIN32
  (void)path; // avoid ``unused parameter'' warning
  return false; // does not exist under windows
#else
  struct stat st;
  auto r=int{};
  RESTART_SYSCALL(r, ::stat(data(path), &st));
  return (r!=-1)&&((st.st_mode&S_IFMT)==S_IFIFO);
#endif
}

void
mkdir(const std::string &path)
{
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, ::_mkdir(data(path)));
#elif defined _WIN32
  RESTART_SYSCALL(r, ::mkdir(data(path)));
#else
  RESTART_SYSCALL(r, ::mkdir(data(path),0777));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
}

void
rmdir(const std::string &path)
{
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, ::_rmdir(data(path)));
#else
  RESTART_SYSCALL(r, ::rmdir(data(path)));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
}

void
mkfifo(const std::string &path)
{
#if defined _WIN32
  (void)path; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE(" on Windows");
#else
  auto r=int{};
  RESTART_SYSCALL(r, ::mkfifo(data(path), 0666));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
#endif
}

static
int // file-descriptor
open_(const std::string &path,
      int mode,
      int rights=0666)
{
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, ::_open(data(path), mode, rights));
#else
  RESTART_SYSCALL(r, ::open(data(path), mode, rights));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return r;
}

int // read-only file-descriptor
openR(const std::string &path)
{
  return open_(path, O_RDONLY);
}

int // write-only file-descriptor
openW(const std::string &path,
      bool append,
      bool exclusive)
{
  auto mode=O_WRONLY|O_CREAT;
  mode|=(append ? O_APPEND : O_TRUNC);
  if(exclusive)
  {
    mode|=O_EXCL;
  }
  return open_(path, mode);
}

int // read-write file-descriptor
openRW(const std::string &path)
{
  return open_(path, O_RDWR|O_CREAT);
}

int // new absolute offset
lseek(int fd,
      int offset,
      int origin)
{
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, int(::_lseek(fd, offset, origin)));
#else
  RESTART_SYSCALL(r, int(::lseek(fd, offset, origin)));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return r;
}

void
close(int fd)
{
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, ::_close(fd));
#else
  RESTART_SYSCALL(r, ::close(fd));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
}

void
unlink(const std::string &path)
{
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, ::_unlink(data(path)));
#else
  RESTART_SYSCALL(r, ::unlink(data(path)));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
}

int // written bytes
write(int fd,
      const void *content,
      int content_size)
{
  const auto *ptr=reinterpret_cast<const char *>(content);
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, int(::_write(fd, ptr, content_size)));
#else
  RESTART_SYSCALL(r, int(::write(fd, ptr, content_size)));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return r;
}

int // written bytes (content_size expected)
writeAll(int fd,
         const void *content,
         int content_size)
{
  return writeAll_(fd, content, content_size);
}

int // written bytes (len(msg) expected)
writeAll(int fd,
         const std::string &msg)
{
  return writeAll_(fd, msg);
}

int // read bytes or 0 (EOF)
read(int fd,
     void *buffer,
     int buffer_capacity)
{
  auto *ptr=reinterpret_cast<char *>(buffer);
  auto r=int{};
#if defined _MSC_VER
  RESTART_SYSCALL(r, int(::_read(fd, ptr, buffer_capacity)));
#else
  RESTART_SYSCALL(r, int(::read(fd, ptr, buffer_capacity)));
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SYSCALL_ERRNO);
  }
  return r;
}

int // read bytes (buffer_capacity expected) or 0 (EOF)
readAll(int fd,
        void *buffer,
        int buffer_capacity)
{
  return readAll_(fd, buffer, buffer_capacity);
}

std::string // read text or "" (EOF)
read(int fd,
     int capacity)
{
  return read_(fd, capacity);
}

std::string // read text or "" (EOF)
readAll(int fd,
        int capacity)
{
  return readAll_(fd, capacity);
}

std::string // read text line or "" (EOF)
readLine(int fd)
{
  return readLine_(fd);
}

//----------------------------------------------------------------------------

uint32_t // IPv4 address of dotted-decimal text or 0
parseIpv4Address(const std::string &address)
{
  uint32_t b3, b2, b1, b0;
  if((::sscanf(data(address), " %u.%u.%u.%u ", &b3, &b2, &b1, &b0)!=4)||
     (b3>0x000000FF)||(b2>0x000000FF)||(b1>0x000000FF)||(b0>0x000000FF))
  {
    return 0;
  }
  return ((b3<<24)|(b2<<16)|(b1<<8)|(b0<<0));
}

std::string // dotted-decimal text of IPv4 address
formatIpv4Address(uint32_t address)
{
  return txt("%.%.%.%",
             (address>>24)&0x000000FF,
             (address>>16)&0x000000FF,
             (address>>8)&0x000000FF,
             (address>>0)&0x000000FF);
}

std::string
gethostname()
{
  auto result=std::string{};
  uninitialised_resize(result, 0x100);
  auto r=int{};
  RESTART_SYSCALL(r, ::gethostname(data(result), len(result)));
  result.resize(::strlen(data(result)));
  return result;
}

uint32_t // IPv4 address of host name
gethostbyname(const std::string &hostname)
{
  auto addr=uint32_t{};
  const auto *host=::gethostbyname(data(hostname));
  if(host)
  {
#if defined __APPLE__ && defined __clang__
    // alignment problem with apple's standard library
    auto *ptr=(uint8_t *)nullptr;
    std::memcpy(&ptr, &host->h_addr, sizeof(ptr));
    addr=(uint32_t(ptr[0])<<24)
        |(uint32_t(ptr[1])<<16)
        |(uint32_t(ptr[2])<<8)
        |(uint32_t(ptr[3])<<0);
#else
    addr=ntohl(*reinterpret_cast<const uint32_t *>(host->h_addr));
#endif
  }
  else
  {
    addr=parseIpv4Address(hostname); // try dotted-decimal notation
  }
  if(!addr)
  {
    throw std::runtime_error{txt("%:%:%() unknown host '%'\n%",
                                 __FILE__, __LINE__, __func__,
                                 hostname, computeStackTrace())};
  }
  return addr;
}

SOCKET
socket(int domain,
       int type,
       int protocol)
{
  auto s=SOCKET{};
  RESTART_SYSCALL(s, ::socket(domain, type, protocol));
  if(s==INVALID_SOCKET)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  return s;
}

#if defined _WIN32
  // Windows sockets are handled through the SOCKET type which actualy is a
  // ``long long int'', thus the previously defined close(int) function will
  // be used for usual file-descriptors whereas this close(SOCKET) function
  // will be used for sockets.
  void
  close(SOCKET s)
  {
    auto r=int{};
    RESTART_SYSCALL(r, ::closesocket(s));
    if(r==-1)
    {
      THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
    }
  }
#else
  // anywhere else a socket is simply a file-descriptor thus the
  // previously defined close(int) function will be used in both cases.
#endif

void
shutdown(SOCKET s,
         int how)
{
  auto r=int{};
  RESTART_SYSCALL(r, ::shutdown(s, how));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
}

void
setReuseAddrOption(SOCKET s,
                   bool on)
{
#if defined _WIN32
  const auto option=on ? TRUE : FALSE;
#else
  const auto option=on ? 1 : 0;
#endif
  const auto *opt=reinterpret_cast<const char *>(&option);
  auto r=int{};
  RESTART_SYSCALL(r, ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                                  opt, sizeof(option)));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
}

void
setTcpNodelayOption(SOCKET s,
                    bool on)
{
#if defined _WIN32
  const auto option=on ? TRUE : FALSE;
#else
  const auto option=on ? 1 : 0;
#endif
  const auto *opt=reinterpret_cast<const char *>(&option);
  auto r=int{};
  RESTART_SYSCALL(r, ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                                  opt, sizeof(option)));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
}

void
setBroadcastOption(SOCKET s,
                   bool on)
{
#if defined _WIN32
  const auto option=on ? TRUE : FALSE;
#else
  const auto option=on ? 1 : 0;
#endif
  const auto *opt=reinterpret_cast<const char *>(&option);
  auto r=int{};
  RESTART_SYSCALL(r, ::setsockopt(s, SOL_SOCKET, SO_BROADCAST,
                                  opt, sizeof(option)));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
}

void
bind(SOCKET s,
     uint32_t address,
     uint16_t port)
{
  struct sockaddr_in addr;
  const auto addr_len=socklen_t(sizeof(addr));
  ::memset(&addr, 0, addr_len);
  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);
  addr.sin_addr.s_addr=htonl(address);
  const auto *sa=reinterpret_cast<const struct sockaddr *>(&addr);
  auto r=int{};
  RESTART_SYSCALL(r, ::bind(s, sa, addr_len));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
}

void
bind(SOCKET s,
     uint16_t port)
{
  return bind(s, INADDR_ANY, port);
}

std::tuple<uint32_t, // IPv4 address
           uint16_t> // port number
getsockname(SOCKET s)
{
  struct sockaddr_in addr;
  auto addr_len=socklen_t(sizeof(addr));
  auto *sa=reinterpret_cast<struct sockaddr *>(&addr);
  auto r=int{};
  RESTART_SYSCALL(r, ::getsockname(s, sa, &addr_len));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  return {std::move(ntohl(addr.sin_addr.s_addr)),
          std::move(ntohs(addr.sin_port))};
}

std::tuple<uint32_t, // IPv4 address
           uint16_t> // port number
getpeername(SOCKET s)
{
  struct sockaddr_in addr;
  auto addr_len=socklen_t(sizeof(addr));
  auto *sa=reinterpret_cast<struct sockaddr *>(&addr);
  auto r=int{};
  RESTART_SYSCALL(r, ::getpeername(s, sa, &addr_len));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  return {std::move(ntohl(addr.sin_addr.s_addr)),
          std::move(ntohs(addr.sin_port))};
}

void
listen(SOCKET s,
       int backlog)
{
  auto r=int{};
  RESTART_SYSCALL(r, ::listen(s, backlog));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
}

std::tuple<SOCKET,   // dialog socket
           uint32_t, // IPv4 address
           uint16_t> // port number
acceptfrom(SOCKET listen_socket)
{
  struct sockaddr_in addr;
  auto addr_len=socklen_t(sizeof(addr));
  auto *sa=reinterpret_cast<struct sockaddr *>(&addr);
  auto s=SOCKET{};
  RESTART_SYSCALL(s, ::accept(listen_socket, sa, &addr_len));
  if(s==INVALID_SOCKET)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  return {std::move(s),
          std::move(ntohl(addr.sin_addr.s_addr)),
          std::move(ntohs(addr.sin_port))};
}

SOCKET // dialog socket
accept(SOCKET listen_socket)
{
  const auto [s, address, port]=acceptfrom(listen_socket);
  (void)address; // avoid ``unused variable'' warning
  (void)port;
  return s;
}

void
connect(SOCKET s,
        uint32_t address,
        uint16_t port)
{
  struct sockaddr_in addr;
  const auto addr_len=socklen_t(sizeof(addr));
  ::memset(&addr, 0, addr_len);
  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);
  addr.sin_addr.s_addr=htonl(address);
  const auto *sa=reinterpret_cast<const struct sockaddr *>(&addr);
  auto r=int{};
  RESTART_SYSCALL(r, ::connect(s, sa, addr_len));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
}

int // sent bytes
send(SOCKET s,
     const void *content,
     int content_size)
{
  const auto *ptr=reinterpret_cast<const char *>(content);
  auto r=int{};
  RESTART_SYSCALL(r, int(::send(s, ptr, content_size, 0)));
#if defined _WIN32
  if((r==-1)&&(SOCKET_ERRNO==WSAECONNRESET))
  {
    r=0; // ugly hack!
  }
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  return r;
}

int // sent bytes (content_size expected)
sendAll(SOCKET s,
        const void *content,
        int content_size)
{
  return writeAll_(s, content, content_size);
}

int // sent bytes (len(msg) expected)
sendAll(SOCKET s,
        const std::string &msg)
{
  return writeAll_(s, msg);
}

int // sent bytes
sendto(SOCKET s,
       const void *content,
       int content_size,
       uint32_t address,
       uint16_t port)
{
  const auto *ptr=reinterpret_cast<const char *>(content);
  struct sockaddr_in addr;
  const auto addr_len=socklen_t(sizeof(addr));
  ::memset(&addr, 0, addr_len);
  addr.sin_family=AF_INET;
  addr.sin_port=htons(port);
  addr.sin_addr.s_addr=htonl(address);
  const auto *sa=reinterpret_cast<const struct sockaddr *>(&addr);
  auto r=int{};
  RESTART_SYSCALL(r, int(::sendto(s, ptr, content_size, 0, sa, addr_len)));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  return r;
}

int // sent bytes
sendto(SOCKET s,
       const std::string &msg,
       uint32_t address,
       uint16_t port)
{
  return sendto(s, data(msg), len(msg), address, port);
}

int // received bytes or 0 (EOF)
recv(SOCKET s,
     void *buffer,
     int buffer_capacity)
{
  auto *ptr=reinterpret_cast<char *>(buffer);
  auto r=int{};
  RESTART_SYSCALL(r, int(::recv(s, ptr, buffer_capacity, 0)));
#if defined _WIN32
  if((r==-1)&&(SOCKET_ERRNO==WSAECONNRESET))
  {
    r=0; // ugly hack!
  }
#endif
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  return r;
}

int // received bytes (buffer_capacity expected) or 0 (EOF)
recvAll(SOCKET s,
        void *buffer,
        int buffer_capacity)
{
  return readAll_(s, buffer, buffer_capacity);
}

std::string // received text or "" (EOF)
recv(SOCKET s,
     int capacity)
{
  return read_(s, capacity);
}

std::string // received text or "" (EOF)
recvAll(SOCKET s,
        int capacity)
{
  return readAll_(s, capacity);
}

std::string // received text line or "" (EOF)
recvLine(SOCKET s)
{
  return readLine_(s);
}

std::tuple<int,      // received bytes or 0 (EOF)
           uint32_t, // IPv4 address
           uint16_t> // port number
recvfrom(SOCKET s,
         void *buffer,
         int buffer_capacity)
{
  auto *ptr=reinterpret_cast<char *>(buffer);
  struct sockaddr_in addr;
  auto addr_len=socklen_t(sizeof(addr));
  auto *sa=reinterpret_cast<struct sockaddr *>(&addr);
  auto r=int{};
  RESTART_SYSCALL(r, int(::recvfrom(s, ptr, buffer_capacity,
                                    0, sa, &addr_len)));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  return {std::move(r),
          std::move(ntohl(addr.sin_addr.s_addr)),
          std::move(ntohs(addr.sin_port))};
}

std::tuple<std::string, // received text or "" (EOF)
           uint32_t,    // IPv4 address
           uint16_t>    // port number
recvfrom(SOCKET s,
         int capacity)
{
  auto result=std::string{};
  uninitialised_resize(result, capacity);
  auto [r, address, port]=recvfrom(s, data(result), capacity);
  result.resize(r);
  return {std::move(result),
          std::move(address),
          std::move(port)};
}

int // number of sockets in ready-state
select(std::vector<SOCKET> &inout_read_set,
       std::vector<SOCKET> &inout_write_set,
       double timeout)
{
  auto read_set=fd_set{}, write_set=fd_set{};
  FD_ZERO(&read_set);
  FD_ZERO(&write_set);
  auto max_handle=SOCKET(-1);
  for(const auto &s: inout_read_set)
  {
    FD_SET(s, &read_set);
    max_handle=std::max(max_handle, s);
  }
  for(const auto &s: inout_write_set)
  {
    FD_SET(s, &write_set);
    max_handle=std::max(max_handle, s);
  }
  struct timeval tv;
  if(timeout>=0.0)
  {
#if defined _WIN32
    tv.tv_sec=long(timeout);
    tv.tv_usec=long(1e6*(timeout-double(tv.tv_sec)));
#else
    tv.tv_sec=time_t(timeout);
    tv.tv_usec=int(1e6*(timeout-double(tv.tv_sec)));
#endif
  }
  int r;
  RESTART_SYSCALL(r, ::select((int)max_handle+1,
                              empty(inout_read_set) ? nullptr : &read_set,
                              empty(inout_write_set) ? nullptr : &write_set,
                              nullptr,
                              timeout>=0.0 ? &tv : nullptr));
  if(r==-1)
  {
    THROW_SYSTEM_FAILURE(SOCKET_ERRNO);
  }
  for(auto i=len(inout_read_set); i--; )
  {
    if(!FD_ISSET(inout_read_set[i], &read_set))
    {
      inout_read_set[i]=inout_read_set.back();
      inout_read_set.pop_back();
    }
  }
  for(auto i=len(inout_write_set); i--; )
  {
    if(!FD_ISSET(inout_write_set[i], &write_set))
    {
      inout_write_set[i]=inout_write_set.back();
      inout_write_set.pop_back();
    }
  }
  return r;
}

int // number of sockets in ready-state
select(std::vector<SOCKET> &inout_read_set,
       double timeout)
{
  auto write_set=std::vector<SOCKET>{};
  return select(inout_read_set, write_set, timeout);
}

int16_t // value converted to network byte-order
hton_i16(int16_t host_value)
{
#if BYTE_ORDER==BIG_ENDIAN
  return host_value;
#else
  union { uint8_t b[2]; int16_t v; } u;
  u.b[0]=uint8_t((host_value>>8)&0x00FF);
  u.b[1]=uint8_t((host_value>>0)&0x00FF);
  return u.v;
#endif
}

int16_t // value converted to host byte-order
ntoh_i16(int16_t network_value)
{
  return hton_i16(network_value);
}

uint16_t // value converted to network byte-order
hton_ui16(uint16_t host_value)
{
#if BYTE_ORDER==BIG_ENDIAN
  return host_value;
#else
  union { uint8_t b[2]; uint16_t v; } u;
  u.b[0]=uint8_t((host_value>>8)&0x00FF);
  u.b[1]=uint8_t((host_value>>0)&0x00FF);
  return u.v;
#endif
}

uint16_t // value converted to host byte-order
ntoh_ui16(uint16_t network_value)
{
  return hton_ui16(network_value);
}

int32_t // value converted to network byte-order
hton_i32(int32_t host_value)
{
#if BYTE_ORDER==BIG_ENDIAN
  return host_value;
#else
  union { uint8_t b[4]; int32_t v; } u;
  u.b[0]=uint8_t((host_value>>24)&0x00FF);
  u.b[1]=uint8_t((host_value>>16)&0x00FF);
  u.b[2]=uint8_t((host_value>>8)&0x00FF);
  u.b[3]=uint8_t((host_value>>0)&0x00FF);
  return u.v;
#endif
}

int32_t // value converted to host byte-order
ntoh_i32(int32_t network_value)
{
  return hton_i32(network_value);
}

uint32_t // value converted to network byte-order
hton_ui32(uint32_t host_value)
{
#if BYTE_ORDER==BIG_ENDIAN
  return host_value;
#else
  union { uint8_t b[4]; uint32_t v; } u;
  u.b[0]=uint8_t((host_value>>24)&0x00FF);
  u.b[1]=uint8_t((host_value>>16)&0x00FF);
  u.b[2]=uint8_t((host_value>>8)&0x00FF);
  u.b[3]=uint8_t((host_value>>0)&0x00FF);
  return u.v;
#endif
}

uint32_t // value converted to host byte-order
ntoh_ui32(uint32_t network_value)
{
  return hton_ui32(network_value);
}

int64_t // value converted to network byte-order
hton_i64(int64_t host_value)
{
#if BYTE_ORDER==BIG_ENDIAN
  return host_value;
#else
  union { uint8_t b[8]; int64_t v; } u;
  u.b[0]=uint8_t((host_value>>56)&0x00FF);
  u.b[1]=uint8_t((host_value>>48)&0x00FF);
  u.b[2]=uint8_t((host_value>>40)&0x00FF);
  u.b[3]=uint8_t((host_value>>32)&0x00FF);
  u.b[4]=uint8_t((host_value>>24)&0x00FF);
  u.b[5]=uint8_t((host_value>>16)&0x00FF);
  u.b[6]=uint8_t((host_value>>8)&0x00FF);
  u.b[7]=uint8_t((host_value>>0)&0x00FF);
  return u.v;
#endif
}

int64_t // value converted to host byte-order
ntoh_i64(int64_t network_value)
{
  return hton_i64(network_value);
}

uint64_t // value converted to network byte-order
hton_ui64(uint64_t host_value)
{
#if BYTE_ORDER==BIG_ENDIAN
  return host_value;
#else
  union { uint8_t b[8]; uint64_t v; } u;
  u.b[0]=uint8_t((host_value>>56)&0x00FF);
  u.b[1]=uint8_t((host_value>>48)&0x00FF);
  u.b[2]=uint8_t((host_value>>40)&0x00FF);
  u.b[3]=uint8_t((host_value>>32)&0x00FF);
  u.b[4]=uint8_t((host_value>>24)&0x00FF);
  u.b[5]=uint8_t((host_value>>16)&0x00FF);
  u.b[6]=uint8_t((host_value>>8)&0x00FF);
  u.b[7]=uint8_t((host_value>>0)&0x00FF);
  return u.v;
#endif
}

uint64_t // value converted to host byte-order
ntoh_ui64(uint64_t network_value)
{
  return hton_ui64(network_value);
}

real32_t // value converted to network byte-order
hton_r32(real32_t host_value)
{
#if BYTE_ORDER==BIG_ENDIAN
  return host_value;
#else
  union { real32_t v; uint32_t i; } u1, u2;
  u1.v=host_value;
  u2.i=hton_ui32(u1.i);
  return u2.v;
#endif
}

real32_t // value converted to host byte-order
ntoh_r32(real32_t network_value)
{
  return hton_r32(network_value);
}

real64_t // value converted to network byte-order
hton_r64(real64_t host_value)
{
#if BYTE_ORDER==BIG_ENDIAN
  return host_value;
#else
  union { real64_t v; uint64_t i; } u1, u2;
  u1.v=host_value;
  u2.i=hton_ui64(u1.i);
  return u2.v;
#endif
}

real64_t // value converted to host byte-order
ntoh_r64(real64_t network_value)
{
  return hton_r64(network_value);
}

//----------------------------------------------------------------------------

SSL_CTX *
sslInit(const std::string &ca_cert_path,
        const std::string &cert_path,
        const std::string &key_path)
{
#if !USE_SSL
  (void)ca_cert_path; // avoid ``unused parameter'' warning
  (void)cert_path;
  (void)key_path;
  THROW_NOT_AVAILABLE("");
  return nullptr; // never reached
#else
  auto *ctx=::SSL_CTX_new(::TLS_method());
  // provide SSL with the list of known CA
  if(!empty(ca_cert_path)&&
     !::SSL_CTX_load_verify_locations(ctx, data(ca_cert_path), nullptr))
  {
    THROW_SSL_ERROR(SSL_CTX_load_verify_locations);
  }
  // provide SSL with a certificate/key pair
  if(!empty(cert_path)&&!empty(key_path))
  {
    ::SSL_CTX_use_certificate_file(ctx, data(cert_path), SSL_FILETYPE_PEM);
    ::SSL_CTX_use_PrivateKey_file(ctx, data(key_path), SSL_FILETYPE_PEM);
    if(!::SSL_CTX_check_private_key(ctx))
    {
      THROW_SSL_ERROR(SSL_CTX_check_private_key);
    }
  }
  return ctx;
#endif
}

void
sslDestroy(SSL_CTX *ctx)
{
  if(ctx!=nullptr)
  {
#if !USE_SSL
    THROW_NOT_AVAILABLE("");
#else
    ::SSL_CTX_free(ctx);
#endif
  }
}

SSL *
sslConnect(SOCKET s,
           SSL_CTX *ctx,
           const std::string &hostname)
{
#if !USE_SSL
  (void)s; // avoid ``unused parameter'' warning
  (void)ctx;
  (void)hostname;
  THROW_NOT_AVAILABLE("");
  return nullptr; // never reached
#else
  auto *ssl=::SSL_new(ctx);
  ::SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  ::SSL_set_fd(ssl, s);
  if(!empty(hostname))
  {
    ::SSL_set_tlsext_host_name(ssl, data(hostname));
    // nb: no need to report an error if this extension is not supported
  }
  if(::SSL_connect(ssl)!=1)
  {
    THROW_SSL_ERROR(SSL_connect);
  }
  if(::SSL_get_verify_result(ssl)!=X509_V_OK)
  {
    err("!!! Warning !!! peer certificate not trusted\n");
  }
  if(!empty(hostname))
  {
    auto *cert=::SSL_get_peer_certificate(ssl);
    if(!cert)
    {
      err("!!! Warning !!! %\n", SSL_ERROR_MSG(SSL_get_peer_certificate));
    }
    else
    {
      char common_name[0x100]="";
      ::X509_NAME_get_text_by_NID(::X509_get_subject_name(cert),
                                  NID_commonName,
                                  common_name, sizeof(common_name));
      if(hostname!=common_name)
      {
        err("!!! Warning !!! Common name '%' != host name '%'\n",
            common_name, hostname);
      }
      ::X509_free(cert);
    }
  }
  return ssl;
#endif
}

SSL *
sslAccept(SOCKET s,
          SSL_CTX *ctx)
{
#if !USE_SSL
  (void)s; // avoid ``unused parameter'' warning
  (void)ctx;
  THROW_NOT_AVAILABLE("");
  return nullptr; // never reached
#else
  auto *ssl=SSL_new(ctx);
  ::SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  ::SSL_set_fd(ssl, s);
  if(::SSL_accept(ssl)!=1)
  {
    THROW_SSL_ERROR(SSL_accept);
  }
  if(::SSL_get_verify_result(ssl)!=X509_V_OK)
  {
    err("!!! Warning !!! peer certificate not trusted\n");
  }
  return ssl;
#endif
}

bool // some bytes are immediately available
sslPending(SSL *ssl)
{
#if !USE_SSL
  (void)ssl; // avoid ``unused parameter'' warning
  THROW_NOT_AVAILABLE("");
  return false; // never reached
#else
  return ::SSL_pending(ssl)>0;
#endif
}

void
sslClose(SSL *ssl)
{
  if(ssl!=nullptr)
  {
#if !USE_SSL
    THROW_NOT_AVAILABLE("");
#else
    ::SSL_free(ssl);
#endif
  }
}

int // sent bytes
send(SSL *ssl,
     const void *content,
     int content_size)
{
#if !USE_SSL
  (void)ssl; // avoid ``unused parameter'' warning
  (void)content;
  (void)content_size;
  THROW_NOT_AVAILABLE("");
  return 0; // never reached
#else
  const auto *ptr=reinterpret_cast<const char *>(content);
  const auto r=::SSL_write(ssl, ptr, content_size);
  if(r<0)
  {
    THROW_SSL_ERROR(SSL_write);
  }
  return r;
#endif
}

int // sent bytes (content_size expected)
sendAll(SSL *ssl,
        const void *content,
        int content_size)
{
  return writeAll_(ssl, content, content_size);
}

int // sent bytes (len(msg) expected)
sendAll(SSL *ssl,
        const std::string &msg)
{
  return writeAll_(ssl, msg);
}

int // received bytes or 0 (EOF)
recv(SSL *ssl,
     void *buffer,
     int buffer_capacity)
{
#if !USE_SSL
  (void)ssl; // avoid ``unused parameter'' warning
  (void)buffer;
  (void)buffer_capacity;
  THROW_NOT_AVAILABLE("");
  return 0; // never reached
#else
  auto *ptr=reinterpret_cast<char *>(buffer);
  const auto r=::SSL_read(ssl, ptr, buffer_capacity);
  if(r<0)
  {
    THROW_SSL_ERROR(SSL_read);
  }
  return r;
#endif
}

int // received bytes (buffer_capacity expected) or 0 (EOF)
recvAll(SSL *ssl,
        void *buffer,
        int buffer_capacity)
{
  return readAll_(ssl, buffer, buffer_capacity);
}

std::string // received text or "" (EOF)
recv(SSL *ssl,
     int capacity)
{
  return read_(ssl, capacity);
}

std::string // received text or "" (EOF)
recvAll(SSL *ssl,
        int capacity)
{
  return readAll_(ssl, capacity);
}

std::string // received text line or "" (EOF)
recvLine(SSL *ssl)
{
  return readLine_(ssl);
}

//----------------------------------------------------------------------------

std::string
wsHandshake(std::string key)
{
  // inspired from https://github.com/alexhultman/libwshandshake
  while(len(key)<24)
  {
    key+='=';
  }
  if(len(key)>24)
  {
    key.resize(24);
  }
  auto b_output=std::array<uint32_t, 5>
    {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
  auto b_input=std::array<uint32_t, 16>
    {0, 0, 0, 0, 0, 0, 0x32353845, 0x41464135, 0x2d453931, 0x342d3437,
     0x44412d39, 0x3543412d, 0x43354142, 0x30444338, 0x35423131, 0x80000000};
  for(auto i=0; i<6; i++)
  {
    b_input[i]=(key[4*i+3]&255)<<0  | (key[4*i+2]&255)<<8 |
               (key[4*i+1]&255)<<16 | (key[4*i+0]&255)<<24;
  }
  const auto sha1=
    [&](auto &hash, auto &b)
    {
      const auto rol=
        [&](const auto &value, const auto &bits)
        {
          return (value<<bits)|(value>>(32-bits));
        };
      const auto blk=
        [&](const auto &b, const auto &s)
        {
          return rol(b[(s+13)&15]^b[(s+8)&15]^b[(s+2)&15]^b[s], 1);
        };
      uint32_t a[5]={hash[4], hash[3], hash[2], hash[1], hash[0]};
      for(auto i=0; i<16; ++i)
      {
        a[i%5]+=((a[(3+i)%5]&(a[(2+i)%5]^a[(1+i)%5]))^a[(1+i)%5])+
                b[i]+0x5a827999+rol(a[(4+i)%5], 5);
        a[(3+i)%5]=rol(a[(3+i)%5], 30);
      }
      for(auto i=0; i<4; ++i)
      {
        b[i]=blk(b, i);
        a[(1+i)%5]+=((a[(4+i)%5]&(a[(3+i)%5]^a[(2+i)%5]))^a[(2+i)%5])+
                    b[i]+0x5a827999+rol(a[(5+i)%5], 5);
        a[(4+i)%5]=rol(a[(4+i)%5], 30);
      }
      for(auto i=0; i<20; ++i)
      {
        b[(i+4)%16]=blk(b, (i+4)%16);
        a[i%5]+=(a[(3+i)%5]^a[(2+i)%5]^a[(1+i)%5])+
                b[(i+4)%16]+0x6ed9eba1+rol(a[(4+i)%5], 5);
        a[(3+i)%5]=rol(a[(3+i)%5], 30);
      }
      for(auto i=0; i<20; ++i)
      {
        b[(i+8)%16]=blk(b, (i+8)%16);
        a[i%5]+=(((a[(3+i)%5]|a[(2+i)%5])&a[(1+i)%5])|(a[(3+i)%5]&a[(2+i)%5]))+
                b[(i+8)%16]+0x8f1bbcdc+rol(a[(4+i)%5], 5);
        a[(3+i)%5]=rol(a[(3+i)%5], 30);
      }
      for(auto i=0; i<20; ++i)
      {
        b[(i+12)%16]=blk(b, (i+12)%16);
        a[i%5]+=(a[(3+i)%5]^a[(2+i)%5]^a[(1+i)%5])+
                b[(i+12)%16]+0xca62c1d6+rol(a[(4+i)%5], 5);
        a[(3+i)%5]=rol(a[(3+i)%5], 30);
      }
      for(auto i=0; i<5; ++i)
      {
        hash[i]+=a[4-i];
      }
    };
  sha1(b_output, b_input);
  auto last_b=std::array<uint32_t, 16>{0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 480};
  sha1(b_output, last_b);
  for(auto i=0; i<5; i++)
  {
    const auto tmp=b_output[i];
    auto *bytes=reinterpret_cast<char *>(&b_output[i]);
    bytes[3]=char((tmp>>0)&255);
    bytes[2]=char((tmp>>8)&255);
    bytes[1]=char((tmp>>16)&255);
    bytes[0]=char((tmp>>24)&255);
  }
  auto buffer=std::array<char, 29>{};
  const auto *src=reinterpret_cast<const unsigned char *>(data(b_output));
  const auto *b64="ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                  "abcdefghijklmnopqrstuvwxyz"
                  "0123456789+/";
  auto dst=begin(buffer);
  for(int i=0; i<18; i+=3)
  {
    *dst++=b64[(src[i]>>2)&63];
    *dst++=b64[((src[i]&3)<<4) | ((src[i+1]&240)>>4)];
    *dst++=b64[((src[i+1]&15)<<2) | ((src[i+2]&192)>>6)];
    *dst++=b64[src[i+2]&63];
  }
  *dst++=b64[(src[18]>>2)&63];
  *dst++=b64[((src[18]&3)<<4) | ((src[19]&240)>>4)];
  *dst++=b64[((src[19]&15)<<2)];
  *dst++='=';
  *dst++='\0';
  return data(buffer);
}

template<typename Dst>
inline
void
wsSend_(Dst dst,
        const void *content,
        int content_size,
        WsOpcode opcode)
{
  auto header=std::array<uint8_t, 10>{};
  auto header_length=2;
  header[0]=uint8_t(128|int(opcode));
  if(content_size<126)
  {
    const auto sz=uint8_t(content_size);
    header[1]=sz;
  }
  else if(content_size<65536)
  {
    const auto sz=uint16_t(content_size);
    header[1]=126;
    header[2]=uint8_t((sz>>8)&255);
    header[3]=uint8_t((sz>>0)&255);
    header_length+=int(sizeof(sz));
  }
  else
  {
    const auto sz=uint64_t(content_size);
    header[1]=127;
    header[2]=uint8_t((sz>>56)&255);
    header[3]=uint8_t((sz>>48)&255);
    header[4]=uint8_t((sz>>40)&255);
    header[5]=uint8_t((sz>>32)&255);
    header[6]=uint8_t((sz>>24)&255);
    header[7]=uint8_t((sz>>16)&255);
    header[8]=uint8_t((sz>>8)&255);
    header[9]=uint8_t((sz>>0)&255);
    header_length+=int(sizeof(sz));
  }
  sendAll(dst, data(header), header_length);
  sendAll(dst, content, content_size);
}

void
wsSend(SOCKET s,
       const void *content,
       int content_size,
       WsOpcode opcode)
{
  return wsSend_(s, content, content_size, opcode);
}

void
wsSend(SSL *ssl,
       const void *content,
       int content_size,
       WsOpcode opcode)
{
  return wsSend_(ssl, content, content_size, opcode);
}

void
wsSend(SOCKET s,
       const std::string &msg,
       WsOpcode opcode)
{
  return wsSend(s, data(msg), len(msg), opcode);
}

void
wsSend(SSL *ssl,
       const std::string &msg,
       WsOpcode opcode)
{
  return wsSend(ssl, data(msg), len(msg), opcode);
}

void
wsSendClose(SOCKET s)
{
  wsSend(s, nullptr, 0, WS_CLOSE);
}

void
wsSendClose(SSL *ssl)
{
  wsSend(ssl, nullptr, 0, WS_CLOSE);
}

template<typename Src>
inline
std::tuple<WsOpcode, // opcode
           int,      // message length
           uint32_t> // mask
wsRecvHeader_(Src src)
{
  auto header=std::array<uint8_t, 2>{};
  if(recvAll(src, data(header), 2)!=2)
  {
    return {WS_NONE, 0, 0};
  }
  // const auto fin=bool(header[0]&128); // ignore fragmentation for now
  const auto opcode=WsOpcode(header[0]&15);
  auto mask=uint32_t(header[1]&128);
  auto length=int(header[1]&127);
  if(length==126)
  {
    auto sz=std::array<uint8_t, 2>{};
    if(recvAll(src, data(sz), sizeof(sz))!=sizeof(sz))
    {
      return {WS_NONE, 0, 0};
    }
    length=int((uint16_t(sz[0])<<8)|(uint16_t(sz[1])<<0));
  }
  else if(length==127)
  {
    auto sz=std::array<uint8_t, 8>{};
    if(recvAll(src, data(sz), sizeof(sz))!=sizeof(sz))
    {
      return {WS_NONE, 0, 0};
    }
    length=int((uint64_t(sz[0])<<56)|(uint64_t(sz[1])<<48)|
               (uint64_t(sz[2])<<40)|(uint64_t(sz[3])<<32)|
               (uint64_t(sz[4])<<24)|(uint64_t(sz[5])<<16)|
               (uint64_t(sz[6])<<8 )|(uint64_t(sz[7])<<0 ));
  }
  if(mask&&(recvAll(src, &mask, sizeof(mask))!=sizeof(mask)))
  {
    return {WS_NONE, 0, 0};
  }
  return {opcode, length, mask};
}

static
void
wsApplyMask_(void *content,
             int content_size,
             uint32_t mask)
{
  auto *words=reinterpret_cast<decltype(mask) *>(content);
  const auto word_count=content_size/int(sizeof(*words));
  for(auto i=0; i<word_count; ++i)
  {
    words[i]^=mask;
  }
  const auto *byte_mask=reinterpret_cast<const std::uint8_t *>(&mask);
  auto *bytes=reinterpret_cast<std::uint8_t *>(words+word_count);
  const auto byte_count=content_size-word_count*int(sizeof(*words));
  for(auto i=0; i<byte_count; ++i)
  {
    bytes[i]^=byte_mask[i];
  }
}

template<typename Src>
inline
std::tuple<WsOpcode, // opcode
           int>      // received bytes or 0 (EOF)
wsRecv_(Src src,
        void *buffer,
        int buffer_capacity)
{
  const auto [opcode, length, mask]=wsRecvHeader_(src);
  if(!length)
  {
    return {WS_NONE, 0};
  }
  const auto trailing_zero=opcode==WS_TXT ? 1 : 0;
  if(length+trailing_zero>buffer_capacity)
  {
    throw std::runtime_error{txt("insufficient capacity "
                                 "for websocket message\n%",
                                 computeStackTrace())};
  }
  if(recvAll(src, buffer, length)!=length)
  {
    return {WS_NONE, 0};
  }
  if(mask)
  {
    wsApplyMask_(buffer, length, mask);
  }
  if(trailing_zero)
  {
    reinterpret_cast<char *>(buffer)[length]='\0';
  }
  return {opcode, length};
}

std::tuple<WsOpcode, // opcode
           int>      // received bytes or 0 (EOF)
wsRecv(SOCKET s,
       void *buffer,
       int buffer_capacity)
{
  return wsRecv_(s, buffer, buffer_capacity);
}

std::tuple<WsOpcode, // opcode
           int>      // received bytes or 0 (EOF)
wsRecv(SSL *ssl,
       void *buffer,
       int buffer_capacity)
{
  return wsRecv_(ssl, buffer, buffer_capacity);
}

template<typename Src>
inline
std::tuple<WsOpcode,    // opcode
           std::string> // received text or "" (EOF)
wsRecv_(Src src)
{
  const auto [opcode, length, mask]=wsRecvHeader_(src);
  if(!length)
  {
    return {WS_NONE, ""};
  }
  auto result=std::string{};
  uninitialised_resize(result, length);
  if(recvAll(src, data(result), length)!=length)
  {
    return {WS_NONE, ""};
  }
  if(mask)
  {
    wsApplyMask_(data(result), length, mask);
  }
  return {opcode, std::move(result)};
}

std::tuple<WsOpcode,    // opcode
           std::string> // received text or "" (EOF)
wsRecv(SOCKET s)
{
  return wsRecv_(s);
}

std::tuple<WsOpcode,    // opcode
           std::string> // received text or "" (EOF)
wsRecv(SSL *ssl)
{
  return wsRecv_(ssl);
}

} // namespace crs

//----------------------------------------------------------------------------
