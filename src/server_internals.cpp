#include "irc.hpp"

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

// read the standard output and process commands only quits
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
  if (std::string(buffer).compare("QUIT\n") == 0)
  {
	  free(buffer); 
    safe_shutdown(EXIT_SUCCESS);
  }
	free(buffer);
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
  //possiblty replaced by comand::ping(client) or something
	client *tmp = _connections[_eventlist.ident];
	std::cout<<"fd timer "<<tmp->_fd<<std::endl;//test
	if (tmp->_ping && _eventlist.filter == EVFILT_TIMER)
	{
		close_connection(tmp);
		return;
	}
	//tmp->_ping = 1;
	//tmp->send_buffer<TODO prefix<<" PING :"<<_servername<<"\r\n";
	//write_set(tmp->_fd);
	update_timer(tmp->_fd, CLIENT_TTL);
}

void parse(struct server *s, struct client *c)
{
  unsigned long pos;
  while ((pos = c->_receive_buffer.find('\n')) != std::string::npos)
  {
    Message msg = Message(c->_receive_buffer, s->_cmds);
    c->_receive_buffer.erase(0, pos + 1);
    switch (msg.getCommand())
    {
      // case KICK: s->kill();
      // case TOPIC: s->topic();
      // case MODE: s->mode();
      // case INVITE: s->invite();
      // case PRIVMSG: s->privmsg();
      // case JOIN: s->join();
      // case PING: s->ping();
      // case NICK: s->nick();
      // case BAN: s->ban();
      default:
        break;
    }
  }
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
	_connections[_eventlist.ident]->_receive_buffer.append(buffer, _eventlist.data);
	std::cout<<buffer<<std::endl;//test
	free(buffer);
	parse(this, _connections[_eventlist.ident]);
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
	_connections[newfd] = new(std::nothrow) client();
	if (_connections[newfd] == NULL)
	{
		std::cerr<<"memory error when adding new connection "<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
	_connections[newfd]->_fd = newfd;
	_connections[newfd]->_ping = 0;
	read_set(newfd);
	write_unset(newfd);
	update_timer(newfd, CLIENT_TTL);
}
