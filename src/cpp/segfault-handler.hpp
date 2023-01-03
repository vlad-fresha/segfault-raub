#ifndef _SEGFAULT_HANDLER_HPP_
#define _SEGFAULT_HANDLER_HPP_

#include <addon-tools.hpp>


namespace segfault {
	EXPORT void init();
	EXPORT void registerHandler();
	
	EXPORT JS_METHOD(causeSegfault);
	EXPORT JS_METHOD(causeDivisionInt);
	EXPORT JS_METHOD(causeOverflow);
	EXPORT JS_METHOD(causeIllegal);
	EXPORT JS_METHOD(setSignal);
}


#endif /* _SEGFAULT_HANDLER_HPP_ */
