#include "Headers.hpp"

Headers::Headers(void) {}

Headers::Headers(Headers const &Headers) { *this = Headers; }

Headers::~Headers(void) {}

Headers const &Headers::operator=(Headers const &Headers)
{
	this->_host		  = Headers._host;
	this->_connection = Headers._connection;
	this->_user_agent = Headers._user_agent;
	this->_accept	  = Headers._accept;
	this->_cookie	  = Headers._cookie;
	this->_else		  = Headers._else;
	return (*this);
}

void Headers::clear(void)
{
	this->_host.clear();
	this->_connection.clear();
	this->_user_agent.clear();
	this->_accept.clear();
	this->_cookie.clear();
	this->_else.clear();
}
