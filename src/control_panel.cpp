#include "irc.hpp"

// server utils

void send_reply(struct server &s,struct client &c, std::string message)
{
	//std::cerr << "RENTRE RPL\n";
	c._send_buffer.append(message + "\n");
	s.write_set(c._fd);
}

void send_error(struct server &s,struct client &c, std::string message)
{
	c._send_buffer.append(message);
	s.write_set(c._fd);
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
	_nick_map[client->_nickname] = client;
  client->_isRegistered = true;
}

struct channel *server::createChannel(std::string channelName, struct client *client)
{
  if (checkChannel(channelName))
    return (*getChannel(channelName));
  channel *chan = new channel();
  chan->_name = channelName;
  chan->_operators.push_back(client);
  chan->_creator = client;
  chan->_prefix = '#';
  chan->_chan_id = channelName;
  chan->_capacity = 0;
  _chan_list.push_back(chan);
  return _chan_list.back();
  // Message à print. Pas sur de ce que c'est
}

std::vector<struct channel*>::iterator	server::getChannel(std::string channelName) 
{
	for(std::vector<struct channel*>::iterator it = _chan_list.begin(); it != _chan_list.end(); ++it) {
		if ((*it)->_name == channelName)	{
			return (it);
		}
	}
	return (_chan_list.end());
}

void	channel::removeInvited(struct client *client)
{
	std::vector<struct client*>::iterator i;
	if ((i = std::find(_invite_list.begin(), _invite_list.end(), client)) != _invite_list.end())
		_invite_list.erase(i);
}

void	channel::removeMember(struct client *client) 
{
	std::vector<struct client*>::iterator i;
	if ((i = std::find(_members.begin(), _members.end(), client)) != _members.end())
		_members.erase(i);
	if ((i = std::find(_operators.begin(), _operators.end(), client)) != _operators.end()) {
	  _operators.erase(i);
    if (_operators.empty() && !_members.empty()) {
      _operators.push_back(_members.front());
      //TODO send Mode message
    }
  }

}

void	server::removeChannel(struct channel *channel)
{
  for(std::vector<struct channel*>::iterator it = _chan_list.begin(); it != _chan_list.end(); ++it) {
		if (*it == channel) {
			_chan_list.erase(it);
      delete channel;
      return ;
    }
	}
}

void channel::addClient(client *client)
{
	_members.push_back(client);
}

// bools function

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

bool	channel::isModeSet(char mode)
{
	for (std::vector<char>::iterator it = this->_mode.begin(); it != this->_mode.end(); ++it) {
		if (*it == mode)
			return (true);
	}
	return (false);
}

