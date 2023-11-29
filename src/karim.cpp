#include "main.cpp"

const std::string& client::getNickname() const
{
	return (_nickname);
}


//                 PASS

bool    client::isRegistered(){
    return (_isRegistered); //must be set at FALSE
}

int server::PASS(Message &m, client &client){
    std::vector<std::string> params = m.getContent();
    if (params.size() < 2)
        return (ERR_NEEDMOREPARAMS);
    else if (client.isRegistered())
        return (ERR_ALREADYREGISTRED);
    else {
        if (params == server._password)
            client._isRegistered = TRUE;
    }
    return 0;
}

//                  NICK

int server::NICK(Message &m, client &client){
    std::vector<std::string> params = m.getContent();
    if (params.size == 0)
        return(ERR_NONICKNAMEGIVEN);
    if (params.size > 1)
        return (ERR_ERRONEUSNICKNAME);
    try { 
        server._nick_map.at(params[0]);
        return (ERR_NICKNAMEINUSE);
    }
    catch (std::exception &e){
        server._nick_map[params[0]] = client;
        client._nickname = params[0];
    }
    return 0;
}

//                  USER

int server::USER(Message &m, client &client){
    std::vector<std::string> params = m.getContent();
    std::string realName;
    if (client.getNickname().empty())
        return (ERR_NONICKNAMEGIVEN);
    if (params.size() < 4)
        return (ERR_NEEDMOREPARAMS);
    if (client.isRegisteredU == TRUE)
        return (ERR_ALREADYREGISTRED);
    client._username = params[0];
    if (params.size() > 4)
    {
        if (!realName.empty)
            realName.erase(realName.size() - 1);
        for (int i = 3; i < params.size(); i++)
            realName += params[i] + " ";
        //RPL_WELCOME ?
    }
    return 0;
}

//                  OPER

int server::OPER(Message &m, client &client) {
    std::vector<std::string> params = m.getContent();
    if (params.size() < 3)
        return (ERR_NEEDMOREPARAMS);
    std::string password = params.at(2);
    if (this->_oper_pswd != password)
        return (ERR_PASSWDMISMATCH);
    else {
        client::_is_oper = true;
        return (RPL_YOUREOPER);
    }
}

//                  KILL

int server::KILL(Message &m, client &client) {
    std::vector<std::string> params = m.getContent();
    int i = 0;
    int check = 0;
    if (client::_is_oper == false)
        return (ERR_NOPRIVILEGES);
    if (params.size() < 2)
        return (ERR_NEEDMOREPARAMS);
    for (i = 0; i < _nick_map.size(); i++) {
        if (params.at(1) == _nick_map.at(i).getNickname()) {
            check = 1;
            break ;
        }
    }
    if (check == 0)
        return (ERR_NOSUCHNICK);
    server::close_connection(_nick_map.at(i));
}

//                  MODE

int server::MODE(Message &m, client &client){

}

//                  Join

void	channel::removeInvited(client client) {
	for (std::vector<std::string>::iterator it = _invited.begin(); it != _invited.end(); ++it) {
		if (*it == client.getNickname()) {
			_invited.erase(it);
			return;
		}
	}
}

void channel::addClient(client client) {
	_members.push_back(client);
}


bool	channel::isModeSet(char mode) {
	for (std::vector<char>::iterator it = this->_mode.begin(); it != this->_mode.end(); ++it) {
		if (*it == mode)
			return (true);
	}
	return (false);
}

std::vector<channel>::iterator	Server::getChannel(std::string channelName) {
	for(std::vector<channel>::iterator it = _chan_list.begin(); it != _chan_list.end(); ++it) {
		if (it->_name == channelName)	{
			return (it);
		}
	}
	return (_channels.end());
}

bool	server::checkChannel(std::string channelName, client &client){
	std::vector<channel>::iterator it;
	for(it = server::_chan_list.begin(); it != server::_chan_list.end(); ++it) {
		if (it->_name == channelName) {
			if (!it->isModeSet('i'))
            it->addClient(client);
			return (true);
		}
	}
	return (false);
}

int server::createChannel(string channelName, client &client){
    Channel newChannel(channelName, client);// A coder
    server::_chan_list.pushback(channelName);
    // Message à print. Pas sur de ce que c'est
}


int server::JOIN(Message &m, client &client){
    std::vector<std::string> params = m.getContent();
    std::string channelName = params[0];
    //verif
    //if 0 params return (ERR_NEEDMOREPARAMS)
    //if client is registered ? return (ERR_NOTREGISTERED) possible ?
    //if not in server::_chan_list create new channel
    if (params[0][0] == '#'){
        if (!checkChannel(channelName))
            createChannel(channelName, client);
        //chan invite only ?
        if (!itChannel->isModeSet('i'))
			return (RPL_JOIN);
        else {
            if (itChannel->isInvited(client.getNickname())) {
				//itChannel->addClient(client); A coder
				itChannel->removeInvited(client.getNickname());
				//rpl_Join(client, *itChannel); Commande à créer ou fonction de Nico gère ?
			}
			else {
				return (ERR_INVITEONLYCHAN)
			}
        }
    }
}