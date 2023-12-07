#include "irc.hpp"
#include <algorithm>
#include <iostream>
#include <iterator>

const std::string& client::getNickname() const
{
	return (_nickname);
}

//                 PART
                 
void server::part(Message &m, struct client *client)
{
	std::vector<std::string> params = m.getContent();
	if (params.size() < 1)
	{
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
		return ;
	}
	std::string channel_name = params[0];
	if (!checkChannel(channel_name))
	{
		reply(m, *this, *client, ERR_NOSUCHCHANNEL);
		return ;
	}
	struct channel *chan = *getChannel(channel_name);
	if (!client->isMember(chan))
	{
		reply(m, *this, *client, ERR_NOTONCHANNEL);
		return ;
	}
	if (params.size() == 1)
	{
		for (std::vector<struct client *>::iterator it = chan->_members.begin(); it != chan->_members.end() ; ++it) {
			std::cout << "sending message to " << (*it)->getNickname() << std::endl;
			send_reply(*this, **it, ":" + client->_nickname + "!" + client->_username + "@" + _servername + " PART " + chan->_name);
		}
	}
	else
	{
		std::string part_msg = params[1];
		for (std::vector<struct client *>::iterator it = chan->_members.begin(); it != chan->_members.end() ; ++it) {
			std::cout << "sending message to " << (*it)->getNickname() << std::endl;
			send_reply(*this, **it, ":" + client->_nickname + "!" + client->_username + "@" + _servername + " PART " + chan->_name + " :" + part_msg);
		}
	}
	chan->_members.erase(std::find(chan->_members.begin(), chan->_members.end(), client));
}

void server::register_client(Message &m, struct client *client)
{
  std::cout<< client->_pass << ":" << client->_nickname << ':' <<client->_username <<"\n";
  if (!client->_pass || client->_nickname.empty() || client->_username.empty())
    return;
	reply(m, *this, *client, RPL_WELCOME);
	reply(m, *this, *client, RPL_YOURHOST);
	reply(m, *this, *client, RPL_CREATED);
	reply(m, *this, *client, RPL_MYINFO);
  client->_isRegistered = true;
}

//                 PASS

void server::pass(Message &m, struct client *client) {
  std::vector<std::string> params = m.getContent();
  std::cout<<params[0] + "=" + _password <<"\n";
  if (params.size() < 1)
    reply(m, *this, *client, ERR_NEEDMOREPARAMS);
  else if (client->_isRegistered)
    reply(m, *this, *client, ERR_ALREADYREGISTRED);
  else {
    std::cout<<params[0] + "=" + _password <<"\n";
    if (params[0].compare(_password) == 0)
      client->_pass = true;
    else
      client->_pass = false;
  }
}

//                  NICK

void server::nick(Message &m, struct client *client){
	std::vector<std::string> params = m.getContent();
	if (params.size() == 0)
	{
		reply(m, *this, *client, ERR_NONICKNAMEGIVEN);
		return ;
	}
	if (params.size() > 1)
	{
		reply(m, *this, *client, ERR_ERRONEUSNICKNAME);
		return ;
	}
	if (_nick_map.find(params[0]) != _nick_map.end()) {
		reply(m, *this, *client, ERR_NICKNAMEINUSE);
		return ;
	}
	bool hasNick = false;
	std::map<std::string, struct client*>::iterator i;
	if (!_nick_map.empty())
	{
		i = _nick_map.begin();
		std::cout << "debug\n";
		for ( ; i != _nick_map.end() ; ++i) {
			std::cout << "nick: "<< i->second->getNickname() << "\n";
			if (client == i->second)
			{
				hasNick = true;
				break ;
			}
			std::cout << "loop\n";
		}
	}
	std::cout << "debug 2\n";
	if (hasNick)
	{
		send_reply(*this, *client, ":" + i->second->_nickname + "!" + i->second->_username + "@" + _servername + " NICK :" + params[0]);
		_nick_map.erase(i);
	}
	std::cout << "debug 3\n";
	_nick_map[params[0]] = client;
	client->_nickname = params[0];
	register_client(m, client);
}

//                  USER

void server::user(Message &m, client *client){
	std::vector<std::string> params = m.getContent();
	if (params.size() < 4) {
    std::cout << "test\n";
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
    return;
  }
	if (client->_isRegistered) {
		reply(m, *this, *client, ERR_ALREADYREGISTRED);
    return;
  }
	client->_username = params[0];
  //TODO set mode
	client->_realname = params[3];
	client->_hostname = params[3];
  register_client(m, client);
}


//                  PING
void server::ping(Message &m, struct client *client)
{
	std::vector<std::string> params = m.getContent();
  if (params.size() > 1)
    reply(m, *this, *client, ERR_NOSUCHSERVER);
  else
	  send_reply(*this, *client, ":"+ _servername +" PONG \n");
	update_timer(client->_fd, CLIENT_TTL);
}

//                 PONG 
void server::pong(Message &m, struct client *client)
{
	std::vector<std::string> params = m.getContent();
  if (params.size() > 1)
    reply(m, *this, *client, ERR_NOSUCHSERVER);
  else
    client->_ping = 0;
  update_timer(client->_fd, CLIENT_TTL);
}

//                  OPER

void server::oper(Message &m, client *client) {
	std::vector<std::string> params = m.getContent();
	if (params.size() < 3)
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
	std::string password = params.at(2);
	if (_oper_pwd != password)
		reply(m, *this, *client, ERR_PASSWDMISMATCH);
	else {
		client->_is_oper = true;
		reply(m, *this, *client, RPL_YOUREOPER);
	}
}

//                  KILL

void server::kill(Message &m, client *client) {
	std::vector<std::string> params = m.getContent();
	int nosuchnick = 0;
	if (!client->_is_oper)
		reply(m, *this, *client, ERR_NOPRIVILEGES);
	if (params.size() < 2)
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
	try{_nick_map[params[1]]->getNickname();}
	catch (std::exception &e){nosuchnick = 1;}
	if (nosuchnick)
		reply(m, *this, *client, ERR_NOSUCHNICK);
	else
		server::close_connection(_nick_map[params[1]]);
}

//                  MODE
bool server::isAchannel(std::string name)
{
	std::vector<channel *>::iterator i = _chan_list.begin();
	for ( ;  i != _chan_list.end() && (*i)->_name != name && i != _chan_list.end() ; ++i) {}
	return (i != _chan_list.end());
}

bool client::isChanop(channel *c) {
	std::vector<client *>::iterator i = c->_operators.begin();
	for ( ;  (*i) != this && i != c->_operators.end() ; i++) {}
	return (i != c->_operators.end());
}

bool client::isMember(channel *c){
	std::vector<client *>::iterator i = c->_members.begin();
	for ( ;  (*i) != this && i != c->_members.end() ; i++) {}
	return (i != c->_members.end());
}

void server::mode(Message &m, client *client){
	std::vector<std::string> params = m.getContent();
	std::string modes("airoO");
	std::string chanModes("itkol");
	if (params[0][0] == '#')//CHANNEL MODE
	{
		channel *c;
		if (params.size() < 2)
		{
			reply(m, *this, *client, ERR_NEEDMOREPARAMS);
			return ;
		}
		if (!isAchannel(params[0]))
		{
			reply(m, *this, *client, ERR_NOSUCHCHANNEL);
			return ;
		}
		else
		{
			c = *getChannel(params[0]);
			if (!client->isChanop(c))
			{
				reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
				return ;
			}
			if (!client->isMember(c))
			{
				reply(m, *this, *client, ERR_NOSUCHNICK);
				return ;
			}
			if (params.size() > 2)
			{
				std::string mode(params[1]);
				if (mode[0] == '+')
				{
					for (size_t i = 1; i < mode.size() - 1; ++i) {
						if (chanModes.find(mode[i]))
						{
							if (!c->isModeSet(mode[i]))
								c->_mode += mode[i];
							if (mode[i] == 'i')
							{
								if (params.size() > 2)
								{
									if (_nick_map.find(params[2]) != _nick_map.end())
									{
										struct client *invited = _nick_map[params[2]];
										c->_invite_list.push_back(invited);
									}
								}
								else
								{
									reply(m, *this, *client, RPL_INVITELIST);
									reply(m, *this, *client, RPL_ENDOFINVITELIST);
									return ;
								}
							}
							if (mode[i] == 'k')
							{
								if (params.size() > 2)
								{
									if (client->isChanop(c))
										c->_pwd = params[2];
									else
									{
										reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
										return ;
									}
								}
								else
								{
									c->_mode.erase(c->_mode.find(mode[i]));
									reply(m, *this, *client, ERR_NEEDMOREPARAMS);
									return ;
								}
							}
							if (mode[i] == 'o')
							{
								if (!client->isChanop(c))
								{
									reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
									return;
								}
								else if (params.size() < 3)
								{
									reply(m, *this, *client, ERR_NEEDMOREPARAMS);
									return ;
								}
								else if (_nick_map.find(params[2]) == _nick_map.end())
								{
									reply(m, *this, *client, ERR_NOSUCHNICK);
									return ;
								}
								else if (!_nick_map[params[2]]->isMember(c))
								{
									reply(m, *this, *client, ERR_NOTONCHANNEL);
									return ;
								}
								else
									c->_operators.push_back(_nick_map[params[2]]);
							}
							if (mode[i] == 'l')
							{
								if (params.size() < 3)
								{
									reply(m, *this, *client, ERR_NEEDMOREPARAMS);
									return ;
								}
								int size = stoi(params[2]);
								if (size < 0)
									c->_capacity = 0;
								else
									c->_capacity = size;
							}
						}
					}
				}
				else if (params[1][0] == '-')
				{
					for (size_t i = 1; i < mode.size() -1; ++i) {
						if (chanModes.find(mode[i])) {
							if (c->_mode.find(mode[i]))
								c->_mode.erase(c->_mode.find(mode[i]));
							if (mode[i] == 'o')
							{
								struct client *target;
								if (params.size() < 3)
								{
									reply(m, *this, *client, ERR_NEEDMOREPARAMS);
									return ;
								}
								if (_nick_map.find(params[2]) == _nick_map.end())
								{
									reply(m, *this, *client, ERR_NOSUCHNICK);
									return ;
								}
								if (!_nick_map[params[2]]->isMember(c))
								{
									reply(m, *this, *client, ERR_NOTONCHANNEL);
									return ;
								}
								else
									target = _nick_map[params[2]];
								if (!client->isChanop(c) || target == c->_creator)
								{
									reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
									return;
								}
								else
									c->_operators.erase(std::find(c->_operators.begin(),c->_operators.end(), target));
							}
						}
					}
				}
				else
				{

				};
			}
			else
			{

			}
		}
	}
	else//USER MODE
	{
		if (params.size() < 2)
		{
			reply(m, *this, *client, ERR_NEEDMOREPARAMS);
			return ;
		}
		if (params[0] != client->getNickname())
		{
			reply(m, *this, *client, ERR_USERSDONTMATCH);
			return ;
		}
		if (modes.find(params[1][1]) == std::string::npos)
		{
			reply(m, *this, *client, ERR_UMODEUNKNOWNFLAG);
			return ;
		}
		else if (!((params[1] == "+o" || params[1] == "+O") && !client->_is_oper))
		{
			if (params[1][0] == '+' && client->_mode.find(params[1][1]) == std::string::npos)
				client->_mode += params[1][1];
			else if (params[1][0] == '-')
				client->_mode.erase(client->_mode.find(params[1][0]));
		}
		reply(m, *this, *client, RPL_UMODEIS);
	}
}

//                  Join

void	channel::removeInvited(struct client *client) {
	std::vector<struct client*>::iterator i;
	if ((i = std::find(_invite_list.begin(), _invite_list.end(), client)) != _invite_list.end())
		_invite_list.erase(i);
}

void channel::addClient(client *client) {
	_members.push_back(client);
}


bool	channel::isModeSet(char mode) {
	for (std::vector<char>::iterator it = this->_mode.begin(); it != this->_mode.end(); ++it) {
		if (*it == mode)
			return (true);
	}
	return (false);
}

std::vector<struct channel*>::iterator	server::getChannel(std::string channelName) {
	for(std::vector<struct channel*>::iterator it = _chan_list.begin(); it != _chan_list.end(); ++it) {
		if ((*it)->_name == channelName)	{
			return (it);
		}
	}
	return (_chan_list.end());
}

bool	server::checkChannel(std::string channelName){
	std::vector<channel *>::iterator it;
	for(it = server::_chan_list.begin(); it != server::_chan_list.end(); ++it) {
		if ((*it)->_name == channelName) {
			return (true);
		}
	}
	return (false);
}

struct channel *server::createChannel(std::string channelName, struct client *client){

	if (checkChannel(channelName))
		return (*getChannel(channelName));
	else
	{
		channel *chan = new channel();
		chan->_name = channelName;
		chan->_operators.push_back(client);
		chan->_creator = client;
		chan->_prefix = '#';
		chan->_chan_id = channelName;
		chan->_capacity = 0;
		_chan_list.push_back(chan);
		return _chan_list.back();
	}

	// Message à print. Pas sur de ce que c'est
}

bool channel::isInvited(client *c) {
	return (std::find(_invite_list.begin(), _invite_list.end(), c) != _invite_list.end());
}

void server::joinMessage(channel *target, client *c)
{
	for (std::vector<struct client *>::iterator it = target->_members.begin(); it != target->_members.end() ; ++it) {
		std::cout << "sending message to " << (*it)->getNickname() << std::endl;
		send_reply(*this, **it, ":" + c->_nickname + "!" + c->_username + "@" + _servername + " JOIN " + target->_name);
	}
}

