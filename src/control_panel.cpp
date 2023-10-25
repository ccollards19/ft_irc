#include "main.cpp"
#include "errors.hpp"
#include "parsing.cpp"
void send_error(server &s,client &c, std::string message)
{
	c._send_buffer.append(message);
	s.write_set(c._fd);
}
void handle_error_message(Message &m, server &s, client &c, int error)
{
	switch (error) {
		case ERR_NOSUCHNICK : send_error(s, c, m.getContent()[0] + " :No such nick/channel\n");break;
		case ERR_NOSUCHSERVER : send_error(s, c, m.getContent()[0] + " :No such server\n");break;
		case ERR_NOSUCHCHANNEL : send_error(s, c, m.getContent()[0] + " :No such channel\n");break;
		case ERR_CANNOTSENDTOCHAN  : send_error(s, c, m.getContent()[0] + " :Cannot send to channel\n ");break;
		case ERR_TOOMANYCHANNELS  : send_error(s, c, m.getContent()[0] + " :You have joined too many channels\n");break;
		case ERR_WASNOSUCHNICK  : send_error(s, c, m.getContent()[0] + " :There was no such nickname");break;
		case ERR_TOOMANYTARGETS  : send_error(s, c, m.getContent()[0] + " :Duplicate recipients. No message delivered\n");break;
		case ERR_NOORIGIN  : send_error(s, c, ":No origin specified ");break;
		case ERR_NORECIPIENT  : send_error(s, c, ":No recipient given " + m.getCommandName() + "\n");break;
		case ERR_NOTEXTTOSEND  : send_error(s, c, ":No text to send\n");break;
		case ERR_NOTOPLEVEL  : send_error(s, c, m.getContent()[0] + " :No top level domain specified\n");break;
		case ERR_WILDTOPLEVEL  : send_error(s, c, m.getContent()[0] + " :Wildcard in toplevel domain\n");break;
		case ERR_UNKNOWNCOMMAND  : send_error(s, c, m.getCommandName() + " :Unknown command\n");break;
		case ERR_NOMOTD  : send_error(s, c, ":MOTD File is missing\n");break;
		case ERR_NOADMININFO  : send_error(s, c, s._servername + " :No administrative info available\n");break;
		case ERR_FILEERROR  : send_error(s, c, ":File error doing " + m.getContent()[0] + " on " + m.getContent()[1] + "\n");break;
		case ERR_NONICKNAMEGIVEN  : send_error(s, c, ":No nickname given\n");break;
		case ERR_ERRONEUSNICKNAME  : send_error(s, c, m.getContent()[0] + " :Erroneus nickaname\n");break;
		case ERR_NICKNAMEINUSE  : send_error(s, c, m.getContent()[0] + " :Nickname is already in use\n");break;
		case ERR_NICKCOLLISION  : send_error(s, c, m.getContent()[0] + " :Nickname collision KILL\n");break;
		case ERR_USERNOTINCHANNEL  : send_error(s, c, "");break; //TODO ask Cyril to add the channel provenance
		case ERR_NOTONCHANNEL  : send_error(s, c, m.getContent()[1] +" :You're not on that channel\n");break;
		case ERR_USERONCHANNEL  : send_error(s, c, m.getContent()[0] + " " + m.getContent()[1]+" :is already on channel\n");break;
		case ERR_NOLOGIN  : send_error(s, c, m.getContent()[0] + " :User not logged in\n");break;
		case ERR_SUMMONDISABLED  : send_error(s, c, ":SUMMON has been disabled\n");break;
		case ERR_USERSDISABLED  : send_error(s, c, ":USERS has been disabled\n");break;
		case ERR_NOTREGISTERED  : send_error(s, c, ":You have not registered\n");break;
		case ERR_NEEDMOREPARAMS  : send_error(s, c, m.getCommandName() + " :Not enough parameters\n");break;
		case ERR_ALREADYREGISTRED  : send_error(s, c, ":You may not register\n");break;
		case ERR_NOPERMFORHOST  : send_error(s, c, ":Your host isn't among the privileged");break;
		case ERR_PASSWDMISMATCH  : send_error(s, c, ":Password incorrect\n");break;
		case ERR_YOUREBANNEDCREEP  : send_error(s, c, "You are banned from this server\n");break;
		case ERR_KEYSET  : send_error(s, c, m.getContent()[0] + ":Channel key already set\n");break;
		case ERR_CHANNELISFULL  : send_error(s, c, m.getContent()[0] + ":Cannot join channel (+l)");break;
		case ERR_UNKNOWNMODE  : send_error(s, c, m.getContent()[0] + ":is unknown mode char to me");break;
		case ERR_INVITEONLYCHAN  : send_error(s, c, m.getContent()[0] + ":Cannot join channel (+i)");break;
		case ERR_BANNEDFROMCHAN  : send_error(s, c, m.getContent()[0] + ":Cannot join channel (+b)");break;
		case ERR_BADCHANNELKEY  : send_error(s, c, m.getContent()[0] + ":Cannot join channel (+k)");break;
		case ERR_NOPRIVILEGES  : send_error(s, c, ":Permission Denied- You're not an IRC operator\n");break;
		case ERR_CHANOPRIVSNEEDED  : send_error(s, c, "");break; //TODO Cyr
		case ERR_CANTKILLSERVER  : send_error(s, c, ":You can't kill a server\n");break;
		case ERR_NOOPERHOST  : send_error(s, c, "No O-lines for your host\n");break;
		case ERR_UMODEUNKNOWNFLAG  : send_error(s, c, ":Unknown MODE flag\n");break;
		case ERR_USERSDONTMATCH  : send_error(s, c, ":Can't change mode for other users\n");break;
		case RPL_NONE  : send_error(s, c, "");break;
		case RPL_USERHOST  : send_error(s, c, "");break;
		case RPL_ISON  : send_error(s, c, "");break;
		case RPL_AWAY  : send_error(s, c, "");break;
		case RPL_UNAWAY  : send_error(s, c, "");break;
		case RPL_NOWAWAY  : send_error(s, c, "");break;
		case RPL_WHOISUSER  : send_error(s, c, "");break;
		case RPL_WHOISSERVER  : send_error(s, c, "");break;
		case RPL_WHOISOPERATOR  : send_error(s, c, "");break;
		case RPL_WHOISIDLE  : send_error(s, c, "");break;
		case RPL_ENDOFWHOIS  : send_error(s, c, "");break;
		case RPL_WHOISCHANNELS  : send_error(s, c, "");break;
		case RPL_WHOWASUSER  : send_error(s, c, "");break;
		case RPL_ENDOFWHOWAS  : send_error(s, c, "");break;
		case RPL_LISTSTART  : send_error(s, c, "");break;
		case RPL_LIST  : send_error(s, c, "");break;
		case RPL_LISTEND  : send_error(s, c, "");break;
		case RPL_CHANNELMODEIS  : send_error(s, c, "");break;
		case RPL_NOTOPIC  : send_error(s, c, "");break;
		case RPL_TOPIC  : send_error(s, c, "");break;
		case RPL_INVITING  : send_error(s, c, "");break;
		case RPL_SUMMONING  : send_error(s, c, "");break;
		case RPL_VERSION  : send_error(s, c, "");break;
		case RPL_WHOREPLY  : send_error(s, c, "");break;
		case RPL_ENDOFWHO  : send_error(s, c, "");break;
		case RPL_NAMREPLY  : send_error(s, c, "");break;
		case RPL_ENDOFNAMES  : send_error(s, c, "");break;
		case RPL_LINKS  : send_error(s, c, "");break;
		case RPL_ENDOFLINKS  : send_error(s, c, "");break;
		case RPL_BANLIST  : send_error(s, c, "");break;
		case RPL_ENDOFBANLIST  : send_error(s, c, "");break;
		case RPL_INFO  : send_error(s, c, "");break;
		case RPL_ENDOFINFO  : send_error(s, c, "");break;
		case RPL_MOTDSTART  : send_error(s, c, "");break;
		case RPL_MOTD  : send_error(s, c, "");break;
		case RPL_ENDOFMOTD  : send_error(s, c, "");break;
		case RPL_YOUREOPER  : send_error(s, c, "");break;
		case RPL_REHASHING  : send_error(s, c, "");break;
		case RPL_TIME  : send_error(s, c, "");break;
		case RPL_USERSSTART  : send_error(s, c, "");break;
		case RPL_USERS  : send_error(s, c, "");break;
		case RPL_ENDOFUSERS  : send_error(s, c, "");break;
		case RPL_NOUSERS  : send_error(s, c, "");break;
		case RPL_TRACELINK  : send_error(s, c, "");break;
		case RPL_TRACECONNECTING  : send_error(s, c, "");break;
		case RPL_TRACEHANDSHAKE  : send_error(s, c, "");break;
		case RPL_TRACEUNKNOWN  : send_error(s, c, "");break;
		case RPL_TRACEOPERATOR  : send_error(s, c, "");break;
		case RPL_TRACEUSER  : send_error(s, c, "");break;
		case RPL_TRACESERVER  : send_error(s, c, "");break;
		case RPL_TRACENEWTYPE  : send_error(s, c, "");break;
		case RPL_TRACELOG  : send_error(s, c, "");break;
		case RPL_STATSLINKINFO  : send_error(s, c, "");break;
		case RPL_STATSCOMMANDS  : send_error(s, c, "");break;
		case RPL_STATSCLINE  : send_error(s, c, "");break;
		case RPL_STATSNLINE  : send_error(s, c, "");break;
		case RPL_STATSILINE  : send_error(s, c, "");break;
		case RPL_STATSKLINE  : send_error(s, c, "");break;
		case RPL_STATSYLINE  : send_error(s, c, "");break;
		case RPL_ENDOFSTATS  : send_error(s, c, "");break;
		case RPL_STATSLLINE  : send_error(s, c, "");break;
		case RPL_STATSUPTIME  : send_error(s, c, "");break;
		case RPL_STATSOLINE  : send_error(s, c, "");break;
		case RPL_STATSHLINE  : send_error(s, c, "");break;
		case RPL_UMODEIS  : send_error(s, c, "");break;
		case RPL_LUSERCLIENT  : send_error(s, c, "");break;
		case RPL_LUSEROP  : send_error(s, c, "");break;
		case RPL_LUSERUNKNOWN  : send_error(s, c, "");break;
		case RPL_LUSERCHANNELS  : send_error(s, c, "");break;
		case RPL_LUSERME  : send_error(s, c, "");break;
		case RPL_ADMINME  : send_error(s, c, "");break;
		case RPL_ADMINLOC1  : send_error(s, c, "");break;
		case RPL_ADMINLOC2  : send_error(s, c, "");break;
		case RPL_ADMINEMAIL  : send_error(s, c, "");break;
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