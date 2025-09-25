#include <map>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <cstring>
#include <inttypes.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#include "stack-windows.hpp"
#else
#include <signal.h>
#include <unistd.h>
#ifdef __has_include
  #if __has_include(<execinfo.h>)
    #include <execinfo.h>
    #define HAVE_EXECINFO_H 1
    #define HAVE_LIBUNWIND_H 0
  #elif __has_include(<libunwind.h>)
    #include <libunwind.h>
    #define HAVE_EXECINFO_H 0
    #define HAVE_LIBUNWIND_H 1
  #else
    #define HAVE_EXECINFO_H 0
    #define HAVE_LIBUNWIND_H 0
  #endif
#else
  // Fallback for older compilers - try execinfo first
  #include <execinfo.h>
  #define HAVE_EXECINFO_H 1
  #define HAVE_LIBUNWIND_H 0
#endif
#endif

#include "segfault-handler.hpp"


namespace segfault {

// Configuration: true for JSON output, false for plain text output
bool useJsonOutput = false;

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
	
	size_t stackBytes = SIGSTKSZ;
	char* _altStackBytes = new char[stackBytes];

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-narrowing"
#endif
	stack_t _altStack = {
		_altStackBytes,
		0,
		stackBytes
	};
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif

time_t timeInfo;

const std::map<uint32_t, std::string> signalNames = {
#ifdef _WIN32
#define EXCEPTION_ALL 0x0
	{ EXCEPTION_ALL, "CAPTURE ALL THE EXCEPTIONS" },
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
	{ STATUS_STACK_BUFFER_OVERRUN, "STACK_BUFFER_OVERRUN" },
//	{ EXCEPTION_POSSIBLE_DEADLOCK, "POSSIBLE_DEADLOCK" },
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
	{ EXCEPTION_ALL, false },
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
	{ STATUS_STACK_BUFFER_OVERRUN, false },
//	{ EXCEPTION_POSSIBLE_DEADLOCK, false },
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
	return signalActivity[EXCEPTION_ALL] || (
		signalNames.count(signalId) && signalActivity.count(signalId) && signalActivity[signalId]
	);
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


// Write JSON stack trace to stderr
static inline void _writeJsonStackTrace(uint32_t signalId, uint64_t address) {
	constexpr int STDERR_FD = 2;

	// Get current time in ISO format
	time_t now = time(0);
	struct tm *utc_tm = gmtime(&now);
	char timestamp[32];
	strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S.000Z", utc_tm);

	// Start JSON object with timestamp and level
	const char* json_start = "{\"time\":\"";
	write(STDERR_FD, json_start, strlen(json_start));
	write(STDERR_FD, timestamp, strlen(timestamp));

	const char* level_part = "\",\"level\":\"ERROR\",\"type\":\"segfault\",\"signal\":";
	write(STDERR_FD, level_part, strlen(level_part));

	// Write signal ID
	char signal_str[32];
	int signal_len = snprintf(signal_str, sizeof(signal_str), "%u", signalId);
	write(STDERR_FD, signal_str, signal_len);

	// Write signal name
	const char* signal_name_prefix = ",\"signal_name\":\"";
	write(STDERR_FD, signal_name_prefix, strlen(signal_name_prefix));

	std::string signalName;
	if (signalNames.count(signalId)) {
		signalName = signalNames.at(signalId);
	} else {
		signalName = std::to_string(signalId);
	}
	write(STDERR_FD, signalName.c_str(), signalName.length());

	// Write human-readable message
	const char* msg_prefix = "\",\"message\":\"Process ";
	write(STDERR_FD, msg_prefix, strlen(msg_prefix));

	int pid = GETPID();
	char pid_msg[16];
	int pid_msg_len = snprintf(pid_msg, sizeof(pid_msg), "%d", pid);
	write(STDERR_FD, pid_msg, pid_msg_len);

	const char* msg_middle = " received ";
	write(STDERR_FD, msg_middle, strlen(msg_middle));
	write(STDERR_FD, signalName.c_str(), signalName.length());
	const char* msg_end = " signal";
	write(STDERR_FD, msg_end, strlen(msg_end));

	// Write address
	const char* addr_prefix = "\",\"address\":\"0x";
	write(STDERR_FD, addr_prefix, strlen(addr_prefix));

	char addr_str[32];
	int addr_len = snprintf(addr_str, sizeof(addr_str), "%" PRIx64, address);
	write(STDERR_FD, addr_str, addr_len);

	// Write PID
	const char* pid_prefix = "\",\"pid\":";
	write(STDERR_FD, pid_prefix, strlen(pid_prefix));
	write(STDERR_FD, pid_msg, pid_msg_len);

	// Start stack trace array
	const char* stack_prefix = ",\"stack\":[";
	write(STDERR_FD, stack_prefix, strlen(stack_prefix));

#ifdef _WIN32
	// TODO: Implement Windows JSON stack trace
	const char* placeholder = "{\"frame\":0,\"address\":\"0x0\",\"symbol\":\"<windows_stack_not_implemented>\"}";
	write(STDERR_FD, placeholder, strlen(placeholder));
#else
#if HAVE_EXECINFO_H
	void *array[32];
	size_t size = backtrace(array, 32);
	char **symbols = backtrace_symbols(array, size);

	for (size_t i = 0; i < size; i++) {
		if (i > 0) {
			const char* comma = ",";
			write(STDERR_FD, comma, 1);
		}

		const char* frame_start = "{\"frame\":";
		write(STDERR_FD, frame_start, strlen(frame_start));

		char frame_num[16];
		int frame_len = snprintf(frame_num, sizeof(frame_num), "%zu", i);
		write(STDERR_FD, frame_num, frame_len);

		const char* addr_start = ",\"address\":\"";
		write(STDERR_FD, addr_start, strlen(addr_start));

		char addr_hex[32];
		int hex_len = snprintf(addr_hex, sizeof(addr_hex), "%p", array[i]);
		write(STDERR_FD, addr_hex, hex_len);

		const char* symbol_start = "\",\"symbol\":\"";
		write(STDERR_FD, symbol_start, strlen(symbol_start));

		if (symbols && symbols[i]) {
			// Escape any quotes in the symbol name
			const char* symbol = symbols[i];
			while (*symbol) {
				if (*symbol == '"' || *symbol == '\\') {
					const char* escape = "\\";
					write(STDERR_FD, escape, 1);
				}
				write(STDERR_FD, symbol, 1);
				symbol++;
			}
		} else {
			const char* unknown = "<unknown>";
			write(STDERR_FD, unknown, strlen(unknown));
		}

		const char* frame_end = "\"}";
		write(STDERR_FD, frame_end, strlen(frame_end));
	}

	if (symbols) {
		free(symbols);
	}

#elif HAVE_LIBUNWIND_H
	// Use libunwind for stack unwinding with robust error handling
	unw_cursor_t cursor;
	unw_context_t context;
	unw_word_t ip, sp, off;
	char symbol[256];
	int frame = 0;
	bool first_frame = true;

	// Initialize libunwind with error checking
	int getcontext_ret = unw_getcontext(&context);
	if (getcontext_ret != 0) {
		const char* error_frame = "{\"frame\":0,\"address\":\"0x0\",\"symbol\":\"<libunwind_getcontext_failed>\"}";
		write(STDERR_FD, error_frame, strlen(error_frame));
	} else {
		int init_ret = unw_init_local(&cursor, &context);
		if (init_ret != 0) {
			const char* error_frame = "{\"frame\":0,\"address\":\"0x0\",\"symbol\":\"<libunwind_init_failed>\"}";
			write(STDERR_FD, error_frame, strlen(error_frame));
		} else {
			// Successfully initialized - unwind the stack
			int step_ret;
			bool frames_written = false;

			while ((step_ret = unw_step(&cursor)) > 0 && frame < 32) {
				if (!first_frame) {
					const char* comma = ",";
					write(STDERR_FD, comma, 1);
				}
				first_frame = false;
				frames_written = true;

				// Get registers with error handling
				ip = 0;
				sp = 0;
				int ip_ret = unw_get_reg(&cursor, UNW_REG_IP, &ip);
				unw_get_reg(&cursor, UNW_REG_SP, &sp);

				// Format frame start
				char frame_start[64];
				int frame_start_len = snprintf(frame_start, sizeof(frame_start),
					"{\"frame\":%d,\"address\":\"0x%lx\",\"symbol\":\"", frame, ip);
				write(STDERR_FD, frame_start, frame_start_len);

				// Get symbol name
				symbol[0] = '\0';
				int symbol_ret = unw_get_proc_name(&cursor, symbol, sizeof(symbol), &off);

				if (symbol_ret == 0 && symbol[0] != '\0') {
					// Write symbol with offset information
					const char* symbol_ptr = symbol;
					while (*symbol_ptr) {
						if (*symbol_ptr == '"' || *symbol_ptr == '\\') {
							const char* escape = "\\";
							write(STDERR_FD, escape, 1);
						}
						write(STDERR_FD, symbol_ptr, 1);
						symbol_ptr++;
					}

					// Add offset if available
					if (off > 0) {
						char offset_info[32];
						int offset_len = snprintf(offset_info, sizeof(offset_info), " + %lu", off);
						write(STDERR_FD, offset_info, offset_len);
					}
				} else if (ip_ret == 0) {
					const char* unknown_with_addr = "<unknown>";
					write(STDERR_FD, unknown_with_addr, strlen(unknown_with_addr));
				} else {
					const char* no_info = "<no_address_or_symbol>";
					write(STDERR_FD, no_info, strlen(no_info));
				}

				const char* frame_end = "\"}";
				write(STDERR_FD, frame_end, strlen(frame_end));
				frame++;
			}

			// If we didn't write any frames, write a fallback
			if (!frames_written) {
				const char* no_frames = "{\"frame\":0,\"address\":\"0x0\",\"symbol\":\"<no_frames_unwound>\"}";
				write(STDERR_FD, no_frames, strlen(no_frames));
			} else if (step_ret < 0) {
				// Add error frame if stepping failed
				const char* comma = ",";
				write(STDERR_FD, comma, 1);
				char error_frame[128];
				int error_len = snprintf(error_frame, sizeof(error_frame),
					"{\"frame\":%d,\"address\":\"0x0\",\"symbol\":\"<unwind_step_error_%d>\"}", frame, step_ret);
				write(STDERR_FD, error_frame, error_len);
			}
		}
	}
#else
	const char* no_stack = "{\"frame\":0,\"address\":\"0x0\",\"symbol\":\"<no_stack_trace_available>\"}";
	write(STDERR_FD, no_stack, strlen(no_stack));
#endif
#endif

	// Close JSON object
	const char* json_end = "]}\n";
	write(STDERR_FD, json_end, strlen(json_end));
}

// Write stack trace to outfile and cerr
static inline void _writeStackTrace(std::ofstream &outfile, uint32_t signalId) {
#ifdef _WIN32
	showCallstack(outfile);
#else
	outfile.close();

#if HAVE_EXECINFO_H
	void *array[32];
	size_t size = backtrace(array, 32);

	constexpr int STDERR_FD = 2;
	backtrace_symbols_fd(array, size, STDERR_FD);

	int fd = open("segfault.log", O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH);
	if (fd > 0) {
		backtrace_symbols_fd(array, size, fd);
		close(fd);
	}
#elif HAVE_LIBUNWIND_H
	// Use libunwind for stack unwinding (better for Alpine/musl) with robust error handling
	unw_cursor_t cursor;
	unw_context_t context;
	unw_word_t ip, sp, off;
	char symbol[256];
	int frame = 0;
	constexpr int STDERR_FD = 2;

	const char* header = "Stack trace (libunwind):\n";
	write(STDERR_FD, header, strlen(header));

	int fd = open("segfault.log", O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH);
	if (fd > 0) {
		write(fd, header, strlen(header));
	}

	// Initialize libunwind with error checking
	int getcontext_ret = unw_getcontext(&context);
	if (getcontext_ret != 0) {
		const char* error_msg = "Error: Failed to get libunwind context\n";
		write(STDERR_FD, error_msg, strlen(error_msg));
		if (fd > 0) {
			write(fd, error_msg, strlen(error_msg));
		}
	} else {
		int init_ret = unw_init_local(&cursor, &context);
		if (init_ret != 0) {
			const char* error_msg = "Error: Failed to initialize libunwind cursor\n";
			write(STDERR_FD, error_msg, strlen(error_msg));
			if (fd > 0) {
				write(fd, error_msg, strlen(error_msg));
			}
		} else {
			// Successfully initialized - unwind the stack
			int step_ret;
			bool frames_written = false;

			while ((step_ret = unw_step(&cursor)) > 0 && frame < 32) {
				frames_written = true;

				// Get registers with error handling
				ip = 0;
				sp = 0;
				int ip_ret = unw_get_reg(&cursor, UNW_REG_IP, &ip);
				unw_get_reg(&cursor, UNW_REG_SP, &sp);

				symbol[0] = '\0';
				int symbol_ret = unw_get_proc_name(&cursor, symbol, sizeof(symbol), &off);

				if (symbol_ret == 0 && symbol[0] != '\0') {
					char buffer[512];
					int len = snprintf(buffer, sizeof(buffer), "%2d: 0x%lx <%s+0x%lx>\n",
						frame, ip, symbol, off);
					write(STDERR_FD, buffer, len);
					if (fd > 0) {
						write(fd, buffer, len);
					}
				} else {
					char buffer[256];
					int len;
					if (ip_ret == 0) {
						len = snprintf(buffer, sizeof(buffer), "%2d: 0x%lx <unknown>\n", frame, ip);
					} else {
						len = snprintf(buffer, sizeof(buffer), "%2d: <no_address> <unknown>\n", frame);
					}
					write(STDERR_FD, buffer, len);
					if (fd > 0) {
						write(fd, buffer, len);
					}
				}
				frame++;
			}

			// Check if we failed to get any frames or if step failed
			if (!frames_written) {
				const char* no_frames_msg = "Warning: No stack frames could be unwound\n";
				write(STDERR_FD, no_frames_msg, strlen(no_frames_msg));
				if (fd > 0) {
					write(fd, no_frames_msg, strlen(no_frames_msg));
				}
			} else if (step_ret < 0) {
				const char* step_error_msg = "Warning: Stack unwinding terminated due to error\n";
				write(STDERR_FD, step_error_msg, strlen(step_error_msg));
				if (fd > 0) {
					write(fd, step_error_msg, strlen(step_error_msg));
				}
			}
		}
	}

	if (fd > 0) {
		close(fd);
	}
#else
	// Neither execinfo nor libunwind available
	constexpr int STDERR_FD = 2;
	const char* msg = "Stack trace not available (no unwinding library found)\n";
	write(STDERR_FD, msg, strlen(msg));

	int fd = open("segfault.log", O_CREAT | O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IROTH);
	if (fd > 0) {
		write(fd, msg, strlen(msg));
		close(fd);
	}
#endif
#endif
}


static inline std::ofstream _openLogFile() {
	std::ofstream outfile;
	
	if (!std::filesystem::exists("segfault.log")) {
		std::cerr
			<< "SegfaultHandler: The exception won't be logged into a file"
			<< ", unless 'segfault.log' exists." << std::endl;
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
	std::string signalName;
	if (signalNames.count(signalId)) {
		signalName = signalNames.at(signalId);
	} else {
		signalName = std::to_string(signalId);
	}
	stream
		<< "\nPID " << pid << " received " << signalName
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


DBG_EXPORT SEGFAULT_HANDLER {
	auto signalAndAdress = _getSignalAndAddress(info);
	uint32_t signalId = signalAndAdress.first;
	uint64_t address = signalAndAdress.second;

	if (!_isSignalEnabled(signalId)) {
		HANDLER_CANCEL;
	}

	if (useJsonOutput) {
		// Write JSON stack trace to stderr
		_writeJsonStackTrace(signalId, address);
	} else {
		// Write traditional output
		std::ofstream outfile = _openLogFile();

		_writeTimeToFile(outfile);
		_writeLogHeader(outfile, signalId, address);
		_writeStackTrace(outfile, signalId);

		_closeLogFile(outfile);
	}

	HANDLER_DONE;
}


DBG_EXPORT void setJsonOutputMode(bool jsonOutput) {
	useJsonOutput = jsonOutput;
}

DBG_EXPORT bool getJsonOutputMode() {
	return useJsonOutput;
}


// create some stack frames to inspect from CauseSegfault
DBG_EXPORT NO_INLINE void _segfaultStackFrame1() {
	int *foo = reinterpret_cast<int*>(1);
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
	*foo = 42; // triggers a segfault exception (intentional)
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
}

DBG_EXPORT NO_INLINE void _segfaultStackFrame2(void) {
	void (*fn_ptr)() = _segfaultStackFrame1;
	fn_ptr();
}


DBG_EXPORT JS_METHOD(causeSegfault) { NAPI_ENV;
	std::cout << "SegfaultHandler: about to cause a segfault..." << std::endl;
	void (*fn_ptr)() = _segfaultStackFrame2;
	fn_ptr();
	RET_UNDEFINED;
}

DBG_EXPORT NO_INLINE void _divideInt() {
	volatile int a = 42;
	volatile int b = 0;
	a /= b;
}


DBG_EXPORT JS_METHOD(causeDivisionInt) { NAPI_ENV;
	_divideInt();
	RET_UNDEFINED;
}


DBG_EXPORT NO_INLINE void _overflowStack() {
	int foo[1000];
	foo[999] = 1;
	std::vector<int> empty = { foo[999] };
	if (empty.size() != 1) {
		return;
	}
	_overflowStack(); // infinite recursion
}

DBG_EXPORT JS_METHOD(causeOverflow) { NAPI_ENV;
	std::cout << "SegfaultHandler: about to overflow the stack..." << std::endl;
	_overflowStack();
	RET_UNDEFINED;
}


DBG_EXPORT JS_METHOD(causeIllegal) { NAPI_ENV;
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


DBG_EXPORT JS_METHOD(setSignal) { NAPI_ENV;
	if (IS_ARG_EMPTY(0)) {
		RET_UNDEFINED;
	}
	
	LET_INT32_ARG(0, signalId);
	LET_BOOL_ARG(1, value);
	
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


DBG_EXPORT void init() {
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

DBG_EXPORT JS_METHOD(setOutputFormat) { NAPI_ENV;
	if (IS_ARG_EMPTY(0)) {
		RET_UNDEFINED;
	}

	LET_BOOL_ARG(0, jsonOutput);
	setJsonOutputMode(jsonOutput);

	RET_UNDEFINED;
}

DBG_EXPORT JS_METHOD(getOutputFormat) { NAPI_ENV;
	RET_BOOL(getJsonOutputMode());
}

} // namespace segfault
