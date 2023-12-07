#include "irc.hpp"

std::string Message::getCommandName() {
	switch (_command) {
		case KICK:return "KICK";
		case TOPIC:return "TOPIC";
		case MODE:return "MODE";
		case INVITE:return "INVITE";
		case PRIVMSG:return "PRIVMSG";
		case JOIN:return "JOIN";
		case PING:return "PING";
		case PONG:return "PONG";
		case NICK:return "NICK";
		case BAN:return "BAN";
		case USER:return "USER";
		case PART:return "PART";
		default: return "";
	}
}

void Message::showContent()
{
	std::cout << "Message content:\n";
	std::vector<std::string>::iterator i = _content.begin();
	for (; i != _content.end() ; ++i) {
		std::cout << "[" + *i + "]" << " ";
	}
	std::cout << std::endl;

}
Message::Message(std::string msg, std::map<std::string, int> commands) {
	size_t pos;

	pos = msg.find(" ");
	//std::cout << "DEBUG "<< msg << "\n";
	while (msg.find("\n") != std::string::npos)
		msg.erase(msg.find("\n"));
	while (msg.find("\r") != std::string::npos)
		msg.erase(msg.find("\r"));
	//std::cout <<"DEBUG2 "<< msg << "\n";
	if (msg.find(" ") == std::string::npos) {pos = msg.size();}
	std::string fword = msg.substr(0, pos);
	//check if it is a prefix and setup the next word as the first word
	if (fword.size() && toascii(fword.at(0)) == ':')
	{
		try
		{
			_nick = fword.substr(1, fword.find("!") - 1);
			_user = fword.substr(fword.find("!") + 1, fword.find("@") - fword.find("!") - 1);
			_host = fword.substr(fword.find("@") + 1, fword.size() - 1);
		}
		catch (std::exception &e){}
		msg.erase(0, pos + 1);
		pos = msg.find(" ");
		fword = msg.substr(0, pos);
	}
	//check if it is a valid command and erase the first word if it is. setup the command enum
	int cmd;
	try{cmd = commands.at(fword);}
	catch (std::exception &e) {cmd = 0;}
	std::cout << cmd << std::endl;
	switch (cmd) {
		case KICK: _command = KICK; break;
		case INVITE : _command = INVITE; break;
		case TOPIC : _command = TOPIC; break;
		case MODE : _command = MODE; break;
		case NICK : _command = NICK; break;
		case PRIVMSG : _command = PRIVMSG; break;
		case JOIN : _command = JOIN; break;
		case PING : _command = PING; break;
		case PONG : _command = PONG; break;
		case BAN : _command = BAN; break;
		case PASS: _command = PASS; break;
		case USER : _command = USER;break;
		case PART : _command = PART;break;
		default: _command = NONE;
	}
	if (_command)
		msg.erase(0, pos + 1);
	//loop through the content of the message. if a word start with ':' it is considered as one argument
	while (pos)
	{
		if (msg.size() && toascii(msg.at(0)) == ':')
		{
			msg.erase(0, 1);
			_content.push_back(msg);
			msg.erase(0, msg.size());
			pos = 0;
		}
		else
		{
			pos = msg.find(" ");
			bool end = msg.find(" ") == std::string::npos;
			end ? _content.push_back(msg.substr(0, msg.size())) : _content.push_back(msg.substr(0, pos));
			end ? msg.erase(0, msg.size()) : msg.erase(0, pos + 1);
			if (end)
				pos = 0;
		}
	}
}

void parse(struct server *s, struct client *c) {
	unsigned long pos;
	while ((pos = c->_receive_buffer.find('\n')) != std::string::npos) {
		// std::cout << "PARSING: " + c->_receive_buffer << "\n";
		Message msg((c->_receive_buffer).substr(0, pos + 1), s->_cmds);
		// std::cout << "ERASE: ["  << c->_receive_buffer << "]\n";
		c->_receive_buffer.erase(0, (pos + 1));
		// std::cout << "ERASE: ["  << c->_receive_buffer << "]\n";
		// std::cout << "command : [" << msg.getCommand() << "] "<< msg.getCommandName() << "\n";
		// msg.showContent();
		if (!c->_isRegistered) {
			switch (msg.getCommand()) {
				case NICK:s->nick(msg, c);break;
				case PASS: s->pass(msg, c);break;
				case USER:s->user(msg, c);break;
				default:break;
			}
		}
		else {
			switch (msg.getCommand()) {
				case KICK:s->kill(msg, c);break;
				case TOPIC:s->topic(msg, c);break;
				case MODE:s->mode(msg, c);break;
					  //case INVITE: s->invite(msg, c);break;
				case PRIVMSG: s->privmsg(msg, c);break;
				case JOIN:s->join(msg, c);break;
				case PING:s->ping(msg, c);break;
				case PONG:s->pong(msg, c);break;
				case NICK:s->nick(msg, c);break;
					  //case BAN: s->ban(msg, c);break;
				case PASS: s->pass(msg, c);break;
				case USER:s->user(msg, c);break;
				case PART:s->part(msg, c);break;
				default:break;
			}
		}
	}
}


//
//void display(std::vector<std::string> s)
//{
//	std::cout << "[";
//	std::vector<std::string>::iterator i = s.begin();
//	while (i != s.end())
//	{
//		std::cout << "|" + *i++ + "|" << " + ";
//	}
//	std::cout << "]\n";
//}
//
//void display_command(int cmd)
//{
//	std::cout << "command: ";
//	switch (cmd) {
//		case NONE:std::cout << "NONE\n";break;
//		case KICK:std::cout << "KICK\n";break;
//		case TOPIC:std::cout << "TOPIC\n";break;
//		case MODE:std::cout << "MODE\n";break;
//		case INVITE:std::cout << "INVITE\n";break;
//		case PRIVMSG:std::cout << "PRIVMSG\n";break;
//		case JOIN:std::cout << "JOIN\n";break;
//		case PING:std::cout << "PING\n";break;
//	}
//}
//
//int main()
//{
//	std::map<std::string, int> cmds;
//	cmds["KICK"] = KICK;
//	cmds["TOPIC"] = TOPIC;
//	cmds["MODE"] = MODE;
//	cmds["INVITE"] = INVITE;
//	cmds["NICK"] = NICK;
//	cmds["PRIVMSG"] = PRIVMSG;
//	cmds["JOIN"] = JOIN;
//	cmds["PING"] = PING;
//
//	std::string s1("KICK nico pretty please");
//	std::string s2(":nico!nicolas@user KICK Merlin");
//	std::string s3("PRIVMSG Cyrdu98 :salut ca va ?");
//	std::string s4("kickmoistp");
//
//	Message test(s1, cmds);
//	Message test2(s2, cmds);
//	Message test3(s3, cmds);
//	Message test4(s4, cmds);
//
//	std::cout << "cleared messages: ["<< s1 << ","<<  s2<< "," << s3<< "," << s4 << "]\n";
//	display_command(test.getCommand());
//	display(test.getContent());
//	display_command(test2.getCommand());
//	display(test2.getContent());
//	display_command(test3.getCommand());
//	display(test3.getContent());
//	display_command(test4.getCommand());
//	display(test4.getContent());
//}
