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
	if (_nick_map.find(params[0]) != _nick_map.end())
		reply(m, *this, *client, ERR_NICKNAMEINUSE);
	else {
		_nick_map[params[0]] = client;
		client->_nickname = params[0];
	}
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
void server::pong(Message &m, struct client *client)//TODO
{
	std::vector<std::string> params = m.getContent();
  if (params.size() > 1)
    reply(m, *this, *client, ERR_NOSUCHSERVER);
  else
    client->_ping = 0;
  update_timer(client->_fd, CLIENT_TTL);
}

//                  USER

void server::user(Message &m, client *client){
	std::cout << "USER\n";
	std::vector<std::string> params = m.getContent();
	if (params.size() < 4)
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
	if (client->_isRegistered)
		reply(m, *this, *client, ERR_ALREADYREGISTRED);
	client->_username = params[0];
	client->_realname = params[3];
	reply(m, *this, *client, RPL_WELCOME);
	reply(m, *this, *client, RPL_YOURHOST);
	reply(m, *this, *client, RPL_CREATED);
	reply(m, *this, *client, RPL_MYINFO);
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
		send_reply(*this, **it, ":" + _servername + " :" + c->_nickname + "JOIN" + target->_name + "\n");
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
		if (!current->isModeSet('i'))
		{
			if (std::find(current->_members.begin(), current->_members.end(), client) == current->_members.end())
			{
				//std::cout << current->_name << " has no member named " << client->getNickname() << " yet\n";
				joinMessage(current, client);
				current->_members.push_back(client);
			}
			topic(m, client);
			//reply(m, *this, *client, RPL_TOPIC);
		}
		else if (current->isInvited(client))
		{
			current->_members.push_back(client);
			topic(m, client);
			//reply(m, *this, *client, RPL_TOPIC);
			current->removeInvited(client);
		}
		else
				reply(m, *this, *client, ERR_INVITEONLYCHAN);
		reply(m, *this, *client, RPL_NAMREPLY);
		reply(m, *this, *client, RPL_ENDOFNAMES);
		send_reply(*this, *client, ":" + client->getNickname() + " JOIN " + current->_name + "\n");
		//332 your_nick #chan :Welcome to #chan!
		//send_reply(*this, *client, "332 " + client->_nickname + " " + current->_name + " :Welcome to " + current->_name + "!\n");
		//send_reply(*this, *client, ":" + client->_nickname + client->_username + " JOIN " + ":" + current->_name);
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

		}
		else if (client->isChanop(current))
			current->_topic = params[1];
		else
			reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
	}
	else
		reply(m, *this, *client, ERR_NOSUCHCHANNEL);
}
