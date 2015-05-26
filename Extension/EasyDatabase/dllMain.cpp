#define _VERSION_ "BETA-0.1.0"
#include "dllMain.h"
#include "dotX39\DocumentReader.h"
#include "dotX39\DataString.h"
#include "dotX39\DataBoolean.h"
#include "Connection.hpp"
#include "PreparedStatement.hpp"
#include "sqf\Array.h"
#include "sqf\Command.h"


#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// Standad C++ includes
#include <vector>
using namespace std;

// Include the Connector/C++ headers
#ifdef _DEBUG
#define CONNECTION_CHECK_TIMEOUT 100
#else
#define CONNECTION_CHECK_TIMEOUT 30000
#endif

static struct structConfig {
	std::vector<Connection*> connections;
	std::vector<PreparedStatement*> preparedStatements;
	bool doToUpperForCommands;
	bool doCheckParams;
	bool allowNonPreparedSql;
	structConfig()
	{
		doToUpperForCommands = true;
		doCheckParams = true;
		allowNonPreparedSql = true;
	}
	~structConfig()
	{
		clear();
	}
	int getUniqueIdOfConnection(const char* name)
	{
		int uniqueId = -1;
		for (int i = 0; i < connections.size(); i++)
		{
			if (connections[i]->getName().compare(name) == 0)
			{
				uniqueId = i;
				break;
			}
		}
		return uniqueId;
	}
	int getUniqueIdOfPreparedStatements(const char* name)
	{
		int uniqueId = -1;
		for (int i = 0; i < preparedStatements.size(); i++)
		{
			if (preparedStatements[i]->getName().compare(name) == 0)
			{
				uniqueId = i;
				break;
			}
		}
		return uniqueId;
	}
	void clear(void)
	{
		for (auto& it : this->connections)
			delete it;
		for (auto& it : this->preparedStatements)
			delete it;
		this->connections.clear();
		this->preparedStatements.clear();
	}
} g_config;
static std::vector<sqf::Command>	g_commands;

