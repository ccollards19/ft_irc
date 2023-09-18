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


struct user
{
	int perm;
};

struct server
{
	struct sockaddr		sock_addr;
	socklen_t		socklen;
	int			socketfd;
	struct kevent		changelist;
	struct kevent		eventlist[2];
	int 			kq;
	struct timespec 	timeout;

	std::map<int, user>	connections;

	void			add_connection();
};

//to be expanded as needed
void safe_shutdown(server *server, int exit_code)
{	
	if (server->socketfd) 
		close(server->socketfd);
	if (server->kq) 
		close(server->kq);
	exit(exit_code);
}

void server::add_connection()
{
	std::cout<<"new connection"<<std::endl;
	int newfd  = accept(this->socketfd, &(this->sock_addr), &(this->socklen));
	(this->connections)[newfd] =;
	EV_SET(&(this->changelist), newfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(this->kq, &(this->changelist), 1, NULL, 0, &(this->timeout)) == -1)
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(this, EXIT_FAILURE);
	}
}

void receive_message(server *server)
{
	std::cout<<"received message on fd : "<<server->eventlist[0].ident<<std::endl;
}

void send_message(server *server)
{
	std::cout<<"sent message on fd : "<<server->eventlist[0].ident<<std::endl;
}

void run_server(server *server)
{
	int nbr_event;
	while (1)
	{
		nbr_event = 0;
		nbr_event = kevent(server->kq, NULL, 0, reinterpret_cast<struct kevent *>(server->eventlist), 2, &(server->timeout));
		if (nbr_event == -1)
		{
			std::cerr<<"kqueue error durring runtime"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
			safe_shutdown(server, EXIT_FAILURE);
		}
		for (int i = 0; i < nbr_event; i++)
		{
			if (server->eventlist[i].ident == (uintptr_t)server->socketfd)
				server->add_connection();
			else if (server->eventlist[i].filter & EVFILT_READ)
				receive_message(server);
			else if (server->eventlist[i].filter & EVFILT_WRITE)
				send_message(server);
		}
	}
}

void init_server(server *server, char **argv)
{
	//define the "name" assigned to the server socket
	struct sockaddr_in tmp; 
	tmp.sin_family = AF_INET;
	tmp.sin_port = htons(atoi(argv[1]));
	tmp.sin_addr.s_addr = INADDR_ANY;
	memcpy(&server->sock_addr, &tmp, sizeof(struct sockaddr));//fucking hacky code, fucking c++ and its fucking types
	server->socklen = sizeof(struct sockaddr_in);
	//create the server socket and bind it
	if (((server->socketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)\
	|| (fcntl(server->socketfd, F_SETFL, O_NONBLOCK) == -1)\
	|| (bind(server->socketfd, &(server->sock_addr), sizeof(server->sock_addr)) == -1)\
	|| (listen(server->socketfd, 1) == -1))// TODO add macro for backlog
	{
		std::cerr<<"socket error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(server, EXIT_FAILURE);
	}
	//create the kqueue
	if ((server->kq = kqueue()) == -1)
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(server, EXIT_FAILURE);
	}
	EV_SET(&(server->changelist), server->socketfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(server->kq, &(server->changelist), 1, NULL, 0, &(server->timeout)) == -1)
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(server, EXIT_FAILURE);
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
	memset(&server, 0, sizeof(server));
	init_server(&server, argv);
	run_server(&server);
	safe_shutdown(&server, EXIT_SUCCESS);
}
