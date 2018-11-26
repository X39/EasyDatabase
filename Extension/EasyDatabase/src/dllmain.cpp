#include "dllmain.h"
#include <string>
#include <sstream>
#include <string_view>
#include <algorithm>
#include <cctype>
#include <map>
#include <rapidxml/rapidxml.hpp>
#include <exception>
#include <fstream>
#include <vector>
#include "omemstream.h"


bool xml_get_bool(const rapidxml::xml_node<>* node, bool def)
{
	std::string val = node->value();
	if (val.empty())
	{
		return def;
	}
	if (val[0] == '0' || val[0] == 'F' || val[0] == 'f')
	{
		return false;
	}
	else
	{
		return true;
	}
}
void dllmain::load_config()
{
	try
	{
		rapidxml::xml_document doc;
		std::ifstream file("easydatabase.xml");
		std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		buffer.push_back('\0');
		doc.parse<0>(buffer.data());
		auto root_node = doc.first_node("MyBeerJournal");

		// Get Configuration
		m_allowNonPrepared = xml_get_bool(root_node->first_node("allownonprepared"), false);

		// Get Connections
		for (auto connection_node = root_node->first_node("connection"); connection_node; connection_node = connection_node->next_sibling())
		{
			easydatabase::connection con;
			con.name = connection_node->first_attribute("name")->value();
			con.uri = connection_node->first_node("uri")->value();
			con.database = connection_node->first_node("database")->value();
			con.username = connection_node->first_node("username")->value();
			con.password = connection_node->first_node("password")->value();
			m_connections.push_back(con);
		}

		// Get Prepared Statements
		for (auto statement_node = root_node->first_node("statement"); statement_node; statement_node = statement_node->next_sibling())
		{
			easydatabase::prepared_statement stmnt;
			stmnt.name = statement_node->first_attribute("name")->value();
			stmnt.query = statement_node->value();
			m_prepared_statements.push_back(stmnt);
		}
	}
	catch(std::exception ex) { }
}

dllmain& dllmain::instance()
{
	static dllmain instance;
	return instance;
}
std::map<std::string, rvextension_callback>& dllmain::callbacks()
{
	static std::map<std::string, rvextension_callback> command_map = {
		{ "find_prepared_statement", find_prepared_statement },
		{ "find_connection", find_connection },
		{ "open_connection", open_connection },
		{ "close_connection", close_connection },
		{ "create_operation_set", create_operation_set },
		{ "close_operation_set", close_operation_set },
		{ "execute", execute },
		{ "execute_statement", execute_statement },
		{ "execute_query", execute_query },
		{ "execute_query_statement", execute_query_statement },
		{ "execute_update", execute_update },
		{ "execute_update_statement", execute_update_statement },
		{ "next", next },
		{ "clear", clear }
	};
	return command_map;
}

//--- Called by Engine on extension load 
DLLEXPORT void CALLSPEC_STD RVExtensionVersion(char *output, int outputSize)
{
	dllmain::instance().load_config();
	std::strncpy(output, "EasyDatabase 1.0", outputSize);
}
//--- STRING callExtension STRING
DLLEXPORT void CALLSPEC_STD RVExtension(char *output, int outputSize, const char *function)
{
	std::strncpy(output, "Using 'STRING callExtension STRING' is not supported.", outputSize);
}
//--- STRING callExtension ARRAY
DLLEXPORT int CALLSPEC_STD RVExtensionArgs(char *output, int outputSize, const char *function, const char **argv, int argc)
{
	omemstream out(output, outputSize);
	std::string fnc = function;
	std::transform(fnc.begin(), fnc.end(), fnc.begin(), std::tolower);
	auto res = dllmain::callbacks().find(fnc);
	if (res == dllmain::callbacks().end())
	{
		return -1;
	}
	else
	{
		std::vector<std::string_view> args;
		for (int i = 0; i < argc; i++)
		{
			args.push_back(argv[i]);
		}
		return (*res).second(out, args);
	}
}
