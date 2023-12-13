#include "irc.hpp"

static void init_cmds_map(std::map<std::string, int> &_cmds)
{
  //init map used for parsing  
	_cmds["KICK"] = KICK;
	_cmds["INVITE"] = INVITE;
	_cmds["TOPIC"] = TOPIC;
	_cmds["MODE"] = MODE;
	_cmds["NICK"] = NICK;
	_cmds["PRIVMSG"] = PRIVMSG;
	_cmds["JOIN"] = JOIN;
	_cmds["PING"] = PING;
	_cmds["PONG"] = PONG;
	_cmds["PASS"] = PASS;
	_cmds["USER"] = USER;
	_cmds["PART"] = PART;
	_cmds["QUIT"] = QUIT;
  
} 

void server::network_init(char *port)
{
  // init a addrinfo struct to retrieve necesary infos
  _res_start = NULL;
  memset(&_hints, 0, sizeof(_hints)); // make sure the struct is empty
  _hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
  _hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  _hints.ai_protocol = IPPROTO_TCP; // TCP protocol
  _hints.ai_flags = AI_PASSIVE;     // INADDR_ANY kinda
  // fill the res with the necesary infos
  if (getaddrinfo(NULL, port, &_hints, &_res) != 0) {
		std::cerr<<"getaddrinfo error durring server startup "<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
  }
  _res_start = _res; // keep track of linked list start to free it properly
  // loop through the linked list to find an ip connection
  while (_res->ai_family != PF_INET && _res->ai_family != PF_INET6 && _res->ai_next)
    _res = _res->ai_next;
  if (_res == NULL) {
		std::cerr<<"no IP socket found durring server startup "<<std::endl;
		safe_shutdown(EXIT_FAILURE);
  }
	//create the server socket and bind it
	int yes=1; // for the setsockopt()
  if (((_socketfd = socket(_res->ai_family, _res->ai_socktype, _res->ai_protocol)) == -1)\
			|| (fcntl(_socketfd, F_SETFL, O_NONBLOCK) == -1)\
      || (setsockopt(_socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)\
			|| (bind(_socketfd, _res->ai_addr, _res->ai_addrlen) == -1)\
			|| ((listen(_socketfd, 2) == -1) && printf("HERE\n")))
	{
		std::cerr<<"socket error durring server startup "<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::kevent_init()
{
	//memset shit to 0
	memset(&_timeout, 0, sizeof(struct timespec));
	memset(&_changelist, 0, sizeof(struct kevent));
	memset(&_eventlist, 0, sizeof(struct kevent));
  //create the kqueue
	if ((_kq = kqueue()) == -1)
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
  //add server socket to the kqueue
	read_set(_socketfd);
  //add stdin to the kqueue
	read_set(STDIN_FILENO);
	//add server timer for regular tasks
	EV_SET(&_changelist, _socketfd, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR , 0, REG_TASK_TIMER, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when resetting timer"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::init(char **argv)
{
  //server info
	_password.append(argv[2]);
  _servername = SERVNAME;
	std::time_t tmp_time = std::time(nullptr);
	_creation_date.append(std::asctime(std::localtime(&tmp_time)));
  try { init_cmds_map(_cmds); } catch (std::exception &e) { 
    std::cerr<< "error when init cmds math: "<<e.what()<<std::endl; 
		safe_shutdown(EXIT_FAILURE);
  }
  network_init(argv[1]);
  kevent_init();
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		std::cerr<<"usage: ./ft_irc [port] [password]"<<std::endl;
		return (EXIT_FAILURE);
	}
  if (atoi(argv[1]) <= 1024) {
		std::cerr<<"error: reserved or wrong port"<<std::endl;
		return (EXIT_FAILURE);
	}  
	server server;
	server.init(argv);
  try { server.run(); } 
  catch (std::exception &e) { std::cerr<< "error during runtime: "<<e.what()<<std::endl; }
	server.safe_shutdown(EXIT_SUCCESS);
}
