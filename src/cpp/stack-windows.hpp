#ifndef _STACK_WINDOWS_HPP_
#define _STACK_WINDOWS_HPP_

#include <fstream>

#include "bindings.hpp"


namespace segfault {
	EXPORT void showCallstack(std::ofstream &outfile);
}

#endif /* _STACK_WINDOWS_HPP_ */
