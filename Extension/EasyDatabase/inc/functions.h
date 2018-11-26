#pragma once
#include "omemstream.h"
#include <vector>
#include <string_view>

enum returntype
{
	ret_ok = 0,
	ret_err = -1,
	ret_not_implemented = -2,
	ret_arg_err = -3,
	ret_invalid_operation = -3
};

typedef returntype(*rvextension_callback)(omemstream& out, std::vector<std::string_view>& args);


// Searches for the PreparedStatement and returns its UniqueID.
//
// @Param1		STRING		Name of PreparedStatement (Case Sensitive)
// @Return		SCALAR		UniqueID of given PreparedStatement
// @CanFail		Function will fail when given PreparedStatement is not existing
// @RetCodes	ret_ok, ret_arg_err
// @Example		"easydatabase" callExtension ["find_prepared_statement", "preparedstatement"]
// @ExampleRet	[false, -1]
// @ExampleRet	[true, 0]
returntype find_prepared_statement(omemstream& out, std::vector<std::string_view>& args);

// Searches for the Connection and returns its UniqueID.
//
// @Param1		STRING		Name of Connection (Case Sensitive)
// @Return		SCALAR		UniqueID of given Connection
// @CanFail		Function will fail when given Connection is not existing
// @RetCodes	ret_ok, ret_arg_err
// @Example		"easydatabase" callExtension ["find_connection", "connection"]
// @ExampleRet	[false, -1]
// @ExampleRet	[true, 0]
returntype find_connection(omemstream& out, std::vector<std::string_view>& args);

// Creates a new database connection using provided
// UniqueID of a predefined connection.
// Is expected to be fed the ID returned by "find_connection".
//
// @Param1		SCALAR		UniqueID of the Connection.
// @Return		SCALAR		Identifying number of this database connection.
//							Note that the number gets reassigned when
//							the connection is closed.
// @CanFail		Function will fail when given Connection is not existing or
//				anything else moves wrong.
// @RetCodes	ret_ok, ret_arg_err
// @Example		"easydatabase" callExtension ["open_connection", 1]
// @ExampleRet	[false, -1]
// @ExampleRet	[true, 0]
returntype open_connection(omemstream& out, std::vector<std::string_view>& args);

// Closes an existing, open connection.
// Is expected to be fed the identifier returned by "open_connection".
//
// @Param1		SCALAR		Connection identifier.
// @Return		NIL
// @CanFail		Function will fail when given Connection is not open anymore.
// @RetCodes	ret_ok, ret_arg_err, ret_invalid_operation
// @Example		"easydatabase" callExtension ["close_connection", 1]
// @ExampleRet	[false, nil]
// @ExampleRet	[true, nil]
returntype close_connection(omemstream& out, std::vector<std::string_view>& args);
returntype create_operation_set(omemstream& out, std::vector<std::string_view>& args);
returntype close_operation_set(omemstream& out, std::vector<std::string_view>& args);
returntype execute(omemstream& out, std::vector<std::string_view>& args);
returntype execute_statement(omemstream& out, std::vector<std::string_view>& args);
returntype execute_query(omemstream& out, std::vector<std::string_view>& args);
returntype execute_query_statement(omemstream& out, std::vector<std::string_view>& args);
returntype execute_update(omemstream& out, std::vector<std::string_view>& args);
returntype execute_update_statement(omemstream& out, std::vector<std::string_view>& args);
returntype next(omemstream& out, std::vector<std::string_view>& args);
returntype clear(omemstream& out, std::vector<std::string_view>& args);