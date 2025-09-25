#ifndef _SEGFAULT_HANDLER_HPP_
#define _SEGFAULT_HANDLER_HPP_

#define NODE_ADDON_API_DISABLE_DEPRECATED
#define NAPI_DISABLE_CPP_EXCEPTIONS
#define NAPI_VERSION 9
#include <napi.h>

#ifdef _WIN32
#define DBG_EXPORT __declspec(dllexport)
#else
#define DBG_EXPORT
#endif

#define NAPI_ENV Napi::Env env = info.Env();
#define JS_METHOD(NAME) Napi::Value NAME(const Napi::CallbackInfo &info)
#define RET_UNDEFINED return env.Undefined()
#define RET_BOOL(VAL) return Napi::Boolean::New(env, static_cast<bool>(VAL))

#define IS_EMPTY(VAL) (VAL.IsNull() || VAL.IsUndefined())
#define IS_ARG_EMPTY(I) IS_EMPTY(info[I])

#define CHECK_LET_ARG(I, C, T) \
	if (!(IS_ARG_EMPTY(I) || info[I].C)) { \
		Napi::Error::New(env, "Argument " #I " must be of type `" T "` or be `null`/`undefined`").ThrowAsJavaScriptException(); \
		return env.Undefined(); \
	}

#define USE_INT32_ARG(I, VAR, DEF) \
	CHECK_LET_ARG(I, IsNumber(), "Int32"); \
	int VAR = IS_ARG_EMPTY(I) ? (DEF) : info[I].ToNumber().Int32Value();

#define LET_INT32_ARG(I, VAR) USE_INT32_ARG(I, VAR, 0)

#define USE_BOOL_ARG(I, VAR, DEF) \
	CHECK_LET_ARG(I, IsBoolean(), "Bool"); \
	bool VAR = IS_ARG_EMPTY(I) ? (DEF) : info[I].ToBoolean().Value();

#define LET_BOOL_ARG(I, VAR) USE_BOOL_ARG(I, VAR, false)


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
