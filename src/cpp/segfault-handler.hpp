#ifndef _SEGFAULT_HANDLER_HPP_
#define _SEGFAULT_HANDLER_HPP_

#include <addon-tools.hpp>


namespace segfault {
	DBG_EXPORT void init();
	DBG_EXPORT void registerHandler();
	
	DBG_EXPORT JS_METHOD(causeSegfault);
	DBG_EXPORT JS_METHOD(causeDivisionInt);
	DBG_EXPORT JS_METHOD(causeOverflow);
	DBG_EXPORT JS_METHOD(causeIllegal);
	DBG_EXPORT JS_METHOD(setSignal);
}


#endif /* _SEGFAULT_HANDLER_HPP_ */
