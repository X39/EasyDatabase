#include "functions.h"
#include "dllmain.h"
#include <algorithm>
#include <string>
#include <exception>
#include <array>
namespace
{
	bool try_parse(std::string_view str, int& out)
	{
		return try_parse(std::string(str.data()), out);
	}
	bool try_parse(std::string str, int& out)
	{
		try
		{
			out = std::stoi(str);
			return true;
		}
		catch (std::exception ex)
		{
			return false;
		}
	}
}
returntype find_prepared_statement(omemstream& out, std::vector<std::string_view>& args)
{
	if (args.size() != 1)
	{
		out << "[false,\"" << "Expected exactly one argument of type STRING." << "\"]";
		return ret_arg_err;
	}
	auto name = args[0];
	auto res = std::find_if(
		dllmain::instance().prepared_statements().begin(),
		dllmain::instance().prepared_statements().end(),
		[name](easydatabase::prepared_statement& it) -> bool {
		return it.name == name;
	});
	if (res == dllmain::instance().prepared_statements().end())
	{
		out << "[false,-1]";
		return ret_ok;
	}
	else
	{
		out << "[true," << (res - dllmain::instance().prepared_statements().begin()) << "]";
		return ret_ok;
	}
}
returntype find_connection(omemstream& out, std::vector<std::string_view>& args)
{
	if (args.size() != 1)
	{
		out << "[false,\"" << "Expected exactly one argument of type STRING." << "\"]";
		return ret_arg_err;
	}
	auto name = args[0];
	auto res = std::find_if(
		dllmain::instance().connections().begin(),
		dllmain::instance().connections().end(),
		[name](easydatabase::connection& it) -> bool {
		return it.name == name;
	});
	if (res == dllmain::instance().connections().end())
	{
		out << "[false,-1]";
		return ret_ok;
	}
	else
	{
		out << "[true," << (res - dllmain::instance().connections().begin()) << "]";
		return ret_ok;
	}
}


returntype open_connection(omemstream& out, std::vector<std::string_view>& args)
{
	if (args.size() != 1)
	{
		out << "[false,\"" << "Expected exactly one argument of type SCALAR." << "\"]";
		return ret_arg_err;
	}
	// PARAM 1
	int connectionId;
	if (!try_parse(args[0], connectionId))
	{
		out << "[false,\"" << "First argument is not of type SCALAR. Expected UniqueID of connection." << "\"]";
		return ret_arg_err;
	}

	out << "[false,\"" << "NOT IMPLEMENTED." << "\"]";
	return ret_not_implemented;
}
returntype close_connection(omemstream& out, std::vector<std::string_view>& args)
{
	if (args.size() != 1)
	{
		out << "[false,\"" << "Expected exactly one argument of type SCALAR." << "\"]";
		return ret_arg_err;
	}
	// PARAM 1
	int connection;
	if (!try_parse(args[0], connection))
	{
		out << "[false,\"" << "First argument is not of type SCALAR. Expected connection identifier." << "\"]";
		return ret_arg_err;
	}

	out << "[false,\"" << "NOT IMPLEMENTED." << "\"]";
	return ret_not_implemented;
}
returntype create_operation_set(omemstream& out, std::vector<std::string_view>& args) {}
returntype close_operation_set(omemstream& out, std::vector<std::string_view>& args) {}
returntype execute(omemstream& out, std::vector<std::string_view>& args) {}
returntype execute_statement(omemstream& out, std::vector<std::string_view>& args) {}
returntype execute_query(omemstream& out, std::vector<std::string_view>& args) {}
returntype execute_query_statement(omemstream& out, std::vector<std::string_view>& args) {}
returntype execute_update(omemstream& out, std::vector<std::string_view>& args) {}
returntype execute_update_statement(omemstream& out, std::vector<std::string_view>& args) {}
returntype next(omemstream& out, std::vector<std::string_view>& args) {}
returntype clear(omemstream& out, std::vector<std::string_view>& args) {}