void server::join(Message &m, client *client){
	std::vector<std::string> params = m.getContent();
	if (params.size() < 1)
	{
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
		return ;
	}
	std::string channelName = params[0];
	struct channel *current;
	if (params[0][0] == '#'){
		current = createChannel(channelName, client);
		if (current->_capacity && current->isModeSet('l') && current->_capacity >= current->_members.size())
		{
			reply(m, *this, *client, ERR_CHANNELISFULL);
			return ;
		}
		if (current->isModeSet('k'))
		{

		}
		if (!current->isModeSet('i'))
		{
			if (std::find(current->_members.begin(), current->_members.end(), client) == current->_members.end())
			{
				joinMessage(current, client);
				current->_members.push_back(client);
			}
			topic(m, client);
		}
		else if (current->isInvited(client))
		{
			current->_members.push_back(client);
			topic(m, client);
			current->removeInvited(client);
		}
		else
				reply(m, *this, *client, ERR_INVITEONLYCHAN);
		reply(m, *this, *client, RPL_NAMREPLY);
		reply(m, *this, *client, RPL_ENDOFNAMES);
		send_reply(*this, *client, ":" + client->getNickname() + " JOIN " + current->_name);
	}
	else if (params[0][0] == '0')//leave all channels
	{
		for(std::vector<channel *>::iterator it = _chan_list.begin(); it != _chan_list.end(); ++it)
		{
			if (client->isMember(*it))
				(*it)->_members.erase(std::find((*it)->_members.begin(), (*it)->_members.end(), client));
		}
	}

}

void server::topic(Message &m, client *client)
{
	std::vector<std::string> params = m.getContent();

	if (params.size() < 1)
	{
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
		return ;
	}
	if (isAchannel(params[0]))
	{
		channel *current = *getChannel(params[0]);
		if (params.size() == 1)
		{
			if (current->_topic.empty())
				reply(m, *this, *client, RPL_NOTOPIC);
			else
				reply(m, *this, *client, RPL_TOPIC);
			return ;

		}
		if (client->isMember(current))
		{
			if (client->isChanop(current) || !current->isModeSet('t'))
				current->_topic = params[1];
			else
				reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
		}
		else
			reply(m, *this, *client, ERR_NOTONCHANNEL);
	}
	else
		reply(m, *this, *client, ERR_NOSUCHCHANNEL);
}


//			PRIVMSG

// int	server::msg_chan(Message m, client *client) {
// 	std::vector<std::string> params = m.getContent();
// 	if (params.size() < 3)
// 		return (ERR_NEEDMOREPARAMS);
// 	std::string	msg = params[2];
//     std::vector<channel>::iterator chan = getChannel(params[1]);
//     if (isClientInChannel(*chan, client.getNickname()))
//         //chan->reply(RPL_PRIVMSG);
//     else
//         reply(ERR_CANNOTSENDTOCHANNEL);//Bien ça ?
// }

void server::chanMessage(channel *target, client *c, std::string msg)
{
	for (std::vector<struct client *>::iterator it = target->_members.begin(); it != target->_members.end() ; ++it) {
		if (*it != c)
			send_reply(*this, **it, ":" + c->_nickname + "!" + c->_username + "@"\
			+ _servername + " PRIVMSG " + target->_name + " :" + msg);
	}
}

void	server::privmsg(Message &m, client *client) {
	std::cout << "in privmsg !\n";
	std::vector<std::string> params = m.getContent();
	std::string	msg = params[1];
	channel *chan;
	if (params.size() < 2)
		reply (m, *this, *client, ERR_NEEDMOREPARAMS);
	else if (params[0][0] == '#') {
		std::cout << "channel message !\n";
		if (checkChannel(params[0]))
    		chan = *getChannel(params[0]);
		else
		{
			reply(m, *this, *client, ERR_NOSUCHCHANNEL);
			return ;
		}
		if (client->isMember(chan))
        	chanMessage(chan, client, msg);
    	else
        	reply(m, *this, *client, ERR_CANNOTSENDTOCHAN);
	}
	else
	{
		std::cout << "client message !\n";
		send_reply(*this, *client, msg);
	}

}

//				QUIT

void	server::quit(Message &m, client *quitting_client) {
	std::vector<std::string> params = m.getContent();
	std::string	msg = params[1];
	std::string quit_message = "QUIT :" + msg;
	close_connection(quitting_client);
    for (std::map<std::string, client *>::iterator it = _nick_map.begin(); it != _nick_map.end(); ++it)
        send_reply(*this, *(it->second), quit_message);
}
