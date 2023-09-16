#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>


struct server
{
	struct sockaddr_in	sock_addr;
	socklen_t		socklen;
	int			socketfd;
	struct kevent		changelist[2];
	struct kevent		eventlist[2];
	int 			kq;
	struct timespec 	timeout;
	void			add_connection();
};

void server::add_connection()
{
	int newfd  = accept(this->socketfd, &(this->sock_addr), &(server->socklen));
	EV_SET(&((this->eventlist)[1]), newfd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
}

//to be expanded as needed
void safe_shutdown(server *server, int exit_code)
{	
	if (server->socketfd) 
		close(server->socketfd);
	if (server->kq) 
		close(server->kq);
	exit(exit_code);
}

void handle_message(server *server)
{
	std::cout<<"received message on fd : "<<server->eventlist[1].ident<<std::endl;
}

void run_server(server *server)
{
	int nbr_event;
	while (1)
	{
		nbr_event = kevent(server->kq, &(server->changelist), 0, &(server->eventlist), 1, &(server->timeout));
		if (nbr_event == -1)
		{
			std::cerr<<"kqueue error durring runtime"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
			safe_shutdown(server, EXIT_FAILURE);
		}
		for (int i = 0; i < nbr_event; i++)
		{
			if (server->eventlist[i].ident == server->socketfd && server->eventlist[i].filter ==  EVFILT_READ)
				server->add_connection();
			else if (server->eventlist[i].filter == EVFILT_READ)
				handle_message(server);
		}
	}
}

void init_server(server *server, char **argv)
{
	//define the "name" assigned to the server socket 
	(server->sock_addr).sin_family = AF_INET;
	(server->sock_addr).sin_port = htons(atoi(argv[1]));
	(server->sock_addr).sin_addr.s_addr = INADDR_ANY;
	server->socklen = sizeof(sock_addr);
	//create the server socket and bind it
	if ((server->socketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)\
	|| (fcntl(server->socketfd, F_SETFL, O_NONBLOCK) == -1)\
	|| (bind(server->socketfd, server->sock_addr, sizeof(server->sock_addr)) == -1)\
	|| (listen(server->socketfd, 1000) == -1)// TODO add macro for backlog
	{
		std::cerr<<"socket error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_exit(server, EXIT_FAILURE);
	}
	//create the kqueue
	if ((server->kq = kqueue()) == -1)
	{
		std::cerr<<"kqueue error durring server startup "<<std::endl
		<<"error: "<<strerror(errno)<<std::endl;
		safe_exit(server, EXIT_FAILURE);
	}
	EV_SET(&((server->eventlist)[0]), server->socketfd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
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
