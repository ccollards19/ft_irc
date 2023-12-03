#include "irc.hpp"

const std::string& client::getNickname() const
{
	return (_nickname);
}


//                 PASS

bool    client::isRegistered(){
	return (_isRegistered); //must be set at FALSE
}

void server::pass(Message &m, struct client *client){
	std::vector<std::string> params = m.getContent();
	if (params.size() < 2)
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
	else if (client->isRegistered())
		reply(m, *this, *client, ERR_ALREADYREGISTRED);
	else {
		if (params[0] == _password)
			client->_isRegistered = true;
	}
}

//                  NICK

void server::nick(Message &m, struct client *client){
	std::vector<std::string> params = m.getContent();
	if (params.size() == 0)
		reply(m, *this, *client, ERR_NONICKNAMEGIVEN);
	if (params.size() > 1)
		reply(m, *this, *client, ERR_ERRONEUSNICKNAME);
	try {
		_nick_map.at(params[0]);
		reply(m, *this, *client, ERR_NICKNAMEINUSE);
	}
	catch (std::exception &e){
		_nick_map[params[0]] = client;
		client->_nickname = params[0];
	}
}

//                  PING
void server::ping(Message &m, struct client *client)
{
  m.getHost();
 //  if ()
 //    reply(msg, *this, *client, ERR_NOORIGIN);           
 //  if ()
 //    reply(msg, *this, *client, ERR_NOSUCHSERVER);
	// client->_ping = 1;
	// client->send_buffer<TODO prefix<<" PONG :"<<_servername<<"\r\n";
	// write_set(client->_fd);
	update_timer(client->_fd, CLIENT_TTL);
}

//                 PONG 
void server::pong(Message &m, struct client *client)//TODO
{
  m.getHost();
  //  if ()
 //    reply(msg, *this, *client, ERR_NOORIGIN);           
 //  if ()
 //    reply(msg, *this, *client, ERR_NOSUCHSERVER);
  client->_ping = 0;
	update_timer(client->_fd, CLIENT_TTL);
}

//                  USER

void server::user(Message &m, client *client){
	std::vector<std::string> params = m.getContent();
	std::string realName;
	if (client->getNickname().empty())
		reply(m, *this, *client, ERR_NONICKNAMEGIVEN);
	if (params.size() < 4)
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
	if (client->_isRegistered)
		reply(m, *this, *client, ERR_ALREADYREGISTRED);
	client->_username = params[0];
	if (params.size() > 4)
	{
		if (!realName.empty())
			realName.erase(realName.size() - 1);
		for (size_t i = 3; i < params.size(); i++)
			realName += params[i] + " ";
		//RPL_WELCOME ?
	}
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
	for ( ;  (*i)->_name != name && i != _chan_list.end() ; i++) {}
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

	if (params[0][1] == '#')//CHANNEL MODE
	{
		channel *c;
		if (params.size() < 3)
			reply(m, *this, *client, ERR_NEEDMOREPARAMS);
		if (!isAchannel(params[0]))
			reply(m, *this, *client, ERR_NOSUCHCHANNEL);
		else
		{
			c = *getChannel(params[0]);
			if (!client->isChanop(c))
				reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
			if (!client->isMember(c))
				reply(m, *this, *client, ERR_NOSUCHNICK);
			if (params.size() == 2)
			{
				if (params[1][0] == '+')
					;
				else if (params[1][0] == '-')
					;
				else
					reply(m, *this, *client, ERR_KEYSET);
			}
			else
			{

			}
		}
	}
	else//USER MODE
	{
		if (params.size() < 2)
			reply(m, *this, *client, ERR_NEEDMOREPARAMS);
		if (params[0] != client->getNickname())
			reply(m, *this, *client, ERR_USERSDONTMATCH);
		if (modes.find(params[1][1]) == std::string::npos)
			reply(m, *this, *client, ERR_UMODEUNKNOWNFLAG);
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

bool	server::checkChannel(std::string channelName, client *client){
	std::vector<channel *>::iterator it;
	for(it = server::_chan_list.begin(); it != server::_chan_list.end(); ++it) {
		if ((*it)->_name == channelName) {
			if (!(*it)->isModeSet('i'))
				(*it)->addClient(client);
			return (true);
		}
	}
	return (false);
}

struct channel *server::createChannel(std::string channelName, struct client *client){
	struct channel newChannel;// A coder
	if (checkChannel(channelName, client))
		return (*getChannel(channelName));
	else
	{
		_chan_list.push_back(&newChannel);
		return _chan_list.back();
	}

	// Message à print. Pas sur de ce que c'est
}

bool channel::isInvited(client *c) {
	return (std::find(_invite_list.begin(), _invite_list.end(), c) != _invite_list.end());
}

void server::join(Message &m, client *client){
	std::vector<std::string> params = m.getContent();
	std::string channelName = params[0];
	struct channel *current;
	//verif
	//if 0 params return (ERR_NEEDMOREPARAMS)
	//if client is registered ? return (ERR_NOTREGISTERED) possible ?
	//if not in server::_chan_list create new channel
	if (params[0][0] == '#'){
		current = createChannel(channelName, client);
		//chan invite only ?
		if (!current->isModeSet('i'))
			reply(m, *this, *client, RPL_TOPIC);
		else {
			if (current->isInvited(client)) {
				//itChannel->addClient(client); A coder
				current->removeInvited(client);
				//rpl_Join(client, *itChannel); Commande à créer ou fonction de Nico gère ?
			}
			else
				reply(m, *this, *client, ERR_INVITEONLYCHAN);
		}
	}
}
