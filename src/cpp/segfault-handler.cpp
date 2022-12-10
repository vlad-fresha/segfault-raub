#include <memory>
#include <map>
#include <string>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include "stack-walker.hpp"
#else
#include <ccignal>
#endif

#include "segfault-handler.hpp"


namespace segfault {

constexpr int STDERR_FD = 2;
constexpr size_t BUFF_SIZE = 256;
constexpr int F_OK = 0;

#ifdef _WIN32
	constexpr auto CLOSE = _close;
	constexpr auto GETPID = _getpid;
	constexpr auto OPEN = _open;
	constexpr auto SNPRINTF = _snprintf;
	constexpr auto WRITE = _write;
	constexpr int S_FLAGS = _S_IWRITE;
	constexpr int O_FLAGS = _O_CREAT | _O_APPEND | _O_WRONLY;
	#define SEGFAULT_HANDLER LONG CALLBACK handleSegfault(PEXCEPTION_POINTERS info)
	#define NO_INLINE __declspec(noinline)
	#define HANDLER_CANCEL return EXCEPTION_CONTINUE_SEARCH
	#define HANDLER_DONE return EXCEPTION_EXECUTE_HANDLER
#else
	constexpr auto CLOSE = close;
	constexpr auto GETPID = getpid;
	constexpr auto OPEN = open;
	constexpr auto SNPRINTF = snprintf;
	constexpr auto WRITE = write;
	constexpr int S_FLAGS = S_IRUSR | S_IRGRP | S_IROTH;
	constexpr int O_FLAGS = O_CREAT | O_APPEND | O_WRONLY;
	#define SEGFAULT_HANDLER static void handleSegfault(int sig, siginfo_t *info, void *unused)
	#define NO_INLINE __attribute__ ((noinline))
	#define HANDLER_CANCEL return
	#define HANDLER_DONE return
#endif

char sbuff[BUFF_SIZE];
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
	{ SIGKILL, "SIGKILL" },
	{ SIGPIPE, "SIGPIPE" },
	{ SIGQUIT, "SIGQUIT" },
	{ SIGSTOP, "SIGSTOP" },
	{ SIGTSTP, "SIGTSTP" },
	{ SIGTTIN, "SIGTTIN" },
	{ SIGTTOU, "SIGTTOU" },
	{ SIGUSR1, "SIGUSR1" },
	{ SIGUSR2, "SIGUSR2" },
	{ SIGPOLL, "SIGPOLL" },
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
	{ SIGPOLL, false },
	{ SIGPROF, false },
	{ SIGSYS, false },
	{ SIGTRAP, false },
	{ SIGURG, false },
	{ SIGVTALRM, false },
	{ SIGXCPU, false },
	{ SIGXFSZ, false },
#endif
};


bool _isSignalEnabled(uint32_t signal) {
#ifdef _WIN32
	return (signalNames.count(signal) && signalActivity.count(signal) && signalActivity[signal]);
#else
	return true;
#endif
}


#ifdef _WIN32
std::pair<uint32_t, uint64_t> _getSignalAndAddress(PEXCEPTION_POINTERS info) {
	auto r = info->ExceptionRecord;
	return { static_cast<uint32_t>(r->ExceptionCode), reinterpret_cast<uint64_t>(r->ExceptionAddress) };
}
#else
std::pair<uint32_t, uint64_t> _getSignalAndAddress(siginfo_t *info) {
	return { static_cast<uint32_t>(info->si_signo), reinterpret_cast<uint64_t>(info->si_addr) };
}
#endif


void _writeStackTrace(int fd, uint32_t signal) {
	// generate the stack trace and write to fd and stderr
#ifdef _WIN32
	if (EXCEPTION_STACK_OVERFLOW != signal) {
		StackWalker sw(fd);
		sw.ShowCallstack();
	}
#else
	void *array[32];
	size_t size = backtrace(array, 32);
	if (fd > 0) {
		backtrace_symbols_fd(array, size, fd);
	}
	backtrace_symbols_fd(array, size, STDERR_FD);
#endif
}


int _openLogFile() {
	int fd = 0;
	
	if (access("segfault.log", F_OK) == -1) {
		fprintf(
			stderr,
			"SegfaultHandler: The exception won't be logged into a file, unless 'segfault.log' exists.\n"
		);
		return fd;
	}
	
	return OPEN("segfault.log", O_FLAGS, S_FLAGS);
}

void _writeTimeToFile(int fd) {
	time(&timeInfo);
	const char *timeStr = ctime(&timeInfo);
	int n = SNPRINTF(sbuff, BUFF_SIZE, "\n\nAt %s", timeStr);
	
	if (fd > 0) {
		if (WRITE(fd, sbuff, n) != n) {
			fprintf(stderr, "SegfaultHandler: Error writing to file.\n");
		}
	}
}

