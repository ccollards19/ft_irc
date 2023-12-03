#include "irc.hpp"

void send_reply(struct server &s,struct client &c, std::string message)
{
	//std::cerr << "RENTRE RPL\n";
	c._send_buffer.append(message);
	s.write_set(c._fd);
}

void send_error(struct server &s,struct client &c, std::string message)
{
	c._send_buffer.append(message);
	s.write_set(c._fd);
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
	i = target.begin();
	while (i != target.end())
	{
		if ((*i)->_mode.find(flag))
		{
			res += (*i)->_nickname;
			res += "\n";
		}
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
void reply(Message &m, struct server &s, struct client &c, int error)
{
	int nick;
	int server;
	int chan;
	int other;
	m.showContent();
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
		case ERR_NOSUCHNICK : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :No such nick/channel\n");break;
		case ERR_NOSUCHSERVER : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[server] + " :No such server\n");break;
		case ERR_NOSUCHCHANNEL : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :No such channel\n");break;
		case ERR_CANNOTSENDTOCHAN  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :Cannot send to channel\n ");break;
		case ERR_TOOMANYCHANNELS  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :You have joined too many channels\n");break;
		case ERR_WASNOSUCHNICK  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :There was no such nickname");break;
		case ERR_TOOMANYTARGETS  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[other] + " :Duplicate recipients. No message delivered\n");break;
		case ERR_NOORIGIN  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":No origin specified ");break;
		case ERR_NORECIPIENT  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":No recipient given " + m.getCommandName() + "\n");break;
		case ERR_NOTEXTTOSEND  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":No text to send\n");break;
		case ERR_NOTOPLEVEL  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[other] + " :No top level domain specified\n");break;
		case ERR_WILDTOPLEVEL  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[other] + " :Wildcard in toplevel domain\n");break;
		case ERR_UNKNOWNCOMMAND  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getCommandName() + " :Unknown command\n");break;
		case ERR_NOMOTD  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":MOTD File is missing\n");break;
		case ERR_NOADMININFO  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  s._servername + " :No administrative info available\n");break;
		case ERR_FILEERROR  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":File error doing " + m.getContent()[0] + " on " + m.getContent()[1] + "\n");break;
		case ERR_NONICKNAMEGIVEN  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":No nickname given\n");break;
		case ERR_ERRONEUSNICKNAME  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :Erroneus nickname\n");break;
		case ERR_NICKNAMEINUSE  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :Nickname is already in use\n");break;
		case ERR_NICKCOLLISION  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :Nickname collision KILL\n");break;
		case ERR_USERNOTINCHANNEL  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " " + m.getContent()[chan] +" :They aren't on that channel\n");break;
		case ERR_NOTONCHANNEL  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] +" :You're not on that channel\n");break;
		case ERR_USERONCHANNEL  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " " + m.getContent()[1]+" :is already on channel\n");break;
		case ERR_NOLOGIN  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[nick] + " :User not logged in\n");break;
		case ERR_SUMMONDISABLED  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":SUMMON has been disabled\n");break;
		case ERR_USERSDISABLED  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":USERS has been disabled\n");break;
		case ERR_NOTREGISTERED  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":You have not registered\n");break;
		case ERR_NEEDMOREPARAMS  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getCommandName() + " :Not enough parameters\n");break;
		case ERR_ALREADYREGISTRED  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":You may not register\n");break;
		case ERR_NOPERMFORHOST  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":Your host isn't among the privileged");break;
		case ERR_PASSWDMISMATCH  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":Password incorrect\n");break;
		case ERR_YOUREBANNEDCREEP  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  "You are banned from this server\n");break;
		case ERR_KEYSET  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + ":Channel key already set\n");break;
		case ERR_CHANNELISFULL  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + ":Cannot join channel (+l)");break;
		case ERR_UNKNOWNMODE  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + ":is unknown mode char to me");break;
		case ERR_INVITEONLYCHAN  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + ":Cannot join channel (+i)");break;
		case ERR_BANNEDFROMCHAN  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + ":Cannot join channel (+b)");break;
		case ERR_BADCHANNELKEY  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + ":Cannot join channel (+k)");break;
		case ERR_NOPRIVILEGES  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":Permission Denied- You're not an IRC operator\n");break;
		case ERR_CHANOPRIVSNEEDED  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  m.getContent()[chan] + " :You're not channel operator\n");break;
		case ERR_CANTKILLSERVER  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":You can't kill a server\n");break;
		case ERR_NOOPERHOST  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  "No O-lines for your host\n");break;
		case ERR_UMODEUNKNOWNFLAG  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":Unknown MODE flag\n");break;
		case ERR_USERSDONTMATCH  : send_error(s, c,to_string(error) + " " + c._nickname + " " +  ":Can't change mode for other users\n");break;
		case RPL_WELCOME : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + "Welcome to the Internet Relay Network " + c._nickname + "!" +c._username + "@" + c._hostname + "\n");break;
		case RPL_YOURHOST : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + "Your host is " + s._servername + ", running version S19CCKKNER\n");break;
		case RPL_CREATED : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + "This server was created " + s._creation_date + "\n");break;
		case RPL_MYINFO : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + s._servername + " S19CCKKNER +itkol\n");break;
		case RPL_BOUNCE : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + s._servername+ " is already full\n");break;
			//case RPL_EXCEPTLIST : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + get_channel_list(s, m.getContent()[chan], 'l'));break; //TODO exception list
		case RPL_INVITELIST : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + get_mask_list(s,m.getContent()[chan], 'i'));break;
		case RPL_UNIQOPIS : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " " + get_operator(s, m.getContent()[chan])->_nickname + " \n");break;
		case RPL_CHANNELMODEIS  : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " " + m.getContent()[1] + " " + m.getContent()[2] + "\n");break;
		case RPL_NOTOPIC  : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " :No topic is set\n");break;
		case RPL_TOPIC  : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " :" + get_channel(s, m.getContent()[chan])->_topic + "\n");break;
		case RPL_INVITING  : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] +" " + m.getContent()[nick]+ "\n");break;
		case RPL_BANLIST  : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + get_mask_list(s, m.getContent()[chan], 'b'));break;
		case RPL_ENDOFBANLIST  : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " : End of channel ban list\n");break;
			//case RPL_ENDOFEXCEPTLIST : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " : End of channel except list\n");break;
		case RPL_ENDOFINVITELIST : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " : End of channel invite list\n");break;
		case RPL_NAMEREPLY : ;break;
		case RPL_ENDOFNAMES : send_reply(s, c, to_string(error) + + " " +c._nickname + " " + m.getContent()[chan] + " : End of channel ban list\n");break;
		default: ;
	}
}

void exemple(Message &m, struct server &s, struct client &c, int error)
{
	reply(m, s, c, error);
}