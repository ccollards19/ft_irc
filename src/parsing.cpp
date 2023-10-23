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
public:
	Message(std::string msg, std::map<std::string, int> commands);
	bool isCommand(){return _command;};
	~Message(){};

};
//
//long int cmd2int(std::string s)
//{
//	if (s == "KICK") {return 1;}
//	if (s == "INVITE") {return 2;}
//	if (s == "TOPIC") {return 3;}
//	if (s == "MODE") {return 4;}
//	if (s == "NICK") {return 5;}
//	if (s == "PRIVMSG") {return 6;}
//	if (s == "JOIN") {return 7;}
//	if (s == "PING") {return 8;}
//	return 0;
//
//}

Message::Message(std::string msg, std::map<std::string, int> commands) {
	int pos = msg.find(" ");
	std::string fword = msg.substr(0, pos);
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
		default: _command = NONE;
	}
	if (_command)
		msg.erase(0, pos + 1);
	else
		_content.push_back(fword);
	while (pos != std::string::npos)
	{
		pos = msg.find(" ");
		_content.push_back(msg.substr(0, pos));
		msg.erase(0, pos + 1);
	}

}
/*
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
	Message test("KICK nico pretty please", cmds);
	std::cout << test.isCommand() << "\n";
	Message test2("", cmds);
	std::cout << test2.isCommand() << "\n";
	Message test3("TOPIC salut ca va ?", cmds);
	std::cout << test3.isCommand() << "\n";
	Message test4("kickmoistp", cmds);
	std::cout << test4.isCommand() << "\n";
}
*/