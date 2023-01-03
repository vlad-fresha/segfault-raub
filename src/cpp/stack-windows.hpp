#ifndef _STACK_WINDOWS_HPP_
#define _STACK_WINDOWS_HPP_

#include <fstream>
#include <addon-tools.hpp>


namespace segfault {
	DBG_EXPORT void showCallstack(std::ofstream &outfile);
}

#endif /* _STACK_WINDOWS_HPP_ */
