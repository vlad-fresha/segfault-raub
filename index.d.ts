declare module "segfault-raub" {
	/**
	 * Produce a segfault.
	 * 
	 * Issue an actual segfault, accessing some unavailable memory.
	 */
	export const causeSegfault: () => void;
	
	/**
	 * Divides an integer number by zero.
	 */
	export const causeDivisionInt: () => void;
	
	/**
	 * Overflows the program stack.
	 */
	export const causeOverflow: () => void;
	
	/**
	 * Executes an illegal instruction.
	 */
	export const causeIllegal: () => void;
	
	/**
	 * Enable / disable signal handlers.
	 */
	export const setSignal: (signalId: number | null, value: boolean) => void;
	
	/**
	 * Change the path of "segfault.log" location.
	 * 
	 * E.g. you can set it to `"C:/my.log"` or whatever.
	 * But you still have to create the file separately,
	 * the addon won't do that automatically.
	 * 
	 * If `null` or `""` path passed - resets to "segfault.log".
	 */
	export const setLogPath: (path: string | null) => void;

	// Windows OS constants:
	export const EXCEPTION_ALL: number | null;
	export const EXCEPTION_ACCESS_VIOLATION: number | null;
	export const EXCEPTION_DATATYPE_MISALIGNMENT: number | null;
	export const EXCEPTION_BREAKPOINT: number | null;
	export const EXCEPTION_SINGLE_STEP: number | null;
	export const EXCEPTION_ARRAY_BOUNDS_EXCEEDED: number | null;
	export const EXCEPTION_FLT_DENORMAL_OPERAND: number | null;
	export const EXCEPTION_FLT_DIVIDE_BY_ZERO: number | null;
	export const EXCEPTION_FLT_INEXACT_RESULT: number | null;
	export const EXCEPTION_FLT_INVALID_OPERATION: number | null;
	export const EXCEPTION_FLT_OVERFLOW: number | null;
	export const EXCEPTION_FLT_STACK_CHECK: number | null;
	export const EXCEPTION_FLT_UNDERFLOW: number | null;
	export const EXCEPTION_INT_DIVIDE_BY_ZERO: number | null;
	export const EXCEPTION_INT_OVERFLOW: number | null;
	export const EXCEPTION_PRIV_INSTRUCTION: number | null;
	export const EXCEPTION_IN_PAGE_ERROR: number | null;
	export const EXCEPTION_ILLEGAL_INSTRUCTION: number | null;
	export const EXCEPTION_NONCONTINUABLE_EXCEPTION: number | null;
	export const EXCEPTION_STACK_OVERFLOW: number | null;
	export const EXCEPTION_INVALID_DISPOSITION: number | null;
	export const EXCEPTION_GUARD_PAGE: number | null;
	export const EXCEPTION_INVALID_HANDLE: number | null;
	export const STATUS_STACK_BUFFER_OVERRUN: number | null;

	// Linux/Unix OS constants
	export const SIGABRT: number | null;
	export const SIGFPE: number | null;
	export const SIGSEGV: number | null;
	export const SIGTERM: number | null;
	export const SIGILL: number | null;
	export const SIGINT: number | null;
	export const SIGALRM: number | null;
	export const SIGBUS: number | null;
	export const SIGCHLD: number | null;
	export const SIGCONT: number | null;
	export const SIGHUP: number | null;
	export const SIGKILL: number | null;
	export const SIGPIPE: number | null;
	export const SIGQUIT: number | null;
	export const SIGSTOP: number | null;
	export const SIGTSTP: number | null;
	export const SIGTTIN: number | null;
	export const SIGTTOU: number | null;
	export const SIGUSR1: number | null;
	export const SIGUSR2: number | null;
	export const SIGPROF: number | null;
	export const SIGSYS: number | null;
	export const SIGTRAP: number | null;
	export const SIGURG: number | null;
	export const SIGVTALRM: number | null;
	export const SIGXCPU: number | null;
	export const SIGXFSZ: number | null;
}
