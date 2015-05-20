#include <string>
extern "C"
{
	__declspec(dllexport) void __stdcall RVExtension(char *output, int outputSize, const char *function);
};
void setBool(char* out, unsigned int maxOut, bool flag);
inline void setLastError(void);
inline void setLastError(const std::string& str);
inline void setLastError(const char* str);
void clearConfig(void);
bool readConfig(void);
inline void updateLastConnectionAccess(void);
void connectionWatch(unsigned long timeout);
bool closeConnection(void);