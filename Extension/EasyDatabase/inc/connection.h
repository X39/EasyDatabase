#pragma once
#include <string>

namespace easydatabase
{
	struct connection
	{
	public:
		std::string name;
		std::string uri;
		std::string database;
		std::string username;
		std::string password;
	};
}