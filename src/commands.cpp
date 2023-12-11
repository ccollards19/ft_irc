#include "irc.hpp"

//                 PART
                 
void server::part(Message &m, struct client *client)
{
	std::vector<std::string> params = m.getContent();
	if (params.size() < 1) {
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
		return ;
	}
	std::string channel_name = params[0];
	if (!checkChannel(channel_name)) {
		reply(m, *this, *client, ERR_NOSUCHCHANNEL);
		return ;
	}
	struct channel *chan = *getChannel(channel_name);
	if (!client->isMember(chan)) {
		reply(m, *this, *client, ERR_NOTONCHANNEL);
		return ;
	}
	if (params.size() == 1)	{
		for (std::vector<struct client *>::iterator it = chan->_members.begin(); it != chan->_members.end() ; ++it) {
			std::cout << "sending message to " << (*it)->_nickname << std::endl;
			send_reply(*this, **it, ":" + client->_nickname + "!" + client->_username + "@" + _servername + " PART " + chan->_name);
		}
	}
	else {
		std::string part_msg = params[1];
		for (std::vector<struct client *>::iterator it = chan->_members.begin(); it != chan->_members.end() ; ++it) {
			std::cout << "sending message to " << (*it)->_nickname << std::endl;
			send_reply(*this, **it, ":" + client->_nickname + "!" + client->_username + "@" + _servername + " PART " + chan->_name + " :" + part_msg);
		}
	}
	chan->_members.erase(std::find(chan->_members.begin(), chan->_members.end(), client));
}

//                 INVITE
                 
void server::invite(Message &m, struct client *client)
{
	std::vector<std::string> params = m.getContent();
	if (params.size() < 2) {
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
		return ;
	}
	std::string channel_name = params[1];
	if (!checkChannel(channel_name)) {
		reply(m, *this, *client, ERR_NOSUCHCHANNEL);
		return ;
	}
	struct channel *chan = *getChannel(channel_name);
	if (!client->isMember(chan)) {
		reply(m, *this, *client, ERR_NOTONCHANNEL);
		return ;
	}
  std::map<std::string, struct client *>::iterator targetit = _nick_map.find(params[0]);
  if (targetit == _nick_map.end()) {
		reply(m, *this, *client, ERR_NOSUCHNICK);
		return ;
	}
  struct client *target = targetit->second;
  if (target->isMember(chan)) {
		reply(m, *this, *client, ERR_USERONCHANNEL);
		return ;
  }
  if (!client->isChanop(chan)) {
		reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
		return ;
  }
	chan->_invite_list.push_back(target);
	reply(m, *this, *client, RPL_INVITING);
	reply(m, *this, *target, RPL_INVITING);
}

//                 PASS

void server::pass(Message &m, struct client *client)
{
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

void server::nick(Message &m, struct client *client)
{
	std::vector<std::string> params = m.getContent();
	if (params.size() == 0) {
		reply(m, *this, *client, ERR_NONICKNAMEGIVEN);
		return ;
	}
	if (params.size() > 1) {
		reply(m, *this, *client, ERR_ERRONEUSNICKNAME);
		return ;
	}
	if (_nick_map.find(params[0]) != _nick_map.end()) {
		reply(m, *this, *client, ERR_NICKNAMEINUSE);
		return ;
	}
	bool hasNick = false;
	std::map<std::string, struct client*>::iterator i;
	if (!_nick_map.empty())	{
		i = _nick_map.begin();
		std::cout << "debug\n";
		for ( ; i != _nick_map.end() ; ++i) {
			std::cout << "nick: "<< i->second->_nickname << "\n";
			if (client == i->second) {
				hasNick = true;
				break ;
			}
			std::cout << "loop\n";
		}
	}
	std::cout << "debug 2\n";
	if (hasNick) {
		send_reply(*this, *client, ":" + i->second->_nickname + "!" + i->second->_username + "@" + _servername + " NICK :" + params[0]);
		_nick_map.erase(i);
	}
	std::cout << "debug 3\n";
	client->_nickname = params[0];
	register_client(m, client);
}

//                  USER

void server::user(Message &m, client *client) 
{
	std::vector<std::string> params = m.getContent();
	if (params.size() < 4) {
		reply(m, *this, *client, ERR_NEEDMOREPARAMS);
    return;
  }
	if (client->_isRegistered) {
		reply(m, *this, *client, ERR_ALREADYREGISTRED);
    return;
  }
	client->_username = params[0];
	client->_realname = params[3];
	client->_hostname = _servername;
  register_client(m, client);
}

//                  PING

void server::ping(Message &m, struct client *client)
{
  std::vector<std::string> params = m.getContent();
  if (params.size() < 1)
    reply(m, *this, *client, ERR_NOORIGIN);
  else if (params.size() > 1 && params[1].compare(_servername))
    reply(m, *this, *client, ERR_NOSUCHSERVER);
  else
    send_reply(*this, *client, ":"+ _servername +" PONG \n");
}

//                 PONG 
                
void server::pong(Message &m, struct client *client)
{
    std::vector<std::string> params = m.getContent();
  if (params.size() < 1)
    reply(m, *this, *client, ERR_NOORIGIN);
  else if (params.size() > 1 && params[1].compare(_servername))
    reply(m, *this, *client, ERR_NOSUCHSERVER);
  else
    client->_ping = 0;
}

//                  MODE

int server::ErrMode(Message &m, client *c, int part, channel *chan)
{
	int err = 0;
	if (part == 1) {
		std::vector<std::string> params = m.getContent();
		if (params.size() < 2) {
			reply(m, *this, *c, ERR_NEEDMOREPARAMS);
			err = 1;
		}
		else if (m.getContent()[1].size() == 2 && (m.getContent()[1][1] != 't' && m.getContent()[1][1] != 'i') && params.size() < 3)
		{
			reply(m, *this, *c, ERR_NEEDMOREPARAMS);
			err = 1;
		}
		else if (!isAchannel(params[0])) {
			reply(m, *this, *c, ERR_NOSUCHCHANNEL);
			err = 1;
		}
	}
	else if (part == 2) {
		if (!c->isChanop(chan) && ++err)
		{
			reply(m, *this, *c, ERR_CHANOPRIVSNEEDED);
			err = 1;
		}
		else if (!c->isMember(chan))
		{
			reply(m, *this, *c, ERR_USERNOTINCHANNEL);
			err = 1;
		}
		else if (m.getContent()[1].size() > 2)
		{
			reply(m, *this, *c, ERR_UNKNOWNMODE);
			err = 1;
		}
	}
	return err;
}

void server::modeK(Message &m, client *client, channel *c)
{
	std::cout << "In mode K\n";

	std::vector<std::string> params = m.getContent();
	std::string mode = params[1];
	if (client->isChanop(c))
	{
		if (mode[0] == '+')
		{
			if (c->_mode.find('k') == std::string::npos)
				c->_mode += 'k';
			c->_pwd = params[2];
		}
		if (mode[0] == '-' && c->_mode.find('k') != std::string::npos)
			c->_mode.erase(c->_mode.find('k'));
	}
	else
		reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
}

void server::modeO(Message &m, client *client, channel *c)
{
	std::cout << "In mode O\n";

	std::vector<std::string> params = m.getContent();
	std::string mode = params[1];
	if (mode[0] == '+')
	{
		std::cout << "changing operator\n";
		if (!client->isChanop(c))
			reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
		else if (_nick_map.find(params[2]) == _nick_map.end())
			reply(m, *this, *client, ERR_NOSUCHNICK);
		else if (!_nick_map[params[2]]->isMember(c))
			reply(m, *this, *client, ERR_USERNOTINCHANNEL);
		else
			c->_operators.push_back(_nick_map[params[2]]);
	}
	else if (mode[0] == '-')
	{
		struct client *target;
		if (_nick_map.find(params[2]) == _nick_map.end())
			reply(m, *this, *client, ERR_NOSUCHNICK);
		else if (!_nick_map[params[2]]->isMember(c))
			reply(m, *this, *client, ERR_USERNOTINCHANNEL);
		else
			target = _nick_map[params[2]];
		if (!client->isChanop(c) || target == c->_creator)
			reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
		else
			c->_operators.erase(std::find(c->_operators.begin(),c->_operators.end(), target));
	}
}

void server::modeI(Message &m, client *client, channel *c)
{
	std::cout << "In mode I\n";

	std::vector<std::string> params = m.getContent();
	std::string mode = params[1];
	if (!client->isChanop(c))
		reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
	else
	{
		if (mode[0] == '+') {
			if (c->_mode.find('i') == std::string::npos)
				c->_mode += 'i';

			if ( params.size() > 3 && _nick_map.find(params[2]) != _nick_map.end() && \
			std::find(c->_invite_list.begin(), c->_invite_list.end(), \
			_nick_map[params[2]]) == c->_invite_list.end())
				c->_invite_list.push_back(_nick_map[params[2]]);
		}
		if (mode[0] == '-' && c->_mode.find('i') != std::string::npos)
			c->_mode.erase(c->_mode.find('i'));
		reply(m, *this, *client, RPL_INVITELIST);
		reply(m, *this, *client, RPL_ENDOFINVITELIST);
	}
}

void server::modeL(Message &m, client *client, channel *c)
{
	std::cout << "In mode L\n";

	std::vector<std::string> params = m.getContent();
	std::string mode = params[1];
	int size = stoi(params[2]);
	if (!client->isChanop(c))
		reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
	else
	{
		if (mode[0] == '+')
		{
			if (c->_mode.find('l') == std::string::npos)
				c->_mode += 'l';
			if (size < 0)
				c->_capacity = 0;
			else
				c->_capacity = size;
		}
		if (mode[0] == '-' && c->_mode.find('l') != std::string::npos)
			c->_mode.erase(c->_mode.find('l'));
	}
}

void server::modeT(Message &m, client *client, channel *c)
{
	std::cout << "In mode T\n";
	std::vector<std::string> params = m.getContent();
	std::string mode = params[1];
	if (!client->isChanop(c))
		reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
	else
	{
		if (mode[0] == '+')
		{
			if (c->_mode.find('t') == std::string::npos)
				c->_mode += 't';
		}
		if (mode[0] == '-' && c->_mode.find('t') != std::string::npos)
			c->_mode.erase(c->_mode.find('t'));
	}
}

std::string getModeMessage(std::string _servername, std::string _clientname, std::string _channelname, std::string _modename, int mode, std::vector<std::string> params)
{
	std::string res;

	res = ":" + _servername + " :" + _clientname + " MODE " + _channelname + " " + _modename;
	switch (mode) {
		case 'i': params.size() > 3 ? res += params[2] : res += "";break;
		case 't':res += params[2];break;
		case 'k':res += params[2];break;
		case 'o':res += params[2];break;
		case 'l':res += params[2];break;
	}
	return res;
}

void server::mode(Message &m, client *client)
{
	std::vector<std::string> params = m.getContent();
	std::string chanModes("itkol");
	if (ErrMode(m, client, 1, NULL))
		return ;
	channel *c;
	c = *getChannel(params[0]);
	if (ErrMode(m, client, 2, c))
		return;
	std::string mode(params[1]);
	std::cout << mode <<  "\n";
	std::cout << chanModes.find(mode[1]) <<  "\n";
	std::string msg;
	if (chanModes.find(mode[1]) != std::string::npos && (mode[0] == '-' || mode[0] == '+')) {
		if (mode[1] == 'i')
			modeI(m, client, c);
		else if (mode[1] == 'k')
			modeK(m, client, c);
		else if (mode[1] == 'o')
			modeO(m, client, c);
		else if (mode[1] == 'l')
			modeL(m, client, c);
		else if (mode[1] == 't')
			modeT(m, client, c);
		msg = getModeMessage(_servername, client->_nickname, c->_name, mode, mode[1], params);
		for (std::vector<struct client *>::iterator it = c->_members.begin(); it != c->_members.end(); ++it)
			send_reply(*this, **it, msg);
	}
	else
	{
		m.showContent();
		reply(m, *this, *client, ERR_UNKNOWNMODE);
	}
}

//                  Join

void server::joinMessage(channel *target, client *c)
{
	for (std::vector<struct client *>::iterator it = target->_members.begin(); it != target->_members.end() ; ++it) {
		std::cout << "sending message to " << (*it)->_nickname << std::endl;
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
		send_reply(*this, *client, ":" + client->_nickname + " JOIN " + current->_name);
		reply(m, *this, *client, RPL_NAMREPLY);
		reply(m, *this, *client, RPL_ENDOFNAMES);
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
		struct client *target;
		if (_nick_map.find(params[0]) != _nick_map.end())
		{
			target = _nick_map[params[0]];
			send_reply(*this, *target, ":" + client->_nickname + "!" + client->_username + "@"\
			+ _servername + " PRIVMSG " + client->_nickname + " :" + msg);
		}
		else
			reply(m, *this, *client, ERR_NOSUCHNICK);
	}
}

//				KICK
void	server::kick(Message &m, struct client *client)
{
  std::vector<std::string> params = m.getContent();
  int size = params.size();
  if (size < 2) {
    reply(m, *this, *client, ERR_NEEDMOREPARAMS);
    return ;
  }
  std::vector<struct channel *>::iterator chanit = getChannel(params[0]);
  if (chanit == _chan_list.end()) {
    reply(m, *this, *client, ERR_NOSUCHCHANNEL);
    return ;
  }
  struct channel *chan = *chanit;
  if (!client->isMember(chan)) {
    reply(m, *this, *client, ERR_NOTONCHANNEL);
    return ;
  }
  if (!client->isChanop(chan)) {
    reply(m, *this, *client, ERR_CHANOPRIVSNEEDED);
    return ;
  }
  std::map<std::string, struct client *>::iterator target = _nick_map.find(params[1]);
  if (target == _nick_map.end() || !target->second->isMember(chan)) {
    reply(m, *this, *client, ERR_USERNOTINCHANNEL);
    return ;
  }

  std::string kick_message = ":" + client->_nickname + "!" + client->_username + "@" + client->_hostname + " KICK " + chan->_name + " " + target->second->_nickname;
  if (size > 2) 
    kick_message.append(" :" + params.back());
  for (std::vector<struct client *>::iterator it = chan->_members.begin(); it != chan->_members.end(); ++it)
    send_reply(*this, **it, kick_message);
  chan->removeMember(target->second);
}

//				QUIT
void	server::quit(Message &m, client *client)
{
  std::vector<std::string> params = m.getContent();
  std::string quit_message = ":" + client->_nickname + "!" + client->_username + "@" + client->_hostname + " QUIT";
  if (params.size() > 0)
    quit_message.append( " :"+ params[1]);
  close_connection(client);
  for (std::map<std::string, struct client*>::iterator it = _nick_map.begin(); it != _nick_map.end(); ++it)
    send_reply(*this, *(it->second), quit_message);
  std::vector<channel *>::iterator end = _chan_list.end();
  for (std::vector<channel *>::iterator it = _chan_list.begin(); it != end; it++) {
    if ((*it)->_members.empty())
      removeChannel(*it);
  }
}