void _writeLogHeader(int fd, uint32_t signal, uint64_t address) {
	int pid = GETPID();
	int n = SNPRINTF(
		sbuff,
		BUFF_SIZE,
		"\nPID %d received %s for address: 0x%llx\n",
		pid,
		signalNames.at(signal).c_str(),
		address
	);
	
	if (fd > 0) {
		if (WRITE(fd, sbuff, n) != n) {
			fprintf(stderr, "SegfaultHandler: Error writing to file\n");
		}
	}
	
	fprintf(stderr, "%s", sbuff);
}

void _writeStackNote(int fd, uint32_t signal, uint64_t address) {
	int pid = GETPID();
	int n = SNPRINTF(
		sbuff,
		BUFF_SIZE,
		"\nPID %d received %s for address: 0x%llx\n",
		pid,
		signalNames.at(signal).c_str(),
		address
	);
	
	if (fd > 0) {
		if (WRITE(fd, sbuff, n) != n) {
			fprintf(stderr, "SegfaultHandler: Error writing to file\n");
		}
	}
	
	fprintf(stderr, "%s", sbuff);
}

void _closeLogFile(int fd) {
	fflush(stderr);
	if (fd) {
		CLOSE(fd);
	}
}


SEGFAULT_HANDLER {
	auto signalAndAdress = _getSignalAndAddress(info);
	uint32_t signal = signalAndAdress.first;
	uint64_t address = signalAndAdress.second;
	
	if (!_isSignalEnabled(signal)) {
		HANDLER_CANCEL;
	}
	
	int fd = _openLogFile();
	
	if (signal != EXCEPTION_STACK_OVERFLOW) {
		_writeTimeToFile(fd);
	}
	
	_writeLogHeader(fd, signal, address);
	
	if (signal != EXCEPTION_STACK_OVERFLOW) {
		_writeStackTrace(fd, signal);
	}
	
	_closeLogFile(fd);
	
	HANDLER_DONE;
}


// create some stack frames to inspect from CauseSegfault
NO_INLINE void _segfaultStackFrame1() {
	int *foo = reinterpret_cast<int*>(1);
	*foo = 78; // triggers a segfault exception
}

NO_INLINE void _segfaultStackFrame2(void) {
	void (*fn_ptr)() = _segfaultStackFrame1;
	fn_ptr();
}


JS_METHOD(causeSegfault) { NAPI_ENV;
	fprintf(stderr, "SegfaultHandler: about to cause a segfault...\n");
	fflush(stderr);
	void (*fn_ptr)() = _segfaultStackFrame2;
	fn_ptr();
	RET_UNDEFINED;
}

NO_INLINE void _divideInt() {
	int a = 1.f;
	std::vector<int> empty = { 1 };
	int b = empty[0] - static_cast<int>(empty.size());
	fprintf(stderr, "SegfaultHandler: about to divide by int 0...\n");
	fflush(stderr);
	int c = a / b; // division by zero
	empty.push_back(c);
}


JS_METHOD(causeDivisionInt) { NAPI_ENV;
	_divideInt();
	RET_UNDEFINED;
}


NO_INLINE void _overflowStack() {
	int foo[1000];
	(void)foo;
	std::vector<int> empty;
	if (empty.size()) {
		return;
	}
	_overflowStack(); // infinite recursion
}

JS_METHOD(causeOverflow) { NAPI_ENV;
	fprintf(stderr, "SegfaultHandler: about to overflow the stack...\n");
	fflush(stderr);
	_overflowStack();
	RET_UNDEFINED;
}


JS_METHOD(causeIllegal) { NAPI_ENV;
	fprintf(stderr, "SegfaultHandler: about to raise an illegal operation...\n");
	fflush(stderr);
#ifdef _WIN32
	RaiseException(EXCEPTION_ILLEGAL_INSTRUCTION, 0, 0, nullptr);
#else
	raise(SIGILL);
#endif
	RET_UNDEFINED;
}


void _enableSignal(uint32_t signal) {
	#ifndef _WIN32
		struct sigaction sa;
		memset(&sa, 0, sizeof(struct sigaction));
		sigemptyset(&sa.sa_mask);
		sa.sa_sigaction = handleSegfault;
		sa.sa_flags = SA_SIGINFO;
		sigaction(SIGSEGV, &sa, NULL);
	#endif
}

void _disableSignal(uint32_t signal) {
	#ifndef _WIN32
		signal(signal, SIG_DFL);
	#endif
}


JS_METHOD(setSignal) { NAPI_ENV;
	REQ_INT32_ARG(0, signal);
	LET_BOOL_ARG(0, value);
	
	if (!signalNames.count(signal)) {
		RET_UNDEFINED;
	}
	
	bool wasEnabled = signalActivity[signal];
	if (wasEnabled == value) {
		RET_UNDEFINED;
	}
	
	signalActivity[signal] = value;
	if (value) {
		_enableSignal(signal);
	} else {
		_disableSignal(signal);
	}
	
	RET_UNDEFINED;
}


void init() {
	#ifdef _WIN32
		SetUnhandledExceptionFilter(handleSegfault);
	#endif
	
	for (auto pair: signalActivity) {
		if (pair.second) {
			_enableSignal(pair.first);
		}
	}
}

}
