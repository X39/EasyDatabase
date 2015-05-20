#include "dllMain.h"
#include "dotX39\DocumentReader.h"
#include "dotX39\DataString.h"
#include "dotX39\DataBoolean.h"


#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// Standad C++ includes
#include <iostream>
#include <cstdlib>
#include <thread>
#include <stdio.h>
#include <ctime>
#include <sstream>
#include <mutex>
#include <vector>
#include <array>
using namespace std;

// Include the Connector/C++ headers
#include <cppconn\driver.h>
#include <cppconn\connection.h>
#include <cppconn\resultset.h>
#include <cppconn\statement.h>
#ifdef _DEBUG
#define CONNECTION_CHECK_TIMEOUT 100
#else
#define CONNECTION_CHECK_TIMEOUT 30000
#endif

typedef struct structSqlContainer
{
	sql::Statement*			statement;
	sql::ResultSet*			resultSet;
	std::stringstream		buffer;
	structSqlContainer() : statement(NULL), resultSet(NULL), buffer(std::stringstream("")) {}
	~structSqlContainer()
	{
		if (resultSet != NULL)
		{
			if (!resultSet->isClosed())
				resultSet->close();
			delete resultSet;
		}
		if (statement != NULL)
		{
			statement->close();
			delete statement;
		}
	}
}SQLCONTAINER;
typedef struct structArgument
{
	std::string name;
	std::string token;
	bool isEscaped;
	structArgument(std::string Name, std::string Token, bool IsEscaped = false) : name(Name), token(Token), isEscaped(IsEscaped) {};
} ARGUMENT;
typedef struct structConnection
{
	std::string name;
	std::string username;
	std::string password;
	std::string serverUri;
	std::string database;

	sql::Connection* databaseConnection;
	std::vector<SQLCONTAINER*> sqlOperations;
	time_t lastAccess;
	std::mutex mutex;

	structConnection() : databaseConnection(NULL) {};
	~structConnection()
	{
		for (auto& sqlOperation : sqlOperations)
			if (sqlOperation != NULL)
				delete sqlOperation;
		if (databaseConnection != NULL)
		{
			if (!databaseConnection->isClosed())
				databaseConnection->close();
			delete databaseConnection;
		}
	}
	bool isValid(std::string* outString)
	{
		bool flag = false;
		outString->append("[");
		if (name.empty())
		{
			if (flag)
				outString->append(",");
			outString->append("name");
			flag = true;
		}
		if (username.empty())
		{
			if (flag)
				outString->append(",");
			outString->append("username");
			flag = true;
		}
		if (password.empty())
		{
			if (flag)
				outString->append(",");
			outString->append("password");
			flag = true;
		}
		if (serverUri.empty())
		{
			if (flag)
				outString->append(",");
			outString->append("serverUri");
			flag = true;
		}
		if (database.empty())
		{
			if (flag)
				outString->append(",");
			outString->append("database");
			flag = true;
		}
		outString->append("]");
		return !flag;
	}
} CONNECTION;
typedef struct structPreparedStatement
{
	std::string name;
	std::string statement;
	std::vector<ARGUMENT> arguments;
	bool isValid(std::string* outString)
	{
		bool flag = false;
		outString->append("[");
		if (name.empty())
		{
			if (flag)
				outString->append(",");
			outString->append("name");
			flag = true;
		}
		if (statement.empty())
		{
			if (flag)
				outString->append(",");
			outString->append("statement");
			flag = true;
		}
		outString->append("]");
		return !flag;
	}
	ARGUMENT* getArgumentByName(const char* name)
	{
		for (int i = 0; i < arguments.size(); i++)
		{
			if (arguments[i].name.compare(name) == 0)
				return &arguments[i];
		}
		return nullptr;
	}
	//@param arguments: array structure should be [<argumentName1>, <argument1>, <argumentName2>, <argument2>, <argumentNameN>, <argumentN>]
	bool replaceArguments(const char** argumentArray, unsigned int argumentCount, std::string& outString)
	{
		//Simple Structure that contains informations about the Argument to replace and the text to replace the arguments token with
		struct ARG
		{
			ARGUMENT& argumentReference;
			const char* replacement;
		};
		if (argumentCount % 2 != 0) { setLastError(std::string(to_string(argumentCount)).append(" is no valid argument count (count % 2 == 0)")); return false; }
		if (argumentCount / 2 != this->arguments.size()) { setLastError(std::string("PreparedStatements '").append(this->name).append("' ArgumentCount is ").append(to_string(this->arguments.size())).append(" but got ").append(to_string(argumentCount / 2)).append(" arguments for replacement")); return false; }
		std::vector<struct ARG> args;
		for (int i = 0; i < argumentCount; i += 2)
		{
			ARGUMENT* tmpArgument = getArgumentByName(argumentArray[i]);
			if (tmpArgument == nullptr)
			{
				setLastError(std::string("Cannot find argument '").append(argumentArray[i]).append("' for PreparedStatement '").append(this->name).append("'"));
				return false;
			}
			struct ARG arg = { arg.argumentReference = *tmpArgument, arg.replacement = argumentArray[i + 1] };
			args.push_back(arg);
		}
		int lastFindResult = 0;
		for (auto& arg : args)
		{
			int findResult = this->statement.find(arg.argumentReference.token, lastFindResult);
			if (findResult == -1)
			{
				setLastError(std::string("Cannot find argument token '").append(arg.argumentReference.token).append("' inside of PreparedStatement '").append(this->name).append("'"));
				return false;
			}
			outString.append(this->statement.substr(lastFindResult, findResult));
			outString.append(arg.replacement);
			lastFindResult = findResult;
		}
		outString.append(this->statement.substr(lastFindResult));
	}
} PREPAREDSTATEMENT;

