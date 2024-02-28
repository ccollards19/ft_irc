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
		case USER:return "USER";
		case PART:return "PART";
		case PASS:return "PASS";
		case QUIT:return "QUIT";
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
	while (msg.find("\n") != std::string::npos)
		msg.erase(msg.find("\n"));
	while (msg.find("\r") != std::string::npos)
		msg.erase(msg.find("\r"));
	if (msg.find(" ") == std::string::npos) {pos = msg.size();}
	std::string fword = msg.substr(0, pos);
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
	int cmd;
	try{cmd = commands.at(fword);}
	catch (std::exception &e) {cmd = 0;}
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
		case PASS: _command = PASS; break;
		case USER : _command = USER;break;
		case PART : _command = PART;break;
		case BAN : _command = BAN;break;
		case QUIT : _command = QUIT;break;
		default: _command = NONE;
	}
	if (_command)
		msg.erase(0, pos + 1);
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
	unsigned long pos2;
	while ((pos = c->_receive_buffer.find('\n')) != std::string::npos) {
		Message msg((c->_receive_buffer).substr(0, pos + 1), s->_cmds);
		c->_receive_buffer.erase(0, (pos + 1));
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
				case KICK:s->kick(msg, c);break;
				case TOPIC:s->topic(msg, c);break;
				case MODE:s->mode(msg, c);break;
				case INVITE: s->invite(msg, c);break;
				case PRIVMSG: s->privmsg(msg, c);break;
				case JOIN:
					if (msg.getContent().size() >= 1 && msg.getContent()[0].find(',') != std::string::npos)
					{
						std::string chans = msg.getContent()[0] + ",";
						std::string passwords;
						if (msg.getContent().size() > 1)
							passwords = msg.getContent()[2] + ",";
						else
							passwords = "";
						while ((pos = chans.find(',')) != std::string::npos) {
							std::string message("JOIN :" + chans.substr(0, pos));
							if (!passwords.empty()){
								pos2 = passwords.find(',');
								message += " " + passwords.substr(0, pos2);
								passwords.erase(0, pos + 1);
							}
							Message mjoin(message + "\0", s->_cmds);
							s->join(mjoin, c);
							chans.erase(0, (pos + 1));
						}
					}
					else{
						s->join(msg, c);

					}

					break;
				case PING:s->ping(msg, c);break;
				case PONG:s->pong(msg, c);break;
				case NICK:s->nick(msg, c);break;
				case PASS: s->pass(msg, c);break;
				case USER:s->user(msg, c);break;
				case PART:s->part(msg, c);break;
				case QUIT:s->quit(msg, c);break;
				default:break;
			}
		}
	}
}
