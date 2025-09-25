#ifndef _SEGFAULT_HANDLER_HPP_
#define _SEGFAULT_HANDLER_HPP_

#define NAPI_VERSION 9
#include <node_api.h>
#include <cassert>

#ifdef _WIN32
#define DBG_EXPORT __declspec(dllexport)
#else
#define DBG_EXPORT
#endif

#define JS_METHOD(NAME) napi_value NAME(napi_env env, napi_callback_info info)
#define RET_UNDEFINED do { napi_value undefined; napi_get_undefined(env, &undefined); return undefined; } while(0)
#define RET_BOOL(VAL) do { napi_value result; napi_get_boolean(env, static_cast<bool>(VAL), &result); return result; } while(0)

#define GET_ARGS(ARGC) \
	size_t argc = ARGC; \
	napi_value args[ARGC]; \
	napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

#define IS_UNDEFINED(VAL) ({ \
	napi_valuetype type; \
	napi_typeof(env, VAL, &type); \
	type == napi_undefined; \
})

#define IS_NULL(VAL) ({ \
	napi_valuetype type; \
	napi_typeof(env, VAL, &type); \
	type == napi_null; \
})

#define IS_EMPTY(VAL) (IS_NULL(VAL) || IS_UNDEFINED(VAL))

#define IS_NUMBER(VAL) ({ \
	napi_valuetype type; \
	napi_typeof(env, VAL, &type); \
	type == napi_number; \
})

#define IS_BOOLEAN(VAL) ({ \
	napi_valuetype type; \
	napi_typeof(env, VAL, &type); \
	type == napi_boolean; \
})

#define GET_INT32_ARG(I, VAR, DEF) \
	int VAR = DEF; \
	if (argc > I && !IS_EMPTY(args[I])) { \
		if (!IS_NUMBER(args[I])) { \
			napi_throw_error(env, nullptr, "Argument " #I " must be a number or null/undefined"); \
			RET_UNDEFINED; \
		} \
		napi_get_value_int32(env, args[I], &VAR); \
	}

#define LET_INT32_ARG(I, VAR) GET_INT32_ARG(I, VAR, 0)

#define GET_BOOL_ARG(I, VAR, DEF) \
	bool VAR = DEF; \
	if (argc > I && !IS_EMPTY(args[I])) { \
		if (!IS_BOOLEAN(args[I])) { \
			napi_throw_error(env, nullptr, "Argument " #I " must be a boolean or null/undefined"); \
			RET_UNDEFINED; \
		} \
		napi_get_value_bool(env, args[I], &VAR); \
	}

#define LET_BOOL_ARG(I, VAR) GET_BOOL_ARG(I, VAR, false)


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