static struct config {
	std::vector<CONNECTION> connections;
	std::vector<PREPAREDSTATEMENT> preparedStatements;
	int getUniqueIdOfConnection(const char* name)
	{
		int uniqueId = -1;
		for (int i = 0; i < connections.size(); i++)
		{
			if (connections[i].name.compare(name) == 0)
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
			if (preparedStatements[i].name.compare(name) == 0)
			{
				uniqueId = i;
				break;
			}
		}
		return uniqueId;
	}
} g_config;
static bool								g_initialized = false;
static std::string						g_lastError;
static sql::Driver*						g_dbDriver = NULL;

//Function for RVExtension output
void setBool(char* out, unsigned int maxOut, bool flag)
{
	if (flag)
	{
		char tmp[] = "TRUE";
		if (strlen(tmp) >= maxOut)
			return;
		strcpy(out, tmp);
	}
	else
	{
		char tmp[] = "FALSE";
		if (strlen(tmp) >= maxOut)
			return;
		strcpy(out, tmp);
	}
}
//Function for RVExtension output
void setArray_Bool_Index(char* out, unsigned int maxOut, bool flag, int index)
{
	std::string tmp = std::string("[").append(flag ? "TRUE" : "FALSE").append(",").append(to_string(index)).append("]");
	if (tmp.size() >= maxOut)
		return;
	strcpy(out, tmp.c_str());
}

//Changes LastError variable
inline void setLastError(void)
{
	if (g_lastError.empty())
		return;
	setLastError("");
}
//Changes LastError variable
inline void setLastError(const std::string& str)
{
	setLastError(str.c_str());
}
//Changes LastError variable
inline void setLastError(const char* str)
{
	g_lastError = std::string(str);
}