void toUpper(std::string& s)
{
	for (int i = s.length - 1; i >= 0; i--)
		s[i] = toupper(s[i]);
}
void toUpper(char* s)
{
	for (int i = 0; s[i] != '\0'; i++)
		s[i] = toupper(s[i]);
}
void addCommands(void)
{
	g_commands.push_back(
		sqf::Command(
			"ABOUT",
			[](sqf::Array* arr)
			{
				return std::string("[TRUE,\"\",\"").append("EasyDatabase is an extension for ArmA 3 developed and maintained by X39.").append("\"]");
			},
			"[]",
			"Returns informations about the DLL. Returns a STRING."
		)
	);
	g_commands.push_back(
		sqf::Command(
			"VERSION",
			[](sqf::Array* arr)
			{
				return std::string("[TRUE,\"\",\"").append(_VERSION_).append("\"]");
			},
			"[]",
			"Returns current version of the DLL. Returns a STRING."
		)
	);
	g_commands.push_back(
		sqf::Command(
			"GETPREPAREDSTATEMENT",
			[](sqf::Array* arr)
			{
				auto uniqueID = g_config.getUniqueIdOfPreparedStatements(((sqf::String*) (*arr)[0])->getValue().c_str());
				if (uniqueID < 0)
				{
					throw std::exception(std::string("PreparedStatement '").append(((sqf::String*) (*arr)[0])->getValue()).append("' is not existing").c_str());
				}
				return std::string("[TRUE,\"\",").append(to_string(uniqueID)).append("]");
			},
			"[\"STRING\"]",
			"Searches for the PreparedStatement and returns its UniqueID. Returns a SCALAR."
		)
	);
	g_commands.push_back(
		sqf::Command(
			"GETCONNECTION",
			[](sqf::Array* arr)
			{
				auto uniqueID = g_config.getUniqueIdOfConnection(((sqf::String*) (*arr)[0])->getValue().c_str());
				if (uniqueID < 0)
				{
					throw std::exception(std::string("Connection '").append(((sqf::String*) (*arr)[0])->getValue()).append("' is not existing").c_str());
				}
				return std::string("[TRUE,\"\",").append(to_string(uniqueID)).append("]");
			},
			"[\"STRING\"]",
			"Searches for the Connection and returns its UniqueID. Returns a SCALAR."
		)
	);
	g_commands.push_back(
		sqf::Command(
			"OPENCONNECTION",
			[](sqf::Array* arr)
			{
				auto uniqueID = ((sqf::Scalar*) (*arr)[0])->getValue();
				if (uniqueID < 0 || uniqueID >= g_config.connections.size())
				{
					throw std::exception(std::string("Connection '").append(to_string(uniqueID)).append("' is not existing").c_str());
				}
				auto con = g_config.connections[uniqueID];
				con->openConnection(((sqf::Scalar*) (*arr)[1])->getValue());
				return std::string("[TRUE,\"\",NIL]");
			},
			"[1, 0]",
			"Opens given connection and starts the connection tracker (if Param2 is > 0). Returns a NIL value."
		)
	);
	g_commands.push_back(
		sqf::Command(
			"CLOSECONNECTION",
			[](sqf::Array* arr)
			{
				auto uniqueID = ((sqf::Scalar*) (*arr)[0])->getValue();
				if (uniqueID < 0 || uniqueID >= g_config.connections.size())
				{
					throw std::exception(std::string("Connection '").append(to_string(uniqueID)).append("' is not existing").c_str());
				}
				auto con = g_config.connections[uniqueID];
				con->closeConnection();
				return std::string("[TRUE,\"\",NIL]");
			},
			"[1]",
			"Searches for the Connection and returns its UniqueID. Returns a NIL value."
		)
	);
	g_commands.push_back(
		sqf::Command(
			"CREATEOPERATIONSET",
			[](sqf::Array* arr)
			{
				auto uniqueID = ((sqf::Scalar*) (*arr)[0])->getValue();
				if (uniqueID < 0 || uniqueID >= g_config.connections.size())
				{
					throw std::exception(std::string("Connection '").append(to_string(uniqueID)).append("' is not existing").c_str());
				}
				auto con = g_config.connections[uniqueID];
				con->createOperationSet(((sqf::String*) (*arr)[1])->getValue());
				return std::string("[TRUE,\"\",NIL]");
			},
			"[1, \"foobarKeyword\"]",
			"Creates a new OperationSet where SQL commands can be operated on. Returns a NIL value."
		)
	);
	g_commands.push_back(
		sqf::Command(
			"CLOSEOPERATIONSET",
			[](sqf::Array* arr)
			{
				auto uniqueID = ((sqf::Scalar*) (*arr)[0])->getValue();
				if (uniqueID < 0 || uniqueID >= g_config.connections.size())
				{
					throw std::exception(std::string("Connection '").append(to_string(uniqueID)).append("' is not existing").c_str());
				}
				auto con = g_config.connections[uniqueID];
				con->closeOperationSet(((sqf::String*) (*arr)[1])->getValue());
				return std::string("[TRUE,\"\",NIL]");
			},
			"[1, \"foobarKeyword\"]",
			"Closes an existing OperationSet. Returns a NIL value."
		)
	);
	if (g_config.allowNonPreparedSql) g_commands.push_back(
		sqf::Command(
			"EXECUTE",
			[](sqf::Array* arr)
			{
				auto uniqueID = ((sqf::Scalar*) (*arr)[0])->getValue();
				if (uniqueID < 0 || uniqueID >= g_config.connections.size())
				{
					throw std::exception(std::string("Connection '").append(to_string(uniqueID)).append("' is not existing").c_str());
				}
				auto con = g_config.connections[uniqueID];
				
				con->execute(((sqf::String*) (*arr)[1])->getValue(), "");
				return std::string("[TRUE,\"\",NIL]");
			},
			"[1, \"foobarKeyword\", \"SELECT * FROM table\"]",
			"Performs an operation on given OperationSet. Returns a BOOL containing if the operation was successfully executed."
		)
	);
	g_commands.push_back(
		sqf::Command(
			"EXECUTESTATEMENT",
			[](sqf::Array* arr)
			{
				auto uniqueIDConnection = ((sqf::Scalar*) (*arr)[0])->getValue();
				auto uniqueIDPreparedStatement = ((sqf::Scalar*) (*arr)[2])->getValue();
				if (uniqueIDConnection < 0 || uniqueIDConnection >= g_config.connections.size())
					throw std::exception(std::string("Connection '").append(to_string(uniqueIDConnection)).append("' is not existing").c_str());
				if (uniqueIDPreparedStatement < 0 || uniqueIDPreparedStatement >= g_config.connections.size())
					throw std::exception(std::string("PreparedStatement '").append(to_string(uniqueIDPreparedStatement)).append("' is not existing").c_str());

				auto con = g_config.connections[uniqueIDConnection];
				auto stmnt = g_config.preparedStatements[uniqueIDPreparedStatement];
				con->execute(((sqf::String*) (*arr)[1])->getValue(), stmnt->getStatementString(*((sqf::Array*) (*arr)[3])));
				return std::string("[TRUE,\"\",NIL]");
			},
			"[1, \"foobarKeyword\", 2, [<argumentName1>, <argument1>, <argumentName2>, <argument2>, <argumentNameN>, <argumentN>]]",
			"Performs an operation on given OperationSet with given Statement.Statement Arguments have to be provided like this: [<argumentName1>, <argument1>, <argumentName2>, <argument2>, <argumentNameN>, <argumentN>]. Please note that ONLY STRINGS are allowed for arguments.",
			false
		)
	);
}
std::string readConfig(void)
{
	g_config.clear();
	dotX39::Node* root = new dotX39::Node("root");
	try 
	{
		dotX39::DocumentReader::readDocument("easyDatabase.x39", root);
	}
	catch (std::exception e)
	{
		return std::string("Error while reading config file: ").append(e.what());
	}
	for (int rootNodeIndex = 0; rootNodeIndex < root->getNodeCount(); rootNodeIndex++)
	{
		const dotX39::Node* layer1 = root->getNode(rootNodeIndex);
		const std::string& layer1Name = layer1->getName();
		if (layer1Name.compare("settings") == 0)
		{
			for (int layer2DataIndex = 0; layer2DataIndex < layer1->getDataCount(); layer2DataIndex++)
			{
				const dotX39::Data* data = layer1->getData(layer2DataIndex);
				const std::string& dataName = data->getName();
				auto dataType = data->getType();
				if (dataName.compare("doToUpperForCommands") == 0)
				{
					if (dataType != dotX39::DataTypes::BOOLEAN)
					{
						delete root;
						return std::string("Error while reading config file: Settings '").append(dataName).append("' has invalid datatype, expected BOOLEAN");
					}
					g_config.doToUpperForCommands = ((dotX39::DataBoolean*)data)->getDataAsBoolean();
				}
				if (dataName.compare("doCheckParams") == 0)
				{
					if (dataType != dotX39::DataTypes::BOOLEAN)
					{
						delete root;
						return std::string("Error while reading config file: Settings '").append(dataName).append("' has invalid datatype, expected BOOLEAN");
					}
					g_config.doCheckParams = ((dotX39::DataBoolean*)data)->getDataAsBoolean();
				}
				if (dataName.compare("allowNonPreparedSql") == 0)
				{
					if (dataType != dotX39::DataTypes::BOOLEAN)
					{
						delete root;
						return std::string("Error while reading config file: Settings '").append(dataName).append("' has invalid datatype, expected BOOLEAN");
					}
					g_config.allowNonPreparedSql = ((dotX39::DataBoolean*)data)->getDataAsBoolean();
				}
			}
		}
		else if (layer1Name.compare("connections") == 0)
		{
			for (int layer1NodeIndex = 0; layer1NodeIndex < layer1->getNodeCount(); layer1NodeIndex++)
			{
				const dotX39::Node* layer2 = layer1->getNode(layer1NodeIndex);
				std::string name = layer2->getName();
				std::string serverUri;
				std::string database;
				std::string username;
				std::string password;
				
				for (int layer2ArgumentIndex = 0; layer2ArgumentIndex < layer2->getArgumentCount(); layer2ArgumentIndex++)
				{
					const dotX39::Data* arg = layer2->getArgument(layer2ArgumentIndex);
					std::string& argName = arg->getName();
					dotX39::DataTypes argType = arg->getType();
					if (argName.compare("serverUri") == 0)
					{
						if (argType != dotX39::DataTypes::STRING)
						{
							delete root;
							return std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' has invalid datatype for ").append(argName).append("', expected STRING");
						}
						serverUri = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else if (argName.compare("database") == 0)
					{
						if (argType != dotX39::DataTypes::STRING)
						{
							delete root;
							return std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' has invalid datatype for ").append(argName).append("', expected STRING");
						}
						database = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else if (argName.compare("username") == 0)
					{
						if (argType != dotX39::DataTypes::STRING)
						{
							delete root;
							return std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' has invalid datatype for ").append(argName).append("', expected STRING");
						}
						username = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else if (argName.compare("password") == 0)
					{
						if (argType != dotX39::DataTypes::STRING)
						{
							delete root;
							return std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' has invalid datatype for ").append(argName).append("', expected STRING");
						}
						password = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else
					{
						delete root;
						return std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' contains unknown argument: ").append(argName);
					}
				}
				if (serverUri.empty() || database.empty() || username.empty())
				{
					delete root;
					return std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' is missing arguments");
				}
				g_config.connections.push_back(new Connection(name, serverUri, database, username, password));
			}
		}
		else if (layer1Name.compare("preparedStatements") == 0)
		{

			for (int layer1NodeIndex = 0; layer1NodeIndex < layer1->getNodeCount(); layer1NodeIndex++)
			{
				const dotX39::Node* layer2 = layer1->getNode(layer1NodeIndex);
				std::string name = layer2->getName();
				std::string stmnt;
				for (int layer2ArgumentIndex = 0; layer2ArgumentIndex < layer2->getArgumentCount(); layer2ArgumentIndex++)
				{
					const dotX39::Data* arg = layer2->getArgument(layer2ArgumentIndex);
					std::string& argName = arg->getName();
					dotX39::DataTypes argType = arg->getType();
					if (argName.compare("statement") == 0)
					{
						if (argType != dotX39::DataTypes::STRING)
						{
							delete root;
							return std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' has invalid datatype for '").append(argName).append("', expected STRING");
						}
						stmnt = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else
					{
						delete root;
						return std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' contains unknown argument: ").append(argName);
					}
				}
				if (stmnt.empty())
				{
					delete root;
					return std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' is missing arguments");
				}
				PreparedStatement* statement = new PreparedStatement(name, stmnt);
				for (int layer2NodeIndex = 0; layer2NodeIndex < layer2->getNodeCount(); layer2NodeIndex++)
				{
					const dotX39::Node* layer3 = layer2->getNode(layer2NodeIndex);
					std::string token;
					bool isEscaped = true;
					for (int layer3ArgumentIndex = 0; layer3ArgumentIndex < layer3->getArgumentCount(); layer3ArgumentIndex++)
					{
						const dotX39::Data* arg = layer3->getArgument(layer3ArgumentIndex);
						std::string& argName = arg->getName();
						dotX39::DataTypes argType = arg->getType();
						if (argName.compare("token") == 0)
						{
							if (argType != dotX39::DataTypes::STRING)
							{
								delete root;
								delete statement;
								return std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' has invalid datatype for '").append(argName).append("', expected STRING");
							}
							token = ((dotX39::DataString*)arg)->getDataAsString();
						}
						if (argName.compare("escape") == 0)
						{
							if (argType != dotX39::DataTypes::BOOLEAN)
							{
								delete root;
								delete statement;
								return std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' has invalid datatype for '").append(argName).append("', expected BOOLEAN");
							}
							isEscaped = ((dotX39::DataBoolean*)arg)->getDataAsBoolean();
						}
						else
						{
							delete root;
							delete statement;
							return std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' contains unknown argument: ").append(argName);
						}
					}
					if (token.empty())
					{
						delete root;
						delete statement;
						return std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' is missing argument: 'token");
					}
					statement->addArgument(PreparedStatement::ARGUMENT(layer3->getName(), token, false));
				}
				g_config.preparedStatements.push_back(statement);
			}
		}
	}
	delete root;
	return "";
}

//Return array always: [<Success:BOOL>, <Error:STRING>, <FunctionResult:UNKNOWN>]
void __stdcall RVExtension(char *output, int outputSize, const char *function)
{
	//Check if config was read yet and load it
	if (g_commands.empty())
	{
		std::string& result = readConfig();
		if (!result.empty())
		{
			strncpy(output, std::string("[FALSE,\"").append(result).append("\",NIL]").c_str(), outputSize);
			return;
		}
	}
	//Do basic function parsing
	sqf::Array arr;
	try
	{
		sqf::Array::parsePartially(&arr, function);
	}
	catch (std::exception e)
	{
		strncpy(output, std::string("[FALSE,\"").append("Error while reading input: ").append(sqf::String::escapeString(e.what())).append("\",NIL]").c_str(), outputSize);
		return;
	}
	size_t paramCount = arr.length();
	if (paramCount < 2)
	{
		if (paramCount == 0)
			strncpy(output, std::string("[FALSE,\"").append("Error while reading input: No Function provided.").append("\",NIL]").c_str(), outputSize);
		else
			strncpy(output, std::string("[FALSE,\"").append("Error while reading input: Missing default argument.").append("\",NIL]").c_str(), outputSize);
		return;
	}
	sqf::Base& tmpFnc = *arr[0];
	if (tmpFnc.getType() != sqf::Type::STRING)
	{
		strncpy(output, std::string("[FALSE,\"").append("First function parameter was NOT of the type string!").append("\",NIL]").c_str(), outputSize);
		return;
	}

	sqf::Base& tmpArg = *arr[1];
	if (tmpArg.getType() != sqf::Type::STRING)
	{
		strncpy(output, std::string("[FALSE,\"").append("Second function parameter was NOT of the type array!").append("\",NIL]").c_str(), outputSize);
		return;
	}
	//Instruction Handling
	sqf::Array& arg = *(sqf::Array*)arr[1];
	std::string fnc = ((sqf::String&)tmpFnc).getValue();
	if (g_config.doToUpperForCommands)
		toUpper(fnc);
	bool flag = false;
	std::string cmdOut;
	try
	{
		for (auto& it : g_commands)
			if (flag = it.runIfMatch(fnc, cmdOut, &arg, g_config.doCheckParams))
			{
				strncpy(output, cmdOut.c_str(), outputSize);
				break;
			}
	}
	catch (std::exception e)
	{
		strncpy(output, std::string("[FALSE,\"").append("Function '").append(fnc).append("' raised an exception. Hint: ").append(e.what()).append("\",NIL]").c_str(), outputSize);
		return;
	}
	if (!flag)
	{
		strncpy(output, std::string("[FALSE,\"").append("Function '").append(fnc).append("' is unknown. Hint: Function names are case-sensitive!\",NIL]").c_str(), outputSize);
		return;
	}
}

#ifndef _DEBUG
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		addCommands();
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		g_commands.clear();
		break;
	}
	return TRUE;
}
#endif