#include <cstddef>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <unistd.h>
#include <iostream>
#include <string>
#include <map>
#include <cmath>
#include <vector>
enum cmd
{
	NONE = 0,
	KICK,
	INVITE,
	TOPIC,
	MODE,
	NICK,
	PRIVMSG,
	JOIN,
	PING
};
class Message
{
	std::vector<std::string> _content;
	cmd _command;
	std::string _nick;
	std::string _user;
	std::string _host;
public:
	Message(std::string &msg, std::map<std::string, int> commands);
	bool isCommand(){return _command;};
	std::string getNick(){return _nick;};
	std::string getUser(){return _user;};
	std::string getHost(){return _host;};
	int getCommand(){return _command;};
	std::vector<std::string> getContent(){return _content;};
	~Message(){};

};

Message::Message(std::string &msg, std::map<std::string, int> commands) {
	int pos;

	pos = msg.find(" ");
	pos == 0xffffffffffffffff ? pos = msg.size() : pos = pos;
	std::cout << pos << "\n";
	std::string fword = msg.substr(0, pos);
	std::cout << fword << "\n";
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
	std::cout << fword << "\n";
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
		default: /*std::cout << fword << " is not a command\n"; is a test*/_command = NONE;
	}
	if (_command)
		msg.erase(0, pos + 1);
	else
		_content.push_back(fword);
	do
	{
		std::cout << "current head ->" << msg << "\n";
		if (msg.size() && toascii(msg.at(0)) == ':')
		{
			msg.erase(0, 1);
			_content.push_back(msg);
			msg.erase(0, msg.size());
		}
		else
		{
			pos = msg.find(" ");
			pos == 0xffffffffffffffff ? _content.push_back(msg.substr(0, msg.size())) : _content.push_back(msg.substr(0, pos));
			pos == 0xffffffffffffffff ? msg.erase(0, msg.size()) : msg.erase(0, pos + 1);
		}

	}while (pos != std::string::npos);

}
void display(std::vector<std::string> s)
{
	std::vector<std::string>::iterator i = s.begin();
	while (i != s.end())
	{
		std::cout << *i++ << "\n";
	}
}

int main()
{
	std::map<std::string, int> cmds;
	cmds["KICK"] = KICK;
	cmds["TOPIC"] = TOPIC;
	cmds["MODE"] = MODE;
	cmds["INVITE"] = INVITE;
	cmds["NICK"] = NICK;
	cmds["PRIVMSG"] = PRIVMSG;
	cmds["JOIN"] = JOIN;
	cmds["PING"] = PING;

	std::string s1("KICK nico pretty please");
	std::string s2(":nico!nicolas@user KICK Merlin");
	std::string s3("PRIVMSG Cyrdu98 :salut ca va ?");
	std::string s4("kickmoistp");

	//Message test(s1, cmds);
	Message test2(s2, cmds);
	//Message test3(s3, cmds);
	//Message test4(s4, cmds);

	std::cout << "messages: ["<< s1 << ","<<  s2<< "," << s3<< "," << s4 << "]\n";

	//display(test.getContent());
	//display(test2.getContent());
	display(test2.getContent());
	//display(test4.getContent());
}