void clearConfig(void)
{
	g_config.connections.clear();
	g_config.preparedStatements.clear();
}
bool readConfig(void)
{
	clearConfig();
	dotX39::Node* root = new dotX39::Node("root");
	try 
	{
		dotX39::DocumentReader::readDocument("easyDatabase.x39", root);
	}
	catch (std::exception e)
	{
		setLastError(std::string("Error while reading config file: ").append(e.what()));
		delete root;
		return false;
	}
	for (int rootNodeIndex = 0; rootNodeIndex < root->getNodeCount(); rootNodeIndex++)
	{
		const dotX39::Node* layer1 = root->getNode(rootNodeIndex);
		const std::string& layer1Name = layer1->getName();
		if (layer1Name.compare("connections") == 0)
		{
			for (int layer1NodeIndex = 0; layer1NodeIndex < layer1->getNodeCount(); layer1NodeIndex++)
			{
				const dotX39::Node* layer2 = layer1->getNode(layer1NodeIndex);
				CONNECTION con;
				con.name = layer2->getName();
				for (int layer2ArgumentIndex = 0; layer2ArgumentIndex < layer2->getArgumentCount(); layer2ArgumentIndex++)
				{
					const dotX39::Data* arg = layer2->getArgument(layer2ArgumentIndex);
					std::string& argName = arg->getName();
					dotX39::DataTypes argType = arg->getType();
					if (argName.compare("serverUri") == 0)
					{
						if (argType == dotX39::DataTypes::STRING)
						{
							setLastError(std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' has invalid datatype for ").append(argName).append("', expected STRING"));
							delete root;
							return false;
						}
						con.serverUri = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else if (argName.compare("database") == 0)
					{
						if (argType == dotX39::DataTypes::STRING)
						{
							setLastError(std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' has invalid datatype for ").append(argName).append("', expected STRING"));
							delete root;
							return false;
						}
						con.database = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else if (argName.compare("username") == 0)
					{
						if (argType == dotX39::DataTypes::STRING)
						{
							setLastError(std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' has invalid datatype for ").append(argName).append("', expected STRING"));
							delete root;
							return false;
						}
						con.username = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else if (argName.compare("password") == 0)
					{
						if (argType == dotX39::DataTypes::STRING)
						{
							setLastError(std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' has invalid datatype for ").append(argName).append("', expected STRING"));
							delete root;
							return false;
						}
						con.password = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else
					{
						setLastError(std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' contains unknown argument: ").append(argName));
						delete root;
						return false;
					}
				}
				std::string isValidString;
				if (!con.isValid(&isValidString))
				{
					setLastError(std::string("Error while reading config file: Connection '").append(layer2->getName()).append("' is missing arguments: ").append(isValidString));
					delete root;
					return false;
				}
				g_config.connections.push_back(con);
			}
		}
		else if (layer1Name.compare("PreparedStatements") == 0)
		{

			for (int layer1NodeIndex = 0; layer1NodeIndex < layer1->getNodeCount(); layer1NodeIndex++)
			{
				const dotX39::Node* layer2 = layer1->getNode(layer1NodeIndex);
				PREPAREDSTATEMENT statement;
				statement.name = layer2->getName();
				for (int layer2ArgumentIndex = 0; layer2ArgumentIndex < layer2->getArgumentCount(); layer2ArgumentIndex++)
				{
					const dotX39::Data* arg = layer2->getArgument(layer2ArgumentIndex);
					std::string& argName = arg->getName();
					dotX39::DataTypes argType = arg->getType();
					if (argName.compare("statement") == 0)
					{
						if (argType == dotX39::DataTypes::STRING)
						{
							setLastError(std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' has invalid datatype for '").append(argName).append("', expected STRING"));
							delete root;
							return false;
						}
						statement.statement = ((dotX39::DataString*)arg)->getDataAsString();
					}
					else
					{
						setLastError(std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' contains unknown argument: ").append(argName));
						delete root;
						return false;
					}
				}
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
							if (argType == dotX39::DataTypes::STRING)
							{
								setLastError(std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' has invalid datatype for '").append(argName).append("', expected STRING"));
								delete root;
								return false;
							}
							token = ((dotX39::DataString*)arg)->getDataAsString();
						}
						if (argName.compare("escape") == 0)
						{
							if (argType == dotX39::DataTypes::BOOLEAN)
							{
								setLastError(std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' has invalid datatype for '").append(argName).append("', expected BOOLEAN"));
								delete root;
								return false;
							}
							isEscaped = ((dotX39::DataBoolean*)arg)->getDataAsBoolean();
						}
						else
						{
							setLastError(std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' contains unknown argument: ").append(argName));
							delete root;
							return false;
						}
					}
					if (token.empty())
					{
						setLastError(std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' is missing argument: 'token"));
						delete root;
						return false;
					}
					statement.arguments.push_back(ARGUMENT(layer3->getName(), token, false));
				}
				std::string isValidString;
				if (!statement.isValid(&isValidString))
				{
					setLastError(std::string("Error while reading config file: PreparedStatement '").append(layer2->getName()).append("' is missing arguments: ").append(isValidString));
					delete root;
					return false;
				}
				g_config.preparedStatements.push_back(statement);
			}
		}
	}
	delete root;
	return true;
}
inline void updateLastConnectionAccess(CONNECTION& con)
{
	con.lastAccess = time(NULL);
}
void thread_connectionWatch(unsigned long timeout, unsigned int conIndex)
{
	while (g_config.connections.size() > conIndex && time(NULL) - g_config.connections[conIndex].lastAccess < timeout)
		Sleep(CONNECTION_CHECK_TIMEOUT);
	g_config.connections[conIndex].mutex.lock();
	closeConnection();
	g_config.connections[conIndex].mutex.unlock();
}
void __stdcall RVExtension(char *output, int outputSize, const char *function)
{
	g_connectionWatchMutex.lock();
	//Split function parameter into propper function parameters and
	//do error checking for function parameter
	bool argPresent = true;
	std::string fnc;
	std::string arg;
	int functionLen = strlen(function);
	int tmpFunctionLen;
	output[0] = 0;
	if (functionLen < 3)
		goto functionEnd;
	const char* tmpFunction = strchr(function, ',');
	if (tmpFunction == NULL)
	{
		argPresent = false;
		tmpFunction = strrchr(function, ']');
		if (tmpFunction == NULL)
			goto functionEnd;
	}
	tmpFunctionLen = strlen(tmpFunction);
	if (functionLen - 2 <= 0 || functionLen - tmpFunctionLen - 2 <= 0)
		goto functionEnd;
	fnc = std::string(function + 2, tmpFunction - 1);

	if (argPresent)
	{
		if (tmpFunctionLen < 3)
			goto functionEnd;
		const char* tmpArg = strrchr(tmpFunction, ']');
		
		if (tmpArg == NULL)
			goto functionEnd;
		//No error check needed because of handling from -11 lines
		arg = std::string(tmpFunction + 1, tmpArg);
	}
	//move into the different function
	if (false){} //For simple switching of the commands, compiler should remove it during compile time
#pragma region Function NEXT
	else if (fnc.compare("NEXT") == NULL)
	{
		unsigned int index;
		try	{ index = std::stoul(arg, nullptr, 10); }
		catch (std::exception e) { setLastError(e.what()); setBool(output, outputSize, false); goto functionEnd; }
		if (index >= g_sqlVector.size()) { setLastError("Given index is out of range"); setBool(output, outputSize, false); goto functionEnd; }
		SQLCONTAINER* container = g_sqlVector[index];
		if (container->statement == NULL)
		{
			setLastError("Please open a statement before trying to operate on it.");
			goto functionEnd;
		}
		if (container->resultSet == NULL)
		{
			setLastError("Please query a statement before trying to get results from it.");
			goto functionEnd;
		}
		if (container->buffer.peek() == EOF)
		{//Load a new row into the stringstream
			if (container->resultSet->next())
			{
				container->buffer.clear();
				container->buffer << '[';
				int cellIndex = 1;
				try
				{
					auto columnCount = container->resultSet->getMetaData()->getColumnCount();
					for (unsigned int i = 1; i <= columnCount; i++)
					{
						std::string res = container->resultSet->getString(cellIndex);
						if (cellIndex > 1)
							container->buffer << ",\"" << res << '"';
						else
							container->buffer << '"' << res << '"';
						cellIndex++;
					}
				}
				catch (std::exception e) { }
				container->buffer << ']';
			}
			else
			{
				container->resultSet->close();
				delete container->resultSet;
				container->resultSet = NULL;
				strcpy(output, "");
				goto functionEnd;
			}
		}
		container->buffer.readsome(output, outputSize - 1);
		output[container->buffer.gcount()] = '\0';
	}
#pragma endregion
#pragma region Function QUERY
	else if (fnc.compare("QUERY") == NULL)
	{
		unsigned int index;
		try	{ index = std::stoul(arg, nullptr, 10); }
		catch (std::exception e) { setLastError(e.what()); setBool(output, outputSize, false); goto functionEnd; }
		if (index >= g_sqlVector.size()) { setLastError("Given index is out of range"); setBool(output, outputSize, false); goto functionEnd; }
		SQLCONTAINER* container = g_sqlVector[index];
		const char* tmpStr = strchr(arg.c_str(), ',');
		if(tmpStr == NULL)
		{
			setLastError("Could not find SQL Statement at third place.");
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		arg = tmpStr + 1;
		arg.erase(arg.begin());
		arg.erase(arg.end() - 1);
		if (container->statement == NULL)
		{
			setLastError("Please open a statement before trying to operate on it.");
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		if (container->resultSet != NULL)
		{
			setLastError("The last query is not done yet. Please use either CLEAR or NEXT to continue.");
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		try
		{
			container->resultSet = container->statement->executeQuery(arg);
			setBool(output, outputSize, true);
		}
		catch (sql::SQLException e)
		{
			setLastError(std::string("SQL error. Error message: ").append(e.what()));
		}
	}
#pragma endregion
#pragma region Function UPDATE
	else if (fnc.compare("UPDATE") == NULL)
	{
		unsigned int index;
		try	{ index = std::stoul(arg, nullptr, 10); }
		catch (std::exception e) { setLastError(e.what()); setBool(output, outputSize, false); goto functionEnd; }
		if (index >= g_sqlVector.size()) { setLastError("Given index is out of range"); setBool(output, outputSize, false); goto functionEnd; }
		SQLCONTAINER* container = g_sqlVector[index];
		if (container->statement == NULL)
		{
			setLastError("Please open a statement before trying to operate on it.");
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		if (container->resultSet == NULL)
		{
			setLastError("The last query is not done yet. Please use either CLEAR or NEXT to continue.");
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		try
		{
			container->statement->execute(arg);
			setBool(output, outputSize, true);
		}
		catch (sql::SQLException e)
		{
			setLastError(std::string("SQL error. Error message: ").append(e.what()));
		}
	}
#pragma endregion
#pragma region Function CLOSESTMT
	else if (fnc.compare("CLOSESTMT") == NULL)
	{
		unsigned int index;
		try	{ index = std::stoul(arg, nullptr, 10); }
		catch (std::exception e) { setLastError(e.what()); setBool(output, outputSize, false); goto functionEnd; }
		if (index >= g_sqlVector.size()) { setLastError("Given index is out of range"); setBool(output, outputSize, false); goto functionEnd; }
		SQLCONTAINER* container = g_sqlVector[index];
		if (container->statement == NULL)
		{
			setLastError("Please open a statement before trying to close it.");
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		if (container->resultSet != NULL)
		{
			container->resultSet->close();
			delete container->resultSet;
			container->resultSet = NULL;
		}
		delete container->statement;
		container->statement = NULL;
		setBool(output, outputSize, true);
	}
#pragma endregion
#pragma region Function CREATESTMT
	else if (fnc.compare("CREATESTMT") == NULL)
	{
		unsigned int index = 0;
		SQLCONTAINER* container = NULL;
		for (int i = 0; i < g_sqlVector.size(); i++)
		{
			if (g_sqlVector[i]->statement == NULL)
			{
				index = i;
				container = g_sqlVector[i];
				break;
			}
		}
		if (container == NULL)
		{
			index = g_sqlVector.size();
			container = new SQLCONTAINER();
			g_sqlVector.push_back(container);
		}
		if (g_dbConn == NULL)
		{
			setLastError("Please OPEN a connection before trying to create a statement.");
			setArray_Bool_Index(output, outputSize, false, -1);
			goto functionEnd;
		}
		if (container->statement != NULL)
		{
			setLastError("Please close a statement before trying to create a new one");
			setArray_Bool_Index(output, outputSize, false, -1);
			goto functionEnd;
		}
		if (container->resultSet != NULL)
		{
			setLastError("Please close a statement before trying to create a new one");
			setArray_Bool_Index(output, outputSize, false, -1);
			goto functionEnd;
		}
		try
		{
			container->statement = g_dbConn->createStatement();
			container->statement->execute(std::string("use ").append(g_mysqlDatabase));
			setArray_Bool_Index(output, outputSize, true, index);
		}
		catch (sql::SQLException e)
		{
			setLastError(std::string(e.what()));
			setArray_Bool_Index(output, outputSize, false, -1);
			goto functionEnd;
		}
	}
#pragma endregion
#pragma region Function CLEAR
	else if (fnc.compare("CLEAR") == NULL)
	{
		unsigned int index;
		try	{ index = std::stoul(arg, nullptr, 10); }
		catch (std::exception e) { setLastError(e.what()); setBool(output, outputSize, false); goto functionEnd; }
		if (index >= g_sqlVector.size()) { setLastError("Given index is out of range"); setBool(output, outputSize, false); goto functionEnd; }
		SQLCONTAINER* container = g_sqlVector[index];

		container->buffer.clear();
		if (container->resultSet != NULL)
		{
			container->resultSet->close();
			delete container->resultSet;
			container->resultSet = NULL;
		}
		setBool(output, outputSize, true);
	}
#pragma endregion
#pragma region Function GETLASTERROR
	else if (fnc.compare("GETLASTERROR") == NULL)
	{
		if (outputSize <= g_lastError.size())
			goto functionEnd;
		strcpy(output, g_lastError.c_str());
	}
#pragma endregion
#pragma region Function OPEN
	else if (fnc.compare("OPEN") == NULL)
	{
		if (!g_initialized) { setLastError("You need to call INIT before you can open a connection."); setBool(output, outputSize, false); goto functionEnd; }
		if (g_dbConn != NULL) { setBool(output, outputSize, true); goto functionEnd; }
		try
		{
			
			g_dbConn = g_dbDriver->connect(g_mysqlServer, g_mysqlUsername, g_mysqlPassword);
			if (!arg.empty())
			{//Run async auto close thread
				unsigned int timeout = std::stoul(arg, nullptr, 10);
				if (timeout != 0)
				{
					updateLastConnectionAccess();
					std::thread t = std::thread(connectionWatch, timeout);
					t.detach();
				}
			}
		}
		catch (sql::SQLException e)
		{
			setLastError(std::string("Could not connect to database. Error message: ").append(e.what()));
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		catch (std::invalid_argument e)
		{
			setLastError(std::string("Cannot interpret argument as UNSIGNED INT. Error message: ").append(e.what()));
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		catch (std::out_of_range e)
		{
			setLastError(std::string("Provided value is out of range. Error message: ").append(e.what()));
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		catch (std::exception e)
		{
			setLastError(std::string("Unknown Error. Error message: ").append(e.what()));
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		setBool(output, outputSize, true);
	}
#pragma endregion
#pragma region Function KEEPOPEN
	else if (fnc.compare("KEEPOPEN") == NULL)
	{
		if (g_dbConn == NULL)
		{
			setLastError();
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		updateLastConnectionAccess();
		setBool(output, outputSize, true);
	}
#pragma endregion
#pragma region Function CLOSE
	else if (fnc.compare("CLOSE") == NULL)
	{
		if (g_dbConn == NULL)
		{
			setLastError("Please OPEN a connection before trying to close");
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		if (!closeConnection())
		{
			setBool(output, outputSize, false);
			goto functionEnd;
		}
		setBool(output, outputSize, true);
	}
#pragma endregion
#pragma region Function INIT
	else if (fnc.compare("INIT") == NULL)
	{
		if (g_initialized) { setBool(output, outputSize, g_initialized); goto functionEnd; }
		if (!readConfig()) { setBool(output, outputSize, g_initialized); goto functionEnd; }
		if (g_config.connections.empty()) { setLastError("Config file contains no connection"); setBool(output, outputSize, g_initialized); goto functionEnd; }
		try
		{
			g_dbDriver = get_driver_instance();;
		}
		catch (sql::SQLException e)
		{
			setLastError(std::string("Could not get a database driver. Error message: ").append(e.what()));
			setBool(output, outputSize, g_initialized);
			goto functionEnd;
		}
		g_initialized = true;
		setBool(output, outputSize, g_initialized);
	}
#pragma endregion
#pragma region Function INITIALIZED
	else if (fnc.compare("INITIALIZED") == NULL)
	{
		setBool(output, outputSize, g_initialized);
	}
#pragma endregion
#pragma region Function VERSION
	else if (fnc.compare("VERSION") == NULL)
	{
		char version[] = "0.1.0 BETA";
		if (strlen(version) >= outputSize)
			goto functionEnd;
		strcpy(output, version);
	}
#pragma endregion
functionEnd:
	g_connectionWatchMutex.unlock();
	return;
}

#ifndef _DEBUG
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		g_connectionWatchMutex.lock();
		closeConnection();
		g_connectionWatchMutex.unlock();
		break;
	}
	return TRUE;
}
#endif