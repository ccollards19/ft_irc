#include <cstddef>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <unistd.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>


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
	void			safe_shutdown(int exit_code);
};

//to be expanded as needed
void server::safe_shutdown(int exit_code)
{	
	if (_socketfd) 
		close(_socketfd);
	if (_kq) 
		close(_kq);
	exit(exit_code);
}

void server::close_connection(client *client)
{
  close(client->_fd);
}
void server::add_connection()
{
	std::cout<<"new connection"<<std::endl;
	int newfd  = accept(_socketfd, &(_sock_addr), &(_socklen));
	_connections[newfd] = new client();
	_connections[newfd]->_fd = newfd; 
	EV_SET(&_changelist, newfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kqueue error when adding new connection "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::receive_message()
{
  char *buffer = (char *)malloc(sizeof(char) * (size_t)(_eventlist.data));
	std::cout<<"received message on fd : "<<_eventlist.ident<<std::endl;
  recv(_eventlist.ident, buffer,(size_t)(_eventlist.data), 0);
	std::cout<<buffer<<std::endl;
  free(buffer);
}

void server::send_message()
{
	std::cout<<"sent message on fd : "<<_eventlist.ident<<std::endl;
}

void server::run()
{
	int nbr_event;
	while (1)
	{
		nbr_event = 0;
		nbr_event = kevent(_kq, NULL, 0, &_eventlist, 2, &_timeout);
		if (nbr_event == -1)
		{
			std::cerr<<"kqueue error durring runtime"<<std::endl<<"error: "<<strerror(errno)<<std::endl;
			safe_shutdown(EXIT_FAILURE);
		}
		for (int i = 0; i < nbr_event; i++)
		{
			if (_eventlist.ident == (uintptr_t)_socketfd)
				this->add_connection();
			else if (_eventlist.filter & EVFILT_READ)
				this->receive_message();
			else if (_eventlist.filter & EVFILT_WRITE)
				this->send_message();
      // ?adding closing connection handler?
		}
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
		std::cerr<<"kevent error durring server startup "<<std::endl
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
