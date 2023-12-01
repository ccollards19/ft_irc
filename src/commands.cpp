#include "irc.hpp"

const std::string& client::getNickname() const
{
	return (_nickname);
}


//                 PASS

bool    client::isRegistered(){
	return (_isRegistered); //must be set at FALSE
}

void server::PASS(Message &m, struct client *client){
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

void server::NICK(Message &m, struct client *client){
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

//                  USER

void server::USER(Message &m, client *client){
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

void server::OPER(Message &m, client *client) {
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

void server::KILL(Message &m, client *client) {
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

void server::MODE(Message &m, client *client){
	(void)client;
	(void)m;
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

void server::JOIN(Message &m, client *client){
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