#include <sys/socket.h>
#include <unistd.h>
#include <iostream>


struct server
{
	struct sockaddr_in	sockaddr;
	int			socketfd;
	struct kevent		*changelist;
	struct kevent		eventlist[1];
	int 			kq;
	struct timespec 	timeout = NULL;
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
			safe_exit(server, EXIT_FAILURE);
		}
		for (int i = 0; i < nbr_event; i++)
		{
			if (server->eventlist[i].filter == )
		}

	}
}

void init_server(server *server, char **argv)
{
	//define the "name" assigned to the server socket 
	(server->sockaddr).sin_family = AF_INET;
	(server->sockaddr).sin_port = htons(atoi(argv[1]));
	(server->sockaddr).sin_addr.s_addr = INADDR_ANY;
	//create the server socket and bind it
	if ((server->socketfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	|| (fcntl(server->socketfd, F_SETFL, O_NONBLOCK) == -1)
	|| (bind(server->socketfd, server->sockaddr, sizeof(server->sockadrr)) == -1)
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
