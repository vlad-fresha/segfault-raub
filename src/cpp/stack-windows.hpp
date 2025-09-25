#ifndef _STACK_WINDOWS_HPP_
#define _STACK_WINDOWS_HPP_

#define NAPI_VERSION 9

#ifdef _WIN32
#define DBG_EXPORT __declspec(dllexport)
#else
#define DBG_EXPORT
#endif

#include <fstream>


namespace segfault {
	DBG_EXPORT void showCallstack(std::ofstream &outfile);
}

#endif /* _STACK_WINDOWS_HPP_ */
