#ifndef _SEGFAULT_HANDLER_HPP_
#define _SEGFAULT_HANDLER_HPP_

#include <addon-tools.hpp>


namespace segfault {
	void init();
	void registerHandler();
	
	JS_METHOD(causeSegfault);
	JS_METHOD(causeDivisionInt);
	JS_METHOD(causeOverflow);
	JS_METHOD(causeIllegal);
	JS_METHOD(setSignal);
}


#endif /* _SEGFAULT_HANDLER_HPP_ */
