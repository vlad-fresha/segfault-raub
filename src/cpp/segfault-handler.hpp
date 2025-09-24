#ifndef _SEGFAULT_HANDLER_HPP_
#define _SEGFAULT_HANDLER_HPP_

#define NAPI_VERSION 9

#include <addon-tools.hpp>


namespace segfault {
	DBG_EXPORT void init();
	DBG_EXPORT void registerHandler();

	DBG_EXPORT void setJsonOutputMode(bool jsonOutput);
	DBG_EXPORT bool getJsonOutputMode();

	DBG_EXPORT JS_METHOD(causeSegfault);
	DBG_EXPORT JS_METHOD(causeDivisionInt);
	DBG_EXPORT JS_METHOD(causeOverflow);
	DBG_EXPORT JS_METHOD(causeIllegal);
	DBG_EXPORT JS_METHOD(setSignal);
	DBG_EXPORT JS_METHOD(setOutputFormat);
	DBG_EXPORT JS_METHOD(getOutputFormat);
}


#endif /* _SEGFAULT_HANDLER_HPP_ */
