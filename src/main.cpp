#include <cstddef>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <unistd.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#define SERV_NAME "test"
#define CLIENT_TTL 10000

struct channel
{
   std::string _chan_id; //maybe use another type ?
  char _prefix;
  struct client *_creator;
  std::vector<struct client *> _members;
  std::vector<struct client *> _operators;
  // O - give "channel creator" status;
  // o - give/take channel operator privilege;
  // v - give/take the voice privilege;

  // a - toggle the anonymous channel flag;
  // i - toggle the invite-only channel flag;
  // m - toggle the moderated channel;
  // n - toggle the no messages to channel from clients on the
  //     outside;
  // q - toggle the quiet channel flag;
  // p - toggle the private channel flag;
  // s - toggle the secret channel flag;
  // r - toggle the server reop channel flag;
  // t - toggle the topic settable by channel operator only flag;

  // k - set/remove the channel key (password);
  // l - set/remove the user limit to channel;

  // b - set/remove ban mask to keep users out;
  // e - set/remove an exception mask to override a ban mask;
  // I - set/remove an invitation mask to automatically override
  //     the invite-only flag;

  std::string _mode;
  // 200 char 
  // ' ' ',' ascii 7 
  // begin with '&' (distributed channel) or '#' 
  std::string _name;
  std::string _topic;
};

struct client
{
  //networking
	int			_fd;
  int     _timer_id;
  bool    _ping;
  //user related
  std::string _nickname;
  std::string _username;
  std::string _hostname;
  std::string _servername;
  std::string _realname;
	std::string _mode;
  //buffers
  std::string _send_buffer;
  std::string _receive_buffer;
};

struct server
{
  //general
  std::string _servername;
	std::map<int, client *>	_connections;
	std::map<std::string, client *>	_nick_map;
  std::vector<channel *> _chan_list;
  //socket related
  struct sockaddr		_sock_addr;
	socklen_t		_socklen;
	int			_socketfd;
  //kevent related
	struct kevent		_changelist;
	struct kevent		_eventlist;
	struct timespec 	_timeout;
	int 			_kq;
  //member functions
	void			init(char **argv);
	void			run();
	void			add_connection();
	void			close_connection(client *client);
	void			send_message();
	void			receive_message();
	void			ping();
  void      regular_tasks();
	void			safe_shutdown(int exit_code);
};

//to be expanded as needed
void server::safe_shutdown(int exit_code)
{	
	if (_socketfd) 
		close(_socketfd);
	if (_kq) 
		close(_kq);
  //TODO
	exit(exit_code);
}

void server::close_connection(client *client)
{
  close(client->_fd);
  //TODO
}

void server::regular_tasks()
{
  std::cout<<"regular tasks"<<std::endl;
  //TODO add nick cleaning 
}

void server::ping()
{
  client *tmp = _connections[_eventlist.ident];
  std::cout<<"fd timer "<<tmp->_fd<<std::endl;
  if (tmp->_ping && _eventlist.filter & EVFILT_TIMER)
  {
    //TODO
    //// other logic maybe
    // close_connection(tmp);
  }
  
  
  //   send ping msg;
  // update timer
	EV_SET(&_changelist, _eventlist.ident, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT, 0, CLIENT_TTL, NULL);
  if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when resetting timer"<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::add_connection()
{
	std::cout<<"new connection"<<std::endl;
	int newfd  = accept(_socketfd, &(_sock_addr), &(_socklen));
  if (newfd == -1)
	{
		std::cerr<<"socket error when adding new connection "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
  }
	_connections[newfd] = new client();
	_connections[newfd]->_fd = newfd; 
	_connections[newfd]->_ping = 0;
	EV_SET(&_changelist, newfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when adding new connection "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	EV_SET(&_changelist, newfd, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT, 0, CLIENT_TTL, NULL);
  if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when adding new connection "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::receive_message()
{
  char *buffer = (char *)malloc(sizeof(char) * (size_t)(_eventlist.data));
	std::cout<<"received message on fd : "<<_eventlist.ident<<std::endl;
  recv(_eventlist.ident, buffer,(size_t)(_eventlist.data), 0);
  //TODO error check
	std::cout<<buffer<<std::endl;
  //_connections[_eventlist.ident]->_receive_buffer.append(buffer);
  free(buffer);
	EV_SET(&_changelist, _eventlist.ident, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT, 0, CLIENT_TTL, NULL);
  if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when resetting timer"<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::send_message()
{
  client *tmp = _connections[_eventlist.ident];
	std::cout<<"sent message on fd : "<<_eventlist.ident<<std::endl;
  send(tmp->_fd, tmp->_send_buffer.c_str(), tmp->_send_buffer.size(), 0);
  //TODO error check and clear buffer
  EV_SET(&_changelist, _eventlist.ident, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT, 0, CLIENT_TTL, NULL);
  if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when resetting timer"<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::run()
{
	int nbr_event;
	while (1)
	{
		nbr_event = 0;
		nbr_event = kevent(_kq, NULL, 0, &_eventlist, 1, NULL);// &_timeout);
    std::cout<<nbr_event<<std::endl;
    if (nbr_event == -1)
    {
      std::cerr<<"kqueue error durring runtime"<<std::endl<<"error: "<<strerror(errno)<<std::endl;
      safe_shutdown(EXIT_FAILURE);
    }
    else if (nbr_event == 0)
      continue;
    else if ((_eventlist.filter == EVFILT_READ) && _eventlist.ident != (uintptr_t)_socketfd)
      this->receive_message();
    else if (_eventlist.filter == EVFILT_WRITE)
      this->send_message();
    else if (!(_eventlist.filter == EVFILT_TIMER) && _eventlist.ident == (uintptr_t)_socketfd)
      this->add_connection();
    else if ((_eventlist.filter == EVFILT_TIMER) && _eventlist.ident == (uintptr_t)_socketfd)
      this->regular_tasks();
    else if (_eventlist.filter == EVFILT_TIMER)
      this->ping();
	}
}

void server::init(char **argv)
{
	//define the "name" assigned to the server socket
	struct sockaddr_in tmp; 
	tmp.sin_family = AF_INET;
	tmp.sin_port = htons(atoi(argv[1]));
	tmp.sin_addr.s_addr = INADDR_ANY;
	memcpy(&_sock_addr, &tmp, sizeof(struct sockaddr));//fucking hacky code, fucking c++ and its fucking types
	_socklen = sizeof(struct sockaddr_in);
  //memset shit to 0
  memset(&_timeout, 0, sizeof(struct timespec));
  memset(&_changelist, 0, sizeof(struct kevent));
  memset(&_eventlist, 0, sizeof(struct kevent));
	//create the server socket and bind it
	if (((_socketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)\
	|| (fcntl(_socketfd, F_SETFL, O_NONBLOCK) == -1)\
	|| (bind(_socketfd, &(_sock_addr), sizeof(_sock_addr)) == -1)\
	|| (listen(_socketfd, 1) == -1))
	{
		std::cerr<<"socket error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
	//create the kqueue
	if ((_kq = kqueue()) == -1)
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
  //add server socket to the kqueue
	EV_SET(&(_changelist), _socketfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error durring server startup : read"<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
  //add server timer for regular tasks
	EV_SET(&(_changelist), _socketfd, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 3000, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error durring server startup : timer "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr<<"usage: ./ft_irc [port]"<<std::endl;
		return (EXIT_FAILURE);
	}
	server server;
	server.init(argv);
	server.run();
	server.safe_shutdown(EXIT_SUCCESS);
}
