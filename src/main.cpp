#include "irc.hpp"

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
        check_connection(_connections[_eventlist.ident]);
		}
	}
}

void server::init(char **argv)
{
	_password.append(argv[2]);
	std::time_t tmp_time = std::time(nullptr);
	_creation_date.append(std::asctime(std::localtime(&tmp_time)));
	_cmds["KICK"] = KICK;
	_cmds["TOPIC"] = TOPIC;
	_cmds["MODE"] = MODE;
	_cmds["INVITE"] = INVITE;
	_cmds["NICK"] = NICK;
	_cmds["PRIVMSG"] = PRIVMSG;
	_cmds["JOIN"] = JOIN;
	_cmds["PING"] = PING;
	_cmds["USER"] = USER;
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
	if (argc != 3)
	{
		std::cerr<<"usage: ./ft_irc [port] [password]"<<std::endl;
		return (EXIT_FAILURE);
	}
	server server;
	server.init(argv);
	server.run();
	server.safe_shutdown(EXIT_SUCCESS);
}
