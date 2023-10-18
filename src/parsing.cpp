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
	KICK = 31946792,
	INVITE = 112129322959,
	TOPIC = 1379041673,
	MODE = 22987871,
	NICK = 31946795,
	PRIVMSG = 9424822412554
};
class Message
{
	bool _isCommand;
	std::vector<std::string> _content;
	cmd _command;
public:
	Message(std::string msg);
	~Message(){};

};

long int StringToInt(std::string s, long int result, int n)
{
	if (n >= s.length())
		return result;
	result += pow(toascii(s.at(n)), n + 1);
	return (StringToInt(s, result, ++n));
}
Message::Message(std::string msg) {
	int pos = msg.find(" ");
	std::string fword = msg.substr(0, pos);
	long int cmd = StringToInt(fword, 0, 0);
//	std::cout << StringToInt("KICK", 0, 0) << std::endl;
//	std::cout << StringToInt("INVITE", 0, 0) << std::endl;
//	std::cout << StringToInt("TOPIC", 0, 0) << std::endl;
//	std::cout << StringToInt("MODE", 0, 0) << std::endl;
//	std::cout << StringToInt("NICK", 0, 0) << std::endl;
//	std::cout << StringToInt("PRIVMSG", 0, 0) << std::endl;
	switch (cmd) {
		case 31946792: _command = KICK; break;
		case 112129322959 : _command = INVITE; break;
		case 1379041673 : _command = TOPIC; break;
		case 22987871 : _command = MODE; break;
		case 31946795 : _command = NICK; break;
		case 9424822412554 : _command = PRIVMSG; break;
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
	_isCommand = _command;

}
//int main()
//{
//	Message test("KICK nico pretty please");
//	Message test2("");
//	Message test3("salut ca va ?");
//	Message test4("kickmoistp");
//}