bool channel::isInvited(client *c)
{
	return (std::find(_invite_list.begin(), _invite_list.end(), c) != _invite_list.end());
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

struct channel *get_channel(struct server s, std::string name)
{
	std::vector<channel *> channels = s._chan_list;
	std::vector<channel *>::iterator i = channels.begin();
	while (i != channels.end())
	{
		if (name == (*i)->_name)
			return (*i);
		i++;
	}
	return NULL;
}

std::string get_mask_list(struct server s, std::string name, char flag)
{
	if (flag != 'b' && flag != 'i')
		return "";
	std::string res;
	channel *chan = get_channel(s, name);
	std::vector<client *> target;
	std::vector<client *>::iterator i;
	(flag == 'b' ? target = chan->_ban_list : target = chan->_invite_list);
	for (std::vector<client *>::iterator i = target.begin(); i != target.end() ; ++i) {
		res += (*i)->_nickname + " ";
	}
	return res;
}

client *get_operator(struct server s, std::string name)
{
	channel *target = get_channel(s, name);
	client *uniqop;
	target->_operators.size() == 1 ? uniqop = target->_operators[0] : uniqop = NULL;
	return uniqop;
}

std::string to_string(int error)
{
	std::string res = std::to_string(error);
	while (res.size() != 3)
		res = "0" + res;
	return res;
}

std::string getNameList(Message m, server &s)
{
	std::string res;
	channel *chan = get_channel(s, m.getContent()[0]);
	std::vector<client *>::iterator it = chan->_members.begin();
	for (; it != chan->_members.end() ; ++it) {
		res += (*it)->_nickname;
		res += " ";
	}
	return res;
}

void reply(Message &m, struct server &s, struct client &c, int error)
{
	int nick;
	int server;
	int chan;
	int other;
	//m.showContent();
	std::cout << "RPL = " << error << std::endl;
	switch (m.getCommand()) {
		case KICK : nick = 1; server = 0; chan = 0; other = 0;break;
		case INVITE : nick = 0; server = 0; chan = 1; other = 0;break;
		case TOPIC : nick = 0; server = 0; chan = 0; other = 1;break;
		case PRIVMSG : nick = 0; server = 0; chan = 0; other = 1;break;
		case JOIN : nick = 0; server = 0; chan = 0; other = 1;break;
		default : nick = 0; server = 0; chan = 0; other = 0;
	}
	switch (error) {
		case ERR_NOSUCHNICK : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :No such nick/channel\n");break;
		case ERR_NOSUCHSERVER : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[server] + " :No such server\n");break;
		case ERR_NOSUCHCHANNEL : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :No such channel\n");break;
		case ERR_CANNOTSENDTOCHAN  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :Cannot send to channel\n ");break;
		case ERR_TOOMANYCHANNELS  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :You have joined too many channels\n");break;
		case ERR_WASNOSUCHNICK  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :There was no such nickname\n");break;
		case ERR_TOOMANYTARGETS  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[other] + " :Duplicate recipients. No message delivered\n");break;
		case ERR_NOORIGIN  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":No origin specified ");break;
		case ERR_NORECIPIENT  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":No recipient given " + m.getCommandName() + "\n");break;
		case ERR_NOTEXTTOSEND  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":No text to send\n");break;
		case ERR_NOTOPLEVEL  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[other] + " :No top level domain specified\n");break;
		case ERR_WILDTOPLEVEL  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[other] + " :Wildcard in toplevel domain\n");break;
		case ERR_UNKNOWNCOMMAND  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getCommandName() + " :Unknown command\n");break;
		case ERR_NOMOTD  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":MOTD File is missing\n");break;
		case ERR_NOADMININFO  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  s._servername + " :No administrative info available\n");break;
		case ERR_FILEERROR  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":File error doing " + m.getContent()[0] + " on " + m.getContent()[1] + "\n");break;
		case ERR_NONICKNAMEGIVEN  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":No nickname given\n");break;
		case ERR_ERRONEUSNICKNAME  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :Erroneus nickname\n");break;
		case ERR_NICKNAMEINUSE  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :Nickname is already in use\n");break;
		case ERR_NICKCOLLISION  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :Nickname collision KILL\n");break;
		case ERR_USERNOTINCHANNEL  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " " + m.getContent()[chan] +" :They aren't on that channel\n");break;
		case ERR_NOTONCHANNEL  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] +" :You're not on that channel\n");break;
		case ERR_USERONCHANNEL  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " " + m.getContent()[1]+" :is already on channel\n");break;
		case ERR_NOLOGIN  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :User not logged in\n");break;
		case ERR_SUMMONDISABLED  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":SUMMON has been disabled\n");break;
		case ERR_USERSDISABLED  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":USERS has been disabled\n");break;
		case ERR_NOTREGISTERED  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":You have not registered\n");break;
		case ERR_NEEDMOREPARAMS  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getCommandName() + " :Not enough parameters\n");break;
		case ERR_ALREADYREGISTRED  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":You may not register\n");break;
		case ERR_NOPERMFORHOST  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":Your host isn't among the privileged\n");break;
		case ERR_PASSWDMISMATCH  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":Password incorrect\n");break;
		case ERR_YOUREBANNEDCREEP  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  "You are banned from this server\n");break;
		case ERR_KEYSET  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + ":Channel key already set\n");break;
		case ERR_CHANNELISFULL  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :Cannot join channel (+l)\n");break;
		case ERR_UNKNOWNMODE  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :is unknown mode char to me\n");break;
		case ERR_INVITEONLYCHAN  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :Cannot join channel (+i)\n");break;
		case ERR_BANNEDFROMCHAN  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :Cannot join channel (+b)\n");break;
		case ERR_BADCHANNELKEY  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :Cannot join channel (+k)\n");break;
		case ERR_NOPRIVILEGES  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  " :Permission Denied- You're not an IRC operator\n");break;
		case ERR_CHANOPRIVSNEEDED  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :You're not channel operator\n");break;
		case ERR_CANTKILLSERVER  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":You can't kill a server\n");break;
		case ERR_NOOPERHOST  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  "No O-lines for your host\n");break;
		case ERR_UMODEUNKNOWNFLAG  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":Unknown MODE flag\n");break;
		case ERR_USERSDONTMATCH  : send_error(s, c,":" + s._servername +" " + to_string(error) + " " + c._nickname + " " +  ":Can't change mode for other users\n");break;


		case RPL_WELCOME : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + "Welcome to the Internet Relay Network " + c._nickname + "!" +c._username + "@" + c._hostname);break;
		case RPL_YOURHOST : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + "Your host is " + s._servername + ", running version S19CCKKNER");break;
		case RPL_CREATED : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + "This server was created " + s._creation_date);break;
		case RPL_MYINFO : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + s._servername + " S19CCKKNER +itkol");break;
		case RPL_BOUNCE : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + s._servername+ " is already full");break;
			//case RPL_EXCEPTLIST : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + get_channel_list(s, m.getContent()[chan], 'l'));break; //TODO exception list
		case RPL_INVITELIST : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + get_mask_list(s,m.getContent()[chan], 'i'));break;
		case RPL_UNIQOPIS : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " " + get_operator(s, m.getContent()[chan])->_nickname);break;
		case RPL_CHANNELMODEIS  : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " " + m.getContent()[1] + " " + m.getContent()[2]);break;
		case RPL_NOTOPIC  : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " :No topic is set");break;
		case RPL_TOPIC  : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " :" + get_channel(s, m.getContent()[chan])->_topic);break;
		case RPL_INVITING  : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] +" " + m.getContent()[nick]);break;
		case RPL_BANLIST  : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + get_mask_list(s, m.getContent()[chan], 'b'));break;
		case RPL_ENDOFBANLIST  : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " : End of channel ban list");break;
			//case RPL_ENDOFEXCEPTLIST : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " : End of channel except list\n");break;
		case RPL_NAMREPLY : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " = " + m.getContent()[chan] + " :"+ getNameList(m, s));break;
		case RPL_ENDOFINVITELIST : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " : End of channel invite list.");break;
		case RPL_ENDOFNAMES : send_reply(s, c, ":" + s._servername +" " + to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " :End of NAMES list");break;
		default: ;
	}
}

void exemple(Message &m, struct server &s, struct client &c, int error)
{
	reply(m, s, c, error);
}
