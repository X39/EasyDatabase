#pragma once
#include <map>
#include "functions.h"
#include "connection.h"
#include "prepared_statement.h"

#if defined(WIN32)
#define DLLEXPORT __declspec (dllexport)
#define CALLSPEC_STD __stdcall
#elif defined(__GNUC__)
#define DLLEXPORT __attribute__((dllexport))
#define CALLSPEC_STD 
#else
#error Missing Compiler Exports
#endif

#if defined(__cplusplus)
extern "C" {
#endif
	//--- Called by Engine on extension load 
	DLLEXPORT void CALLSPEC_STD RVExtensionVersion(char *output, int outputSize);
	//--- STRING callExtension STRING
	DLLEXPORT void CALLSPEC_STD RVExtension(char *output, int outputSize, const char *function);
	//--- STRING callExtension ARRAY
	DLLEXPORT int CALLSPEC_STD RVExtensionArgs(char *output, int outputSize, const char *function, const char **argv, int argc);

#if defined(__cplusplus)
}
#endif



class dllmain
{
private:
	std::vector<easydatabase::connection> m_connections;
	std::vector<easydatabase::prepared_statement> m_prepared_statements;
	bool m_allowNonPrepared;
public:
	void load_config();
	std::vector<easydatabase::connection>& connections() { return m_connections; }
	std::vector<easydatabase::prepared_statement>& prepared_statements() { return m_prepared_statements; }
	bool allowNonPrepared() { return m_allowNonPrepared; }

	static std::map<std::string, rvextension_callback>& callbacks();
	static dllmain& instance();

};