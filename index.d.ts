/**
 * Produce a segfault
 * Issue an actual segfault, accessing some unavailable memory
 */
export declare const causeSegfault: () => void;
export declare const causeDivisionInt: () => void;
export declare const causeOverflow: () => void;
export declare const causeIllegal: () => void;

/**
 * Enable/disable signal handlers
 * @param signalId Signal ID to configure
 * @param value Whether to enable the signal handler
 */
export declare const setSignal: (signalId: number | null, value: boolean) => void;

/**
 * Set output format for crash reports
 * @param jsonOutput Whether to use JSON format (true) or plain text (false)
 */
export declare const setOutputFormat: (jsonOutput: boolean) => void;

/**
 * Get current output format
 * @returns Current output format (true = JSON, false = plain text)
 */
export declare const getOutputFormat: () => boolean;

// Windows exception constants
export declare const EXCEPTION_ALL: number | null;
export declare const EXCEPTION_ACCESS_VIOLATION: number | null;
export declare const EXCEPTION_DATATYPE_MISALIGNMENT: number | null;
export declare const EXCEPTION_BREAKPOINT: number | null;
export declare const EXCEPTION_SINGLE_STEP: number | null;
export declare const EXCEPTION_ARRAY_BOUNDS_EXCEEDED: number | null;
export declare const EXCEPTION_FLT_DENORMAL_OPERAND: number | null;
export declare const EXCEPTION_FLT_DIVIDE_BY_ZERO: number | null;
export declare const EXCEPTION_FLT_INEXACT_RESULT: number | null;
export declare const EXCEPTION_FLT_INVALID_OPERATION: number | null;
export declare const EXCEPTION_FLT_OVERFLOW: number | null;
export declare const EXCEPTION_FLT_STACK_CHECK: number | null;
export declare const EXCEPTION_FLT_UNDERFLOW: number | null;
export declare const EXCEPTION_INT_DIVIDE_BY_ZERO: number | null;
export declare const EXCEPTION_INT_OVERFLOW: number | null;
export declare const EXCEPTION_PRIV_INSTRUCTION: number | null;
export declare const EXCEPTION_IN_PAGE_ERROR: number | null;
export declare const EXCEPTION_ILLEGAL_INSTRUCTION: number | null;
export declare const EXCEPTION_NONCONTINUABLE_EXCEPTION: number | null;
export declare const EXCEPTION_STACK_OVERFLOW: number | null;
export declare const EXCEPTION_INVALID_DISPOSITION: number | null;
export declare const EXCEPTION_GUARD_PAGE: number | null;
export declare const EXCEPTION_INVALID_HANDLE: number | null;
export declare const STATUS_STACK_BUFFER_OVERRUN: number | null;

// Unix/Linux signal constants
export declare const SIGABRT: number | null;
export declare const SIGFPE: number | null;
export declare const SIGSEGV: number | null;
export declare const SIGTERM: number | null;
export declare const SIGILL: number | null;
export declare const SIGINT: number | null;
export declare const SIGALRM: number | null;
export declare const SIGBUS: number | null;
export declare const SIGCHLD: number | null;
export declare const SIGCONT: number | null;
export declare const SIGHUP: number | null;
export declare const SIGKILL: number | null;
export declare const SIGPIPE: number | null;
export declare const SIGQUIT: number | null;
export declare const SIGSTOP: number | null;
export declare const SIGTSTP: number | null;
export declare const SIGTTIN: number | null;
export declare const SIGTTOU: number | null;
export declare const SIGUSR1: number | null;
export declare const SIGUSR2: number | null;
export declare const SIGPROF: number | null;
export declare const SIGSYS: number | null;
export declare const SIGTRAP: number | null;
export declare const SIGURG: number | null;
export declare const SIGVTALRM: number | null;
export declare const SIGXCPU: number | null;
export declare const SIGXFSZ: number | null;
export declare const SIGWINCH: number | null;

// Default export for CommonJS compatibility
declare const segfault: {
	causeSegfault: () => void;
	causeDivisionInt: () => void;
	causeOverflow: () => void;
	causeIllegal: () => void;
	setSignal: (signalId: number | null, value: boolean) => void;
	setOutputFormat: (jsonOutput: boolean) => void;
	getOutputFormat: () => boolean;

	// Windows exception constants
	EXCEPTION_ALL: number | null;
	EXCEPTION_ACCESS_VIOLATION: number | null;
	EXCEPTION_DATATYPE_MISALIGNMENT: number | null;
	EXCEPTION_BREAKPOINT: number | null;
	EXCEPTION_SINGLE_STEP: number | null;
	EXCEPTION_ARRAY_BOUNDS_EXCEEDED: number | null;
	EXCEPTION_FLT_DENORMAL_OPERAND: number | null;
	EXCEPTION_FLT_DIVIDE_BY_ZERO: number | null;
	EXCEPTION_FLT_INEXACT_RESULT: number | null;
	EXCEPTION_FLT_INVALID_OPERATION: number | null;
	EXCEPTION_FLT_OVERFLOW: number | null;
	EXCEPTION_FLT_STACK_CHECK: number | null;
	EXCEPTION_FLT_UNDERFLOW: number | null;
	EXCEPTION_INT_DIVIDE_BY_ZERO: number | null;
	EXCEPTION_INT_OVERFLOW: number | null;
	EXCEPTION_PRIV_INSTRUCTION: number | null;
	EXCEPTION_IN_PAGE_ERROR: number | null;
	EXCEPTION_ILLEGAL_INSTRUCTION: number | null;
	EXCEPTION_NONCONTINUABLE_EXCEPTION: number | null;
	EXCEPTION_STACK_OVERFLOW: number | null;
	EXCEPTION_INVALID_DISPOSITION: number | null;
	EXCEPTION_GUARD_PAGE: number | null;
	EXCEPTION_INVALID_HANDLE: number | null;
	STATUS_STACK_BUFFER_OVERRUN: number | null;

	// Unix/Linux signal constants
	SIGABRT: number | null;
	SIGFPE: number | null;
	SIGSEGV: number | null;
	SIGTERM: number | null;
	SIGILL: number | null;
	SIGINT: number | null;
	SIGALRM: number | null;
	SIGBUS: number | null;
	SIGCHLD: number | null;
	SIGCONT: number | null;
	SIGHUP: number | null;
	SIGKILL: number | null;
	SIGPIPE: number | null;
	SIGQUIT: number | null;
	SIGSTOP: number | null;
	SIGTSTP: number | null;
	SIGTTIN: number | null;
	SIGTTOU: number | null;
	SIGUSR1: number | null;
	SIGUSR2: number | null;
	SIGPROF: number | null;
	SIGSYS: number | null;
	SIGTRAP: number | null;
	SIGURG: number | null;
	SIGVTALRM: number | null;
	SIGXCPU: number | null;
	SIGXFSZ: number | null;
	SIGWINCH: number | null;
};

export default segfault;