#include "irc.hpp"
#include <cstdio>

// event loop using kevents
void server::run()
{
	int nbr_event;
	while (1)
	{
		nbr_event = 0;
		nbr_event = kevent(_kq, NULL, 0, &_eventlist, 1, NULL);// &_timeout);
		if (nbr_event == -1)
		{
			std::cerr<<"kqueue error durring runtime"<<std::endl<<"error: "<<strerror(errno)<<std::endl;
			safe_shutdown(EXIT_FAILURE);
		}
		else if (nbr_event == 0)
			continue;
		else if ( _eventlist.ident == STDIN_FILENO)// events on standard input
			server_admin();
		else if ( _eventlist.ident == (uintptr_t)_socketfd)// events on server socket
		{
			if (_eventlist.filter == EVFILT_READ)
				this->add_connection();
			else if (_eventlist.filter == EVFILT_TIMER)
				this->regular_tasks();	
		}
		else // events on a client socket
		{
			if (_eventlist.filter == EVFILT_WRITE)
				this->send_message();
			else if (_eventlist.filter == EVFILT_READ)
				this->receive_message();
			else if (_eventlist.filter == EVFILT_TIMER)
        this->check_connection(_connections[_eventlist.ident]);
		}
	}
}

void server::init(char **argv)
{
  //server info
	_password.append(argv[2]);
  _servername = SERVNAME;
	std::time_t tmp_time = std::time(nullptr);
	_creation_date.append(std::asctime(std::localtime(&tmp_time)));
  _res_start = NULL;
  //create map for parsing 
	_cmds["KICK"] = KICK;
	_cmds["INVITE"] = INVITE;
	_cmds["TOPIC"] = TOPIC;
	_cmds["MODE"] = MODE;
	_cmds["NICK"] = NICK;
	_cmds["PRIVMSG"] = PRIVMSG;
	_cmds["JOIN"] = JOIN;
	_cmds["PING"] = PING;
	_cmds["PONG"] = PONG;
	_cmds["BAN"] = BAN;
	_cmds["PASS"] = PASS;
	_cmds["USER"] = USER;
	_cmds["PART"] = PART;
	//memset shit to 0
	memset(&_timeout, 0, sizeof(struct timespec));
	memset(&_changelist, 0, sizeof(struct kevent));
	memset(&_eventlist, 0, sizeof(struct kevent));
	//define the info's relevant to the server socket
  memset(&_hints, 0, sizeof(_hints)); // make sure the struct is empty
  _hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
  _hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
  _hints.ai_protocol = IPPROTO_TCP; // TCP protocol
  _hints.ai_flags = AI_PASSIVE;     // INADDR_ANY kinda
  if (getaddrinfo(NULL, argv[1], &_hints, &_res) != 0) {
		std::cerr<<"getaddrinfo error durring server startup "<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
  }
  _res_start = _res;
  while (_res->ai_family != PF_INET && _res->ai_family != PF_INET6 && _res->ai_next)
    _res = _res->ai_next;
  if (_res == NULL) {
		std::cerr<<"no ip socket durring server startup "<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
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
	if ((_kq = kqueue()) == -1)//create the kqueue
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	read_set(_socketfd);//add server socket to the kqueue
	read_set(STDIN_FILENO);//add server socket to the kqueue
	//add server timer for regular tasks
	EV_SET(&_changelist, _socketfd, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR , 0, REG_TASK_TIMER, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when resetting timer"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		std::cerr<<"usage: ./ft_irc [port] [password]"<<std::endl;
		return (EXIT_FAILURE);
	}
  else if (atoi(argv[1]) <= 1024) {
		std::cerr<<"error: reserved or wrong port"<<std::endl;
		return (EXIT_FAILURE);
	}  
	server server;
	server.init(argv);
	server.run();
	server.safe_shutdown(EXIT_SUCCESS);
}
