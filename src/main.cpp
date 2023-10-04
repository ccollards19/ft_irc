#include <cstddef>
#include <string>
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
#include <unistd.h>
#include <iostream>
#include <map>
#include <vector>


struct channel
{
// The channel identifier is a function of the time.  The current time
//    (as defined under UNIX by the number of seconds elapsed since
//    00:00:00 GMT, January 1, 1970) is converted in a string of five (5)
//    characters using the following base:
//    "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890" (each character has a decimal
//    value starting from 0 for 'A' to 35 for '0').

//    The channel identifier therefore has a periodicity of 36^5 seconds
//    (about 700 days).
  std::string chan_id;
  char prefix;
  struct user *creator;
  std::vector<struct user *> members;
  std::vector<struct user *> operators;
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
  std::string mode;
};

struct user
{
	std::string mode;
};

struct server
{
  std::string server_id;
	struct sockaddr		_sock_addr;
	socklen_t		_socklen;
	int			_socketfd;

	struct kevent		_changelist;
	struct kevent		_eventlist[2];
	struct timespec 	_timeout;
	int 			_kq;

	std::map<int, user*>	_connections;

	void			init(char **argv);
	void			run();
	void			add_connection();
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

void server::add_connection()
{
	std::cout<<"new connection"<<std::endl;
	int newfd  = accept(_socketfd, &(_sock_addr), &(_socklen));
	_connections[newfd] = new user();
	EV_SET(&_changelist, newfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::receive_message()
{
  char *buffer = (char *)malloc(sizeof(char) * (size_t)(_eventlist[0].data));
	std::cout<<"received message on fd : "<<_eventlist[0].ident<<std::endl;
  recv(_eventlist[0].ident, buffer,(size_t)(_eventlist[0].data), 0);
	std::cout<<buffer<<std::endl;
  free(buffer);
}

void server::send_message()
{
	std::cout<<"sent message on fd : "<<_eventlist[0].ident<<std::endl;
}

void server::run()
{
	int nbr_event;
	while (1)
	{
		nbr_event = 0;
		nbr_event = kevent(_kq, NULL, 0, reinterpret_cast<struct kevent *>(_eventlist), 2, &(_timeout));
		if (nbr_event == -1)
		{
			std::cerr<<"kqueue error durring runtime"<<std::endl<<"error: "<<strerror(errno)<<std::endl;
			safe_shutdown(EXIT_FAILURE);
		}
		for (int i = 0; i < nbr_event; i++)
		{
			if (_eventlist[i].ident == (uintptr_t)_socketfd)
				this->add_connection();
			else if (_eventlist[i].filter & EVFILT_READ)
				this->receive_message();
			else if (_eventlist[i].filter & EVFILT_WRITE)
				this->send_message();
		}
	}
}

void server::init(char **argv)
{
	memset(this, 0, sizeof(server));
	std::map<int, user *> newmap;
	_connections = newmap; 
	//define the "name" assigned to the server socket
	struct sockaddr_in tmp; 
	tmp.sin_family = AF_INET;
	tmp.sin_port = htons(atoi(argv[1]));
	tmp.sin_addr.s_addr = INADDR_ANY;
	memcpy(&_sock_addr, &tmp, sizeof(struct sockaddr));
	//fucking hacky code, fucking c++ and its fucking types
	_socklen = sizeof(struct sockaddr_in);
	//create the server socket and bind it
	if (((_socketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)\
	|| (fcntl(_socketfd, F_SETFL, O_NONBLOCK) == -1)\
	|| (bind(_socketfd, &(_sock_addr), sizeof(_sock_addr)) == -1)\
	|| (listen(_socketfd, 1) == -1))// TODO add macro for backlog
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
	EV_SET(&(_changelist), _socketfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
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
