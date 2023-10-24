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
#define REG_TASK_TIMER 5000

struct channel
{
	std::string _chan_id; //maybe use another type ?
	char _prefix;
	struct client *_creator;
	std::vector<struct client *> _members;
	std::vector<struct client *> _operators;
	std::string _mode;
	std::string _name;
	std::string _topic;
};

struct client
{
	//networking
	int			_fd;
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
	void			server_admin();
	void			add_connection();
	void      regular_tasks();
	void			safe_shutdown(int exit_code);
	void			close_connection(client *client);
	//kevent manip
	void			update_timer(int fd, size_t time);
	void 			delete_timer(int fd);
	void			write_unset(int fd);
	void			write_set(int fd);
	void			read_unset(int fd);
	void			read_set(int fd);
	//actions
	void			send_message();
	void			receive_message();
	void			ping();
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

void server::server_admin()
{
  std::cout<<(size_t)_eventlist.data<<" bytes received on fd : "<<_eventlist.ident<<std::endl;//test
	if ((size_t)_eventlist.data == 0)
    safe_shutdown(EXIT_SUCCESS);
	char *buffer = (char *)malloc(sizeof(char) * ((size_t)_eventlist.data) + 1);
  buffer[(size_t)_eventlist.data] = 0;
	if (buffer == NULL)
	{
		std::cerr<<"malloc error"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	size_t nbyte = read(_eventlist.ident, buffer, (size_t)_eventlist.data);
	if (nbyte < 0) // EAGAIN should have been used here but the subject does not allow for it 
	{
		free(buffer);
		safe_shutdown(EXIT_FAILURE);
	}
  if (std::string(buffer).compare("QUIT") == 0)
  {
	  free(buffer); 
    safe_shutdown(EXIT_SUCCESS);
  }
	free(buffer);
}
void	server::write_unset(int fd)
{
	EV_SET(&(_changelist), fd, EVFILT_WRITE, EV_ADD | EV_DISABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error write_unset"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

void	server::write_set(int fd)
{
	EV_SET(&(_changelist), fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error write_set"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

void	server::read_unset(int fd)
{
	EV_SET(&(_changelist), fd, EVFILT_READ, EV_ADD | EV_DISABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error read_unset"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

void	server::read_set(int fd)
{
	EV_SET(&(_changelist), fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error read_set"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

void server::update_timer(int fd, size_t time)
{
	EV_SET(&_changelist, fd, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT, 0, time, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when resetting timer"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::delete_timer(int fd)
{
	EV_SET(&_changelist, fd, EVFILT_TIMER, EV_DELETE , 0, 0, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when deleting timer"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::close_connection(client *client)
{
	close(client->_fd);
	delete_timer(client->_fd);
	_connections.erase(client->_fd);
	//_nick_map.erase(client->_nickname);
	//TODO remove from channels and nick cleaning when implemented
	delete client;
}

void server::regular_tasks()
{
	std::cout<<"regular tasks"<<std::endl;//test
	//TODO add nick cleaning 
}

void server::ping()
{
	client *tmp = _connections[_eventlist.ident];
	std::cout<<"fd timer "<<tmp->_fd<<std::endl;//test
	if (tmp->_ping && _eventlist.filter == EVFILT_TIMER)
	{
		close_connection(tmp);
		return;
	}
	// send ping msg;
	//tmp->_ping = 1;
	update_timer(tmp->_fd, CLIENT_TTL);
}

void server::receive_message()
{
	std::cout<<(size_t)_eventlist.data<<" bytes received on fd : "<<_eventlist.ident<<std::endl;//test
	if ((size_t)_eventlist.data == 0)
	{
		if ((_eventlist.flags & EV_EOF) == EV_EOF)
			close_connection(_connections[(size_t)_eventlist.ident]);
		return;
	}
	char *buffer = (char *)malloc(sizeof(char) * (size_t)_eventlist.data);
	if (buffer == NULL)
	{
		std::cerr<<"malloc error"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	size_t nbyte = recv(_eventlist.ident, buffer, (size_t)_eventlist.data, 0);
	if (nbyte < 0) // EAGAIN should have been used here but the subject does not allow for it 
	{
		free(buffer);
		return;
	}
	//	_connections[_eventlist.ident]->_receive_buffer.append(buffer, _eventlist.data);
	std::cout<<buffer<<std::endl;//test
	free(buffer);
	//	parse()//TODO
}

void server::send_message()
{
	client *tmp = _connections[_eventlist.ident];
	std::cout<<"sent message on fd : "<<tmp->_fd<<std::endl;//test
	int nbyte = send(tmp->_fd, tmp->_send_buffer.c_str(), tmp->_send_buffer.size(), 0);
	if (nbyte < 0) // EAGAIN should have been used here but the subject does not allow for it 
		return;
	tmp->_send_buffer.erase(0, nbyte);
	if (tmp->_send_buffer.empty())
		write_unset(tmp->_fd);
	update_timer(tmp->_fd, CLIENT_TTL);
}

// add a client (not a user yet)
void server::add_connection()
{
	std::cout<<"new connection"<<std::endl;//test
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
	read_set(newfd);
	write_unset(newfd);
	update_timer(newfd, CLIENT_TTL);
}

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
				this->ping();	
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
	if ((_kq = kqueue()) == -1)//create the kqueue
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
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
