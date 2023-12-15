#ifndef IRC_HPP
#define IRC_HPP
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
#include <iterator>
#include <string>
#include <map>
#include <vector>
#include <netdb.h>
#include "errors.hpp"
#ifndef DEBUG
 #define DEBUG 0
#endif
#define SERVNAME "irc.ircserv.test"
#define CLIENT_TTL 50000
#define REG_TASK_TIMER 10000
enum cmds
{
	NONE,
	KICK,
	INVITE,
	TOPIC,
	MODE,
	NICK,
	PRIVMSG,
	JOIN,
	PING,
	PONG,
	PASS,
	USER,
	PART,
	BAN,
	QUIT,
};

class Message
{
	std::vector<std::string> _content;
	cmds _command;
	std::string _nick;
	std::string _user;
	std::string _host;
public:
	Message(std::string msg, std::map<std::string, int> commands);
	bool isCommand(){return _command;};
	std::string getNick(){return _nick;};
	std::string getUser(){return _user;};
	std::string getHost(){return _host;};
	int getCommand(){return _command;};
	std::vector<std::string> getContent(){return _content;};
	std::string getCommandName();
	~Message(){};
	void showContent();

};
struct client;
struct channel
{
	std::string _chan_id; //maybe use another type ?
  /* Channels names are strings (beginning with a '&', '#', '+' or '!'
   * character) of length up to fifty (50) characters.  Channel names are
   * case insensitive
   */
	char _prefix;
	struct client *_creator;
	size_t _capacity;
	std::vector<struct client *> _members;
	std::vector<struct client *> _operators;
	std::vector<struct client *> _invite_list;
	std::vector<struct client *> _ban_list;
	std::string _mode;
	std::string _name;
	std::string _topic;
	std::string _pwd;
	bool isInvited(client *c);
	void removeInvited(struct client *client);
	void addClient(client *client);
	bool isModeSet(char mode);

};

struct client
{
	//networking
	int			_fd;
	bool    _ping;
  struct sockaddr _sockaddr;
  socklen_t _socklen;
	//user related
	bool _isRegistered;
	std::string _nickname;
	std::string _username;
	std::string _hostname;
	std::string _servername;
	std::string _realname;
	std::string _mode;
	bool _pass;
	//buffers
	std::string _send_buffer;
	std::string _receive_buffer;
	//member functions  
	bool    isRegistered();
	bool isChanop(channel *c);
	bool isMember(channel *c);
};

struct server
{
	//general
	std::string _servername;
	std::string _password;
	std::string _creation_date;
	std::map<int, client *>	_connections;
	std::map<std::string, client *>	_nick_map;
	std::vector<channel *> _chan_list;
	std::map<std::string, int> _cmds;
	//socket related
  struct addrinfo _hints;
  struct addrinfo *_res;
  struct addrinfo *_res_start;
	int			_socketfd;
	//kevent related
	struct kevent		_changelist;
	struct kevent		_eventlist;
	struct timespec 	_timeout;
	int 			_kq;
	//member functions
	void			init(char **argv);
  void      network_init(char *port);
  void      kevent_init();
	void			run();
	void			server_admin();
	void			add_connection();
	void      regular_tasks();
	void			safe_shutdown(int exit_code);
	void			check_connection(struct client *client);
	void			close_connection(struct client *client);
	//kevent manip
	void			update_timer(int fd, size_t time);
	void 			delete_timer(int fd);
	void			write_unset(int fd);
	void			write_set(int fd);
	void			read_unset(int fd);
	void			read_set(int fd);
	//actions
	void			send_message();
	void			receive_message();
	void register_client(Message &m, struct client *client);
	bool isAchannel(std::string name);
	std::vector<struct channel*>::iterator getChannel(std::string channelName);
	bool checkChannel(std::string channelName);
	struct channel *createChannel(std::string channelName, struct client *client);
  void removeChannel(struct channel *channel);
	void joinMessage(channel *target, client *c);
	void chanMessage(channel *target, client *c, std::string msg);
  //commands
	void ping(Message &m, struct client *client);
	void pong(Message &m, struct client *client);
	void pass(Message &m, struct client *client);
	void nick(Message &m, struct client *client);
	void user(Message &m, struct client *client);
  	void kick(Message &m, struct client *client);
	void mode(Message &m, struct client *client);
	void join(Message &m, struct client *client);
	void topic(Message &m, struct client *client);
	void quit(Message &m, client *quitting_client);
	void privmsg(Message &m, client *client);
	void part(Message &m, client *client);
	int ErrMode(Message &m, client *c, int part, channel *chan);
	int modeK(Message &m, client *client, channel *c);
	int modeO(Message &m, client *client, channel *c);
	int modeI(Message &m, client *client, channel *c);
	int modeL(Message &m, client *client, channel *c);
	int modeT(Message &m, client *client, channel *c);
  	void invite(Message &m, struct client *client);

	void removeMember(struct client *client, struct channel *chan);
};


void parse(struct server *s, struct client *c);
//CONTROL PANEL
void send_reply(struct server &s,struct client &c, std::string message);
void send_error(struct server &s,struct client &c, std::string message);
struct channel *get_channel(struct server s, std::string name);
std::string get_mask_list(struct server s, std::string name, char flag);
struct client *get_operator(struct server s, std::string name);
void reply(Message &m, struct server &s, struct client &c, int error);
void exemple(Message &m, struct server &s, struct client &c, int error);

#endif
