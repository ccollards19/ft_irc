#include "irc.hpp"
#include <iostream>
#include <iterator>
#include <map>
#include <netdb.h>

//to be expanded as needed
void server::safe_shutdown(int exit_code) {
	std::cout << "||||||||||SHUTDOWN||||||||||" << std::endl;
	if (_socketfd)
		close(_socketfd);
	if (_kq)
		close(_kq);
	std::map<int, client *>::iterator end = _connections.end();
	for (std::map<int, client *>::iterator it = _connections.begin(); it != end; it++) {
		close(it->second->_fd);
    std::cout << "close" << it->second->_fd << std::endl;
		delete (it->second);
	}
  if (_res_start)
    freeaddrinfo(_res_start);
	exit(exit_code);
}

// read the standard output and process commands only quits
void server::server_admin() {
	std::cout << "||||||||||SERVADMN||||||||||" << std::endl;
	std::cout << (size_t) _eventlist.data << " bytes received on fd : ["
			  << _eventlist.ident << "]\n";//test
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
	if (nbyte <
		0) // EAGAIN should have been used here but the subject does not allow for it
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

void server::check_connection(struct client *c) {
	(void) c;
	/*std::cout<<"fd timer "<<c->_fd<<std::endl;//test
	if (c->_ping)
	  close_connection(c);
	else
	{
	  c->_send_buffer.append("PING\n");
	  c->_ping = 1;
	  write_set(c->_fd);
	  update_timer(c->_fd, CLIENT_TTL);
	}*/
}

void server::close_connection(client *client) {
	close(client->_fd);
	_connections.erase(client->_fd);
	//_nick_map.erase(client->_nickname);
	//TODO remove from channels and nick cleaning when implemented
	delete client;
}

void server::regular_tasks() {
	//std::cout<<"regular tasks"<<std::endl;//test
	//TODO add nick cleaning 
}


void parse(struct server *s, struct client *c) {
	unsigned long pos;
	while ((pos = c->_receive_buffer.find('\n')) != std::string::npos) {
		// std::cout << "PARSING: " + c->_receive_buffer << "\n";
		Message msg((c->_receive_buffer).substr(0, pos + 1), s->_cmds);
		// std::cout << "ERASE: ["  << c->_receive_buffer << "]\n";
		c->_receive_buffer.erase(0, (pos + 1));
		// std::cout << "ERASE: ["  << c->_receive_buffer << "]\n";
		// std::cout << "command : [" << msg.getCommand() << "] "<< msg.getCommandName() << "\n";
		// msg.showContent();
		switch (msg.getCommand()) {
			case KICK:s->kill(msg, c);break;
			case TOPIC:s->topic(msg, c);break;
			case MODE:s->mode(msg, c);break;
				//case INVITE: s->invite(msg, c);break;
			case PRIVMSG: s->privmsg(msg, c);break;
			case JOIN:s->join(msg, c);break;
			case PING:s->ping(msg, c);break;
			case PONG:s->pong(msg, c);break;
			case NICK:s->nick(msg, c);break;
				//case BAN: s->ban(msg, c);break;
			case USER:s->user(msg, c);break;
			case PART:s->part(msg, c);break;
			default:break;
		}
	}
}

void server::receive_message() {
	std::cout << "||||||||||RECVDATA||||||||||" << std::endl;
	std::cout << (size_t) _eventlist.data << " bytes received on fd : ["
			  << _eventlist.ident << "]";//test
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
	if (nbyte <
		0) // EAGAIN should have been used here but the subject does not allow for it
	{
		free(buffer);
		return;
	}
	_connections[_eventlist.ident]->_receive_buffer.append(buffer,
														   _eventlist.data);
	std::cout << "[" << buffer  << "]"<< std::endl;//test
	free(buffer);
	parse(this, _connections[_eventlist.ident]);
}

void server::send_message() {
	client *tmp = _connections[_eventlist.ident];
	std::cout << "||||||||||SENDDATA||||||||||" << std::endl;
	std::cout << "sent message on fd : " << tmp->_fd << std::endl;//test
	std::cout << "[" + tmp->_send_buffer + "]" << std::endl;//test
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
void server::add_connection() {
	std::cout << "||||||||||NEWCNCTN||||||||||" << std::endl;
	std::cout << "new connection" << std::endl;//test
  client *new_client = new(std::nothrow) client();
	if (new_client == NULL) {
		std::cerr << "memory error when adding new connection " << std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	int newfd = accept(_socketfd, &(new_client->_sockaddr), &(new_client->_socklen));
	if (newfd == -1) {
		std::cerr << "socket error when adding new connection " << std::endl
				  << "error: " << strerror(errno) << std::endl;
    delete new_client;
		safe_shutdown(EXIT_FAILURE);
	}
	_connections[newfd] = new_client;
	new_client->_fd = newfd;
	new_client->_ping = 0;
	new_client->_isRegistered = 0;
	read_set(newfd);
	write_unset(newfd);
	update_timer(newfd, CLIENT_TTL);
}
