#include "segfault-handler.hpp"

#include <map>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include "stack-windows.hpp"
#else
#include <signal.h>
#include <unistd.h>
#include <execinfo.h>
#endif


namespace segfault {

#ifdef _WIN32
	constexpr auto GETPID = _getpid;
	#define SEGFAULT_HANDLER LONG CALLBACK handleSignal(PEXCEPTION_POINTERS info)
	#define NO_INLINE __declspec(noinline)
	#define HANDLER_CANCEL return EXCEPTION_CONTINUE_SEARCH
	#define HANDLER_DONE return EXCEPTION_EXECUTE_HANDLER
#else
	constexpr auto GETPID = getpid;
	#define SEGFAULT_HANDLER static void handleSignal(int sig, siginfo_t *info, void *unused)
	#define NO_INLINE __attribute__ ((noinline))
	#define HANDLER_CANCEL return
	#define HANDLER_DONE return
	
	char _altStackBytes[SIGSTKSZ];
	stack_t _altStack = {
		_altStackBytes,
		0,
		SIGSTKSZ,
	};
#endif

time_t timeInfo;

const std::map<uint32_t, std::string> signalNames = {
#ifdef _WIN32
	{ EXCEPTION_ACCESS_VIOLATION, "ACCESS_VIOLATION" },
	{ EXCEPTION_DATATYPE_MISALIGNMENT, "DATATYPE_MISALIGNMENT" },
	{ EXCEPTION_BREAKPOINT, "BREAKPOINT" },
	{ EXCEPTION_SINGLE_STEP, "SINGLE_STEP" },
	{ EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "ARRAY_BOUNDS_EXCEEDED" },
	{ EXCEPTION_FLT_DENORMAL_OPERAND, "FLT_DENORMAL_OPERAND" },
	{ EXCEPTION_FLT_DIVIDE_BY_ZERO, "FLT_DIVIDE_BY_ZERO" },
	{ EXCEPTION_FLT_INEXACT_RESULT, "FLT_INEXACT_RESULT" },
	{ EXCEPTION_FLT_INVALID_OPERATION, "FLT_INVALID_OPERATION" },
	{ EXCEPTION_FLT_OVERFLOW, "FLT_OVERFLOW" },
	{ EXCEPTION_FLT_STACK_CHECK, "FLT_STACK_CHECK" },
	{ EXCEPTION_FLT_UNDERFLOW, "FLT_UNDERFLOW" },
	{ EXCEPTION_INT_DIVIDE_BY_ZERO, "INT_DIVIDE_BY_ZERO" },
	{ EXCEPTION_INT_OVERFLOW, "INT_OVERFLOW" },
	{ EXCEPTION_PRIV_INSTRUCTION, "PRIV_INSTRUCTION" },
	{ EXCEPTION_IN_PAGE_ERROR, "IN_PAGE_ERROR" },
	{ EXCEPTION_ILLEGAL_INSTRUCTION, "ILLEGAL_INSTRUCTION" },
	{ EXCEPTION_NONCONTINUABLE_EXCEPTION, "NONCONTINUABLE_EXCEPTION" },
	{ EXCEPTION_STACK_OVERFLOW, "STACK_OVERFLOW" },
	{ EXCEPTION_INVALID_DISPOSITION, "INVALID_DISPOSITION" },
	{ EXCEPTION_GUARD_PAGE, "GUARD_PAGE" },
	{ EXCEPTION_INVALID_HANDLE, "INVALID_HANDLE" },
#else
	{ SIGABRT, "SIGABRT" },
	{ SIGFPE, "SIGFPE" },
	{ SIGSEGV, "SIGSEGV" },
	{ SIGTERM, "SIGTERM" },
	{ SIGILL, "SIGILL" },
	{ SIGINT, "SIGINT" },
	{ SIGALRM, "SIGALRM" },
	{ SIGBUS, "SIGBUS" },
	{ SIGCHLD, "SIGCHLD" },
	{ SIGCONT, "SIGCONT" },
	{ SIGHUP, "SIGHUP" },
	{ SIGKILL, "SIGKIL" },
	{ SIGPIPE, "SIGPIPE" },
	{ SIGQUIT, "SIGQUIT" },
	{ SIGSTOP, "SIGSTOP" },
	{ SIGTSTP, "SIGTSTP" },
	{ SIGTTIN, "SIGTTIN" },
	{ SIGTTOU, "SIGTTOU" },
	{ SIGUSR1, "SIGUSR1" },
	{ SIGUSR2, "SIGUSR2" },
	{ SIGPROF, "SIGPROF" },
	{ SIGSYS, "SIGSYS" },
	{ SIGTRAP, "SIGTRAP" },
	{ SIGURG, "SIGURG" },
	{ SIGVTALRM, "SIGVTALRM" },
	{ SIGXCPU, "SIGXCPU" },
	{ SIGXFSZ, "SIGXFSZ" },
#endif
};


std::map<uint32_t, bool> signalActivity = {
#ifdef _WIN32
	{ EXCEPTION_ACCESS_VIOLATION, true },
	{ EXCEPTION_ARRAY_BOUNDS_EXCEEDED, true },
	{ EXCEPTION_FLT_DIVIDE_BY_ZERO, true },
	{ EXCEPTION_INT_DIVIDE_BY_ZERO, true },
	{ EXCEPTION_ILLEGAL_INSTRUCTION, true },
	{ EXCEPTION_NONCONTINUABLE_EXCEPTION, true },
	{ EXCEPTION_STACK_OVERFLOW, true },
	{ EXCEPTION_INVALID_HANDLE, true },
	{ EXCEPTION_DATATYPE_MISALIGNMENT, false },
	{ EXCEPTION_BREAKPOINT, false },
	{ EXCEPTION_SINGLE_STEP, false },
	{ EXCEPTION_FLT_DENORMAL_OPERAND, false },
	{ EXCEPTION_FLT_INEXACT_RESULT, false },
	{ EXCEPTION_FLT_INVALID_OPERATION, false },
	{ EXCEPTION_FLT_OVERFLOW, false },
	{ EXCEPTION_FLT_STACK_CHECK, false },
	{ EXCEPTION_FLT_UNDERFLOW, false },
	{ EXCEPTION_INT_OVERFLOW, false },
	{ EXCEPTION_PRIV_INSTRUCTION, false },
	{ EXCEPTION_IN_PAGE_ERROR, false },
	{ EXCEPTION_INVALID_DISPOSITION, false },
	{ EXCEPTION_GUARD_PAGE, false },
#else
	{ SIGABRT, true },
	{ SIGFPE, true },
	{ SIGSEGV, true },
	{ SIGILL, true },
	{ SIGBUS, true },
	{ SIGTERM, false },
	{ SIGINT, false },
	{ SIGALRM, false },
	{ SIGCHLD, false },
	{ SIGCONT, false },
	{ SIGHUP, false },
	{ SIGKILL, false },
	{ SIGPIPE, false },
	{ SIGQUIT, false },
	{ SIGSTOP, false },
	{ SIGTSTP, false },
	{ SIGTTIN, false },
	{ SIGTTOU, false },
	{ SIGUSR1, false },
	{ SIGUSR2, false },
	{ SIGPROF, false },
	{ SIGSYS, false },
	{ SIGTRAP, false },
	{ SIGURG, false },
	{ SIGVTALRM, false },
	{ SIGXCPU, false },
	{ SIGXFSZ, false },
#endif
};


static inline bool _isSignalEnabled(uint32_t signalId) {
#ifdef _WIN32
	return (signalNames.count(signalId) && signalActivity.count(signalId) && signalActivity[signalId]);
#else
	return true;
#endif
}


#ifdef _WIN32
static inline std::pair<uint32_t, uint64_t> _getSignalAndAddress(PEXCEPTION_POINTERS info) {
	auto r = info->ExceptionRecord;
	return { static_cast<uint32_t>(r->ExceptionCode), reinterpret_cast<uint64_t>(r->ExceptionAddress) };
}
#else
static inline std::pair<uint32_t, uint64_t> _getSignalAndAddress(siginfo_t *info) {
	return { static_cast<uint32_t>(info->si_signo), reinterpret_cast<uint64_t>(info->si_addr) };
}
#endif


// Write stack trace to outfile and cerr
static inline void _writeStackTrace(std::ofstream &outfile, uint32_t signalId) {
#ifdef _WIN32
	showCallstack(outfile);
#else
	outfile.close();
	void *array[32];
	size_t size = backtrace(array, 32);
	int fd = open("segfault.log", O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH);
	if (fd > 0) {
		backtrace_symbols_fd(array, size, fd);
		close(fd);
	}
	constexpr int STDERR_FD = 2;
	backtrace_symbols_fd(array, size, STDERR_FD);
#endif
}


static inline std::ofstream _openLogFile() {
	std::ofstream outfile;
	
	if (!std::filesystem::exists("segfault.log")) {
		std::cerr << "SegfaultHandler: The exception won't be logged into a file, unless 'segfault.log' exists." << std::endl;
		return outfile;
	}
	
	outfile.open("segfault.log", std::ofstream::app);
	
	return outfile;
}

static inline void _writeTimeToFile(std::ofstream &outfile) {
	if (!outfile.is_open()) {
		return;
	}
	
	time(&timeInfo);
	
	outfile << "\n\nAt " << ctime(&timeInfo) << std::endl; // NOLINT
	if (outfile.bad()) {
		std::cerr << "SegfaultHandler: Error writing to file." << std::endl;
	}
}

static inline void _writeHeaderToOstream(std::ostream &stream, int pid, uint32_t signalId, uint64_t address) {
	stream
		<< "\nPID " << pid << " received " << signalNames.at(signalId)
		<< " for address: 0x" << std::hex << address << std::endl;
}


static inline void _writeLogHeader(std::ofstream &outfile, uint32_t signalId, uint64_t address) {
	int pid = GETPID();
	_writeHeaderToOstream(std::cerr, pid, signalId, address);
	
	if (!outfile.is_open()) {
		return;
	}
	
	_writeHeaderToOstream(outfile, pid, signalId, address);
	if (outfile.bad()) {
		std::cerr << "SegfaultHandler: Error writing to file." << std::endl;
	}
}

static inline void _closeLogFile(std::ofstream &outfile) {
	if (outfile.is_open()) {
		outfile.close();
	}
}


EXPORT SEGFAULT_HANDLER {
	auto signalAndAdress = _getSignalAndAddress(info);
	uint32_t signalId = signalAndAdress.first;
	uint64_t address = signalAndAdress.second;
	
	if (!_isSignalEnabled(signalId)) {
		HANDLER_CANCEL;
	}
	
	std::ofstream outfile = _openLogFile();
	
	_writeTimeToFile(outfile);
	_writeLogHeader(outfile, signalId, address);
	_writeStackTrace(outfile, signalId);
	
	_closeLogFile(outfile);
	
	HANDLER_DONE;
}


// create some stack frames to inspect from CauseSegfault
EXPORT NO_INLINE void _segfaultStackFrame1() {
	int *foo = reinterpret_cast<int*>(1);
	*foo = 42; // triggers a segfault exception
}

EXPORT NO_INLINE void _segfaultStackFrame2(void) {
	void (*fn_ptr)() = _segfaultStackFrame1;
	fn_ptr();
}


EXPORT JS_METHOD(causeSegfault) { NAPI_ENV;
	std::cout << "SegfaultHandler: about to cause a segfault..." << std::endl;
	void (*fn_ptr)() = _segfaultStackFrame2;
	fn_ptr();
	RET_UNDEFINED;
}

EXPORT NO_INLINE void _divideInt() {
	volatile int a = 42;
	volatile int b = 0;
	a /= b;
}


EXPORT JS_METHOD(causeDivisionInt) { NAPI_ENV;
	_divideInt();
	RET_UNDEFINED;
}


EXPORT NO_INLINE void _overflowStack() {
	int foo[1000];
	foo[999] = 1;
	std::vector<int> empty = { foo[999] };
	if (empty.size() != 1) {
		return;
	}
	_overflowStack(); // infinite recursion
}

EXPORT JS_METHOD(causeOverflow) { NAPI_ENV;
	std::cout << "SegfaultHandler: about to overflow the stack..." << std::endl;
	_overflowStack();
	RET_UNDEFINED;
}


EXPORT JS_METHOD(causeIllegal) { NAPI_ENV;
	std::cout << "SegfaultHandler: about to raise an illegal operation..." << std::endl;
#ifdef _WIN32
	RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION, 0, 0, nullptr);
#else
	raise(SIGILL);
#endif
	RET_UNDEFINED;
}


static inline void _enableSignal(uint32_t signalId) {
	#ifndef _WIN32
		struct sigaction action;
		memset(&action, 0, sizeof(struct sigaction));
		sigemptyset(&action.sa_mask);
		action.sa_sigaction = handleSignal;
		
		if (signalId == SIGSEGV) {
			action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESETHAND;
		} else {
			action.sa_flags = SA_SIGINFO | SA_RESETHAND;
		}
		
		sigaction(signalId, &action, NULL);
	#endif
}

static inline void _disableSignal(uint32_t signalId) {
	#ifndef _WIN32
		signal(signalId, SIG_DFL);
	#endif
}


EXPORT JS_METHOD(setSignal) { NAPI_ENV;
	if (IS_ARG_EMPTY(0)) {
		RET_UNDEFINED;
	}
	
	LET_INT32_ARG(0, signalId);
	LET_BOOL_ARG(0, value);
	
	if (!signalNames.count(signalId)) {
		RET_UNDEFINED;
	}
	
	bool wasEnabled = signalActivity[signalId];
	if (wasEnabled == value) {
		RET_UNDEFINED;
	}
	
	signalActivity[signalId] = value;
	if (value) {
		_enableSignal(signalId);
	} else {
		_disableSignal(signalId);
	}
	
	RET_UNDEFINED;
}


EXPORT void init() {
	// On Windows, a single handler is set on startup.
	// On Unix, handlers for every signal are set on-demand.
	#ifdef _WIN32
		SetUnhandledExceptionFilter(handleSignal);
	#endif
	
	// `SetThreadStackGuarantee` and `sigaltstack` help in handling stack overflows on their platforms.
	#ifdef _WIN32
		ULONG size = 32 * 1024;
		SetThreadStackGuarantee(&size);
	#else
		sigaltstack(&_altStack, nullptr);
	#endif
	
	for (auto pair : signalActivity) {
		if (pair.second) {
			_enableSignal(pair.first);
		}
	}
}

} // namespace segfault
