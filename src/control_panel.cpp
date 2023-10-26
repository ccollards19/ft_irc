#include "main.cpp"
#include "errors.hpp"
#include "parsing.cpp"

void send_reply(server &s,client &c, std::string message)
{
	c._send_buffer.append(message);
	s.write_set(c._fd);
}

void send_error(server &s,client &c, std::string message)
{
	c._send_buffer.append(message);
	s.write_set(c._fd);
}
/*
 RPL_UNIQOPIS
 RPL_ENDOFBANLIST
 RPL_ENDOFEXCEPTLIST
 RPL_ENDOFINVITELIST
 RPL_NAMEREPLY
 RPL_ENDOFNANES
 RPL_INVITING

   */
void handle_error_message(Message &m, server &s, client &c, int error)
{
	int nick;
	int server;
	int chan;
	int other;
	switch (m.getCommand()) {
		case KICK : nick = 1; server = 0; chan = 0; other = 0;break;
		case INVITE : nick = 0; server = 0; chan = 1; other = 0;break;
		case TOPIC : nick = 0; server = 0; chan = 0; other = 1;break;
		case PRIVMSG : nick = 0; server = 0; chan = 0; other = 1;break;
		case JOIN : nick = 0; server = 0; chan = 0; other = 1;break;
		default : nick = 0; server = 0; chan = 0; other = 0;
	}
	switch (error) {
		case ERR_NOSUCHNICK : send_error(s, c, m.getContent()[nick] + " :No such nick/channel\n");break;
		case ERR_NOSUCHSERVER : send_error(s, c, m.getContent()[server] + " :No such server\n");break;
		case ERR_NOSUCHCHANNEL : send_error(s, c, m.getContent()[chan] + " :No such channel\n");break;
		case ERR_CANNOTSENDTOCHAN  : send_error(s, c, m.getContent()[chan] + " :Cannot send to channel\n ");break;
		case ERR_TOOMANYCHANNELS  : send_error(s, c, m.getContent()[chan] + " :You have joined too many channels\n");break;
		case ERR_WASNOSUCHNICK  : send_error(s, c, m.getContent()[nick] + " :There was no such nickname");break;
		case ERR_TOOMANYTARGETS  : send_error(s, c, m.getContent()[other] + " :Duplicate recipients. No message delivered\n");break;
		case ERR_NOORIGIN  : send_error(s, c, ":No origin specified ");break;
		case ERR_NORECIPIENT  : send_error(s, c, ":No recipient given " + m.getCommandName() + "\n");break;
		case ERR_NOTEXTTOSEND  : send_error(s, c, ":No text to send\n");break;
		case ERR_NOTOPLEVEL  : send_error(s, c, m.getContent()[other] + " :No top level domain specified\n");break;
		case ERR_WILDTOPLEVEL  : send_error(s, c, m.getContent()[other] + " :Wildcard in toplevel domain\n");break;
		case ERR_UNKNOWNCOMMAND  : send_error(s, c, m.getCommandName() + " :Unknown command\n");break;
		case ERR_NOMOTD  : send_error(s, c, ":MOTD File is missing\n");break;
		case ERR_NOADMININFO  : send_error(s, c, s._servername + " :No administrative info available\n");break;
		case ERR_FILEERROR  : send_error(s, c, ":File error doing " + m.getContent()[0] + " on " + m.getContent()[1] + "\n");break;
		case ERR_NONICKNAMEGIVEN  : send_error(s, c, ":No nickname given\n");break;
		case ERR_ERRONEUSNICKNAME  : send_error(s, c, m.getContent()[nick] + " :Erroneus nickaname\n");break;
		case ERR_NICKNAMEINUSE  : send_error(s, c, m.getContent()[nick] + " :Nickname is already in use\n");break;
		case ERR_NICKCOLLISION  : send_error(s, c, m.getContent()[nick] + " :Nickname collision KILL\n");break;
		case ERR_USERNOTINCHANNEL  : send_error(s, c, m.getContent()[nick] + " " + m.getContent()[chan] +" :They aren't on that channel\n");break;
		case ERR_NOTONCHANNEL  : send_error(s, c, m.getContent()[chan] +" :You're not on that channel\n");break;
		case ERR_USERONCHANNEL  : send_error(s, c, m.getContent()[chan] + " " + m.getContent()[1]+" :is already on channel\n");break;
		case ERR_NOLOGIN  : send_error(s, c, m.getContent()[nick] + " :User not logged in\n");break;
		case ERR_SUMMONDISABLED  : send_error(s, c, ":SUMMON has been disabled\n");break;
		case ERR_USERSDISABLED  : send_error(s, c, ":USERS has been disabled\n");break;
		case ERR_NOTREGISTERED  : send_error(s, c, ":You have not registered\n");break;
		case ERR_NEEDMOREPARAMS  : send_error(s, c, m.getCommandName() + " :Not enough parameters\n");break;
		case ERR_ALREADYREGISTRED  : send_error(s, c, ":You may not register\n");break;
		case ERR_NOPERMFORHOST  : send_error(s, c, ":Your host isn't among the privileged");break;
		case ERR_PASSWDMISMATCH  : send_error(s, c, ":Password incorrect\n");break;
		case ERR_YOUREBANNEDCREEP  : send_error(s, c, "You are banned from this server\n");break;
		case ERR_KEYSET  : send_error(s, c, m.getContent()[chan] + ":Channel key already set\n");break;
		case ERR_CHANNELISFULL  : send_error(s, c, m.getContent()[chan] + ":Cannot join channel (+l)");break;
		case ERR_UNKNOWNMODE  : send_error(s, c, m.getContent()[chan] + ":is unknown mode char to me");break;
		case ERR_INVITEONLYCHAN  : send_error(s, c, m.getContent()[chan] + ":Cannot join channel (+i)");break;
		case ERR_BANNEDFROMCHAN  : send_error(s, c, m.getContent()[chan] + ":Cannot join channel (+b)");break;
		case ERR_BADCHANNELKEY  : send_error(s, c, m.getContent()[chan] + ":Cannot join channel (+k)");break;
		case ERR_NOPRIVILEGES  : send_error(s, c, ":Permission Denied- You're not an IRC operator\n");break;
		case ERR_CHANOPRIVSNEEDED  : send_reply(s, c, "");break;
		case ERR_CANTKILLSERVER  : send_error(s, c, ":You can't kill a server\n");break;
		case ERR_NOOPERHOST  : send_error(s, c, "No O-lines for your host\n");break;
		case ERR_UMODEUNKNOWNFLAG  : send_error(s, c, ":Unknown MODE flag\n");break;
		case ERR_USERSDONTMATCH  : send_error(s, c, ":Can't change mode for other users\n");break;
		case RPL_WELCOME : send_reply(s, c, "Welcome to the Internet Relay Network " + c._nickname + "!" +c._username + "@" + c._hostname + "\n");break;
		case RPL_YOURHOST : send_reply(s, c, "Your host is " + s._servername + ", running version S19CCKKNER\n");break;
		case RPL_CREATED : send_reply(s, c, "This server was created " + s._servername + "\n");break; //TODO change servername to creation_date
		case RPL_MYINFO : send_reply(s, c, s._servername + " S19CCKKNER +itkol\n");break;
		case RPL_BOUNCE : send_reply(s, c, s._servername+ " is already full\n");break;
		case RPL_EXCEPTLIST : send_reply(s, c, m.getContent()[chan] + " \n");break; //TODO exception list
		case RPL_INVITELIST : send_reply(s, c, m.getContent()[chan] + " \n");break; //TODO invite list
		//case RPL_USERHOST  : send_reply(s, c, c._nickname + " = " + c._hostname + "\n");break;
		//case RPL_ISON  : send_reply(s, c, "");break;
		//case RPL_AWAY  : send_reply(s, c, "");break;
		//case RPL_UNAWAY  : send_reply(s, c, "");break;
		//case RPL_NOWAWAY  : send_reply(s, c, "");break;
		//case RPL_WHOISUSER  : send_reply(s, c, "");break;
		//case RPL_WHOISSERVER  : send_reply(s, c, "");break;
		case RPL_WHOISOPERATOR  : send_reply(s, c, "");break;
		case RPL_WHOISIDLE  : send_reply(s, c, "");break;
		case RPL_ENDOFWHOIS  : send_reply(s, c, "");break;
		case RPL_WHOISCHANNELS  : send_reply(s, c, "");break;
		case RPL_WHOWASUSER  : send_reply(s, c, "");break;
		case RPL_ENDOFWHOWAS  : send_reply(s, c, "");break;
		case RPL_LISTSTART  : send_reply(s, c, "");break;
		case RPL_LIST  : send_reply(s, c, "");break;
		case RPL_LISTEND  : send_reply(s, c, "");break;
		case RPL_CHANNELMODEIS  : send_reply(s, c, m.getContent()[chan] + " " + m.getContent()[1] + " " + m.getContent()[2] + "\n");break;
		case RPL_NOTOPIC  : send_reply(s, c, m.getContent()[chan] + " :No topic is set\n");break;
		case RPL_TOPIC  : send_reply(s, c, m.getContent()[chan] + " :" + "\n");break; //TODO gettopic
		case RPL_INVITING  : send_reply(s, c, "");break;
		case RPL_SUMMONING  : send_reply(s, c, "");break;
		case RPL_VERSION  : send_reply(s, c, "");break;
		case RPL_WHOREPLY  : send_reply(s, c, "");break;
		case RPL_ENDOFWHO  : send_reply(s, c, "");break;
		case RPL_NAMREPLY  : send_reply(s, c, "");break;
		case RPL_ENDOFNAMES  : send_reply(s, c, "");break;
		case RPL_LINKS  : send_reply(s, c, "");break;
		case RPL_ENDOFLINKS  : send_reply(s, c, "");break;
		case RPL_BANLIST  : send_reply(s, c, m.getContent()[chan] + "");break; //TODO ban mask
		case RPL_ENDOFBANLIST  : send_reply(s, c, "");break;
		case RPL_INFO  : send_reply(s, c, "");break;
		case RPL_ENDOFINFO  : send_reply(s, c, "");break;
		case RPL_MOTDSTART  : send_reply(s, c, "");break;
		case RPL_MOTD  : send_reply(s, c, "");break;
		case RPL_ENDOFMOTD  : send_reply(s, c, "");break;
		case RPL_YOUREOPER  : send_reply(s, c, "");break;
		case RPL_REHASHING  : send_reply(s, c, "");break;
		case RPL_TIME  : send_reply(s, c, "");break;
		case RPL_USERSSTART  : send_reply(s, c, "");break;
		case RPL_USERS  : send_reply(s, c, "");break;
		case RPL_ENDOFUSERS  : send_reply(s, c, "");break;
		case RPL_NOUSERS  : send_reply(s, c, "");break;
		case RPL_TRACELINK  : send_reply(s, c, "");break;
		case RPL_TRACECONNECTING  : send_reply(s, c, "");break;
		case RPL_TRACEHANDSHAKE  : send_reply(s, c, "");break;
		case RPL_TRACEUNKNOWN  : send_reply(s, c, "");break;
		case RPL_TRACEOPERATOR  : send_reply(s, c, "");break;
		case RPL_TRACEUSER  : send_reply(s, c, "");break;
		case RPL_TRACESERVER  : send_reply(s, c, "");break;
		case RPL_TRACENEWTYPE  : send_reply(s, c, "");break;
		case RPL_TRACELOG  : send_reply(s, c, "");break;
		case RPL_STATSLINKINFO  : send_reply(s, c, "");break;
		case RPL_STATSCOMMANDS  : send_reply(s, c, "");break;
		case RPL_STATSCLINE  : send_reply(s, c, "");break;
		case RPL_STATSNLINE  : send_reply(s, c, "");break;
		case RPL_STATSILINE  : send_reply(s, c, "");break;
		case RPL_STATSKLINE  : send_reply(s, c, "");break;
		case RPL_STATSYLINE  : send_reply(s, c, "");break;
		case RPL_ENDOFSTATS  : send_reply(s, c, "");break;
		case RPL_STATSLLINE  : send_reply(s, c, "");break;
		case RPL_STATSUPTIME  : send_reply(s, c, "");break;
		case RPL_STATSOLINE  : send_reply(s, c, "");break;
		case RPL_STATSHLINE  : send_reply(s, c, "");break;
		//case RPL_UMODEIS  : send_reply(s, c, "");break;
		//case RPL_LUSERCLIENT  : send_reply(s, c, "");break;
		//case RPL_LUSEROP  : send_reply(s, c, "");break;
		//case RPL_LUSERUNKNOWN  : send_reply(s, c, "");break;
		//case RPL_LUSERCHANNELS  : send_reply(s, c, "");break;
		//case RPL_LUSERME  : send_reply(s, c, "");break;
		//case RPL_ADMINME  : send_reply(s, c, "");break;
		//case RPL_ADMINLOC1  : send_reply(s, c, "");break;
		//case RPL_ADMINLOC2  : send_reply(s, c, "");break;
		//case RPL_ADMINEMAIL  : send_reply(s, c, "");break;
		default: ;
	}
}
//replace error = 1 with command
void handle_message(server &s, client &c, Message &m)
{
	int error = 0;
	switch (m.getCommand()) {
		case NONE : error = 1;break;
		case KICK : error = 1;break;
		case INVITE : error = 1;break;
		case TOPIC : error = 1;break;
		case MODE : error = 1;break;
		case NICK : error = 1;break;
		case PRIVMSG : error = 1;break;
		case JOIN : error = 1;break;
		case PING : error = 1;break;
		default : ; //send message
	}
	try {	handle_error_message(m, s, c, error);}
	catch (std::exception &e){}
}