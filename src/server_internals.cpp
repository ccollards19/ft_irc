#include "irc.hpp"
#include <cstdlib>
#include <vector>

//free the ressources and exit
void server::safe_shutdown(int exit_code) {
	std::cout << "||||||||||SHUTDOWN||||||||||" << std::endl;
	if (_socketfd)
		close(_socketfd);
	if (_kq)
		close(_kq);
	std::map<int, client *>::iterator end = _connections.end();
	for (std::map<int, client *>::iterator it = _connections.begin(); it != end; it++) {
		close(it->second->_fd);
    std::cout << "close connection on fd : " << it->second->_fd << std::endl;
		delete (it->second);
	}
  std::vector<channel *>::iterator chan_end = _chan_list.end();
	for (std::vector<channel *>::iterator it = _chan_list.begin(); it != chan_end; it++) {
    std::cout << "close channel : " << (*it)->_name << std::endl;
		delete (*it);
	}
  if (_res_start)
    freeaddrinfo(_res_start);
  system("leaks ircserv");
	exit(exit_code);
}

// read the standard output and process commands only quits
void server::server_admin() 
{
	if (DEBUG)
	{
		std::cout << "||||||||||SERVADMN||||||||||" << std::endl;
		std::cout << (size_t) _eventlist.data << " bytes received on fd : ["<< _eventlist.ident << "]\n";//test
	}
	if ((size_t) _eventlist.data == 0)
		safe_shutdown(EXIT_SUCCESS);
	char *buffer = (char *) malloc(
			sizeof(char) * ((size_t) _eventlist.data) + 1);
	buffer[(size_t) _eventlist.data] = 0;
	if (buffer == NULL) {
		std::cerr << "malloc error" << std::endl
				  << "error: " << strerror(errno) << std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	size_t nbyte = read(_eventlist.ident, buffer, (size_t) _eventlist.data);
	if (nbyte < 0) // EAGAIN should have been used here but the subject does not allow for it
	{
		free(buffer);
		safe_shutdown(EXIT_FAILURE);
	}
	if (std::string(buffer).compare("QUIT\n") == 0) {
		free(buffer);
		safe_shutdown(EXIT_SUCCESS);
	}
	free(buffer);
}

void server::regular_tasks() 
{
	//std::cout<<"regular tasks"<<std::endl;//test
	//TODO add nick cleaning 
}

void server::receive_message()
{

	if ((size_t) _eventlist.data == 0) {
		if ((_eventlist.flags & EV_EOF) == EV_EOF)
			close_connection(_connections[(size_t) _eventlist.ident]);
		return;
	}
	char *buffer = (char *) malloc(sizeof(char) * (size_t) _eventlist.data);
	if (buffer == NULL) {
		std::cerr << "malloc error" << std::endl
				  << "error: " << strerror(errno) << std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	size_t nbyte = recv(_eventlist.ident, buffer, (size_t) _eventlist.data, 0);
	if (nbyte < 0) {
		free(buffer);
		return;
	}
	client *client = _connections[_eventlist.ident];
	update_timer(client->_fd, CLIENT_TTL);
	client->_receive_buffer.append(buffer, _eventlist.data);
	if (DEBUG){
		std::cout << "||||||||||RECVDATA||||||||||" << std::endl;
		std::cout << (size_t) _eventlist.data << " bytes received on fd : ["<< _eventlist.ident << "]\n";//test
		std::cout << "[" << client->_receive_buffer << "]"<< std::endl;
	}//test
	free(buffer);
	parse(this, client);
}

void server::send_message() 
{
	client *tmp = _connections[_eventlist.ident];
	if (DEBUG)
	{
		std::cout << "||||||||||SENDDATA||||||||||" << std::endl;
		std::cout << "sent message on fd : " << tmp->_fd << std::endl;//test
		std::cout << "[" << tmp->_send_buffer << "]" << std::endl;//test
	}
	int nbyte = send(tmp->_fd, tmp->_send_buffer.c_str(),
					 tmp->_send_buffer.size(), 0);
	if (nbyte <
		0) // EAGAIN should have been used here but the subject does not allow for it
		return;
	tmp->_send_buffer.erase(0, nbyte);
	if (tmp->_send_buffer.empty())
		write_unset(tmp->_fd);
	update_timer(tmp->_fd, CLIENT_TTL);
}

// add a client (not a user yet)
void server::add_connection() 
{
	if (DEBUG)
		std::cout << "||||||||||NEWCNCTN||||||||||" << std::endl;
	client *new_client = new(std::nothrow) client();
	if (new_client == NULL) {
		std::cerr << "memory error when adding new connection " << std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	int newfd = accept(_socketfd, &(new_client->_sockaddr), &(new_client->_socklen));
	if (newfd == -1) {
		std::cerr << "socket error when adding new connection " << std::endl << "error: " << strerror(errno) << std::endl;
		delete new_client;
		safe_shutdown(EXIT_FAILURE);
	}
	_connections[newfd] = new_client;
	new_client->_fd = newfd;
	new_client->_ping = 0;
	new_client->_isRegistered = 0;
	new_client->_hostname = _servername;
	read_set(newfd);
	write_unset(newfd);
	update_timer(newfd, CLIENT_TTL);
}

void server::check_connection(struct client *c) 
{
	if (DEBUG)
	{
		std::cout << "||||||||||CHECKCNT||||||||||" << std::endl;
		std::cout<<"timer on fd : "<<c->_fd<<std::endl;//test
	}
	if (c->_ping) {
		Message msg("", _cmds);
		quit(msg, c);
	}
	else {
		c->_ping = 1;
		send_reply( *this , *c, "PING :" + _servername);
		update_timer(c->_fd, CLIENT_TTL);
	}
}

void server::close_connection(client *client)
{
	if (DEBUG)
	{
		std::cout << "||||||||||CLOSECNT||||||||||" << std::endl;
		std::cout<<"close connection on fd : "<<client->_fd<<std::endl;//test
	}
	_connections.erase(client->_fd);
	close(client->_fd);
	if (client->_isRegistered) {
		std::vector<channel *>::iterator end = _chan_list.end();
		for (std::vector<channel *>::iterator it = _chan_list.begin(); it != end; it++) {
			removeMember(client, *it);
			if ((*it)->_members.empty())
				removeChannel(*it);
		}
		_nick_map.erase(client->_nickname);
	}
	delete_timer(client->_fd);
	delete client;
}

// event loop using kevents
void server::run()
{
	int nbr_event;
	while (1) {
		nbr_event = 0;
		nbr_event = kevent(_kq, NULL, 0, &_eventlist, 1, NULL);// &_timeout);
		if (nbr_event == -1) {
			std::cerr<<"kqueue error durring runtime"<<std::endl<<"error: "<<strerror(errno)<<std::endl;
			safe_shutdown(EXIT_FAILURE);
		}
		else if (nbr_event == 0)
			continue;
		else if ( _eventlist.ident == STDIN_FILENO)// events on standard input
			server_admin();
		else if ( _eventlist.ident == (uintptr_t)_socketfd) { // events on server socket 
			if (_eventlist.filter == EVFILT_READ)
				add_connection();
			else if (_eventlist.filter == EVFILT_TIMER)
				regular_tasks();	
		}
		else { // events on a client socket
			if (_eventlist.filter == EVFILT_WRITE)
				send_message();
			else if (_eventlist.filter == EVFILT_READ)
				receive_message();
			else if (_eventlist.filter == EVFILT_TIMER)
				check_connection(_connections[_eventlist.ident]);
		}
	}
}
