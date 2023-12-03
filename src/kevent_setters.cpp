#include "irc.hpp"

void	server::write_unset(int fd)
{
	EV_SET(&(_changelist), fd, EVFILT_WRITE, EV_ADD | EV_DISABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error write_unset"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

void	server::write_set(int fd)
{
	EV_SET(&(_changelist), fd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error write_set"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

void	server::read_unset(int fd)
{
	EV_SET(&(_changelist), fd, EVFILT_READ, EV_ADD | EV_DISABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error read_unset"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

void	server::read_set(int fd)
{
	EV_SET(&(_changelist), fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
	if (kevent(_kq, &(_changelist), 1, NULL, 0, &(_timeout)) == -1)
	{
		std::cerr<<"kevent error read_set"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		this->safe_shutdown(EXIT_FAILURE);
	}
}

void server::update_timer(int fd, size_t time)
{
	EV_SET(&_changelist, fd, EVFILT_TIMER, EV_ADD | EV_ENABLE | EV_CLEAR | EV_ONESHOT, 0, time, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when resetting timer"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

void server::delete_timer(int fd)
{
	EV_SET(&_changelist, fd, EVFILT_TIMER, EV_DELETE , 0, 0, NULL);
	if (kevent(_kq, &_changelist, 1, NULL, 0, &_timeout) == -1)
	{
		std::cerr<<"kevent error when deleting timer"<<std::endl
			<<"error: "<<strerror(errno)<<std::endl;
		safe_shutdown(EXIT_FAILURE);
	}
}

