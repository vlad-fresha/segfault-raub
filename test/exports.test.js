'use strict';

const { platform } = require('addon-tools-raub');
const Segfault = require('..');


const signalsWindows = [
	'EXCEPTION_ACCESS_VIOLATION', 'EXCEPTION_DATATYPE_MISALIGNMENT',
	'EXCEPTION_BREAKPOINT', 'EXCEPTION_SINGLE_STEP', 'EXCEPTION_ARRAY_BOUNDS_EXCEEDED',
	'EXCEPTION_FLT_DENORMAL_OPERAND', 'EXCEPTION_FLT_DIVIDE_BY_ZERO',
	'EXCEPTION_FLT_INEXACT_RESULT', 'EXCEPTION_FLT_INVALID_OPERATION',
	'EXCEPTION_FLT_OVERFLOW', 'EXCEPTION_FLT_STACK_CHECK', 'EXCEPTION_FLT_UNDERFLOW',
	'EXCEPTION_INT_DIVIDE_BY_ZERO', 'EXCEPTION_INT_OVERFLOW',
	'EXCEPTION_PRIV_INSTRUCTION', 'EXCEPTION_IN_PAGE_ERROR',
	'EXCEPTION_ILLEGAL_INSTRUCTION', 'EXCEPTION_NONCONTINUABLE_EXCEPTION',
	'EXCEPTION_STACK_OVERFLOW', 'EXCEPTION_INVALID_DISPOSITION',
	'EXCEPTION_GUARD_PAGE', 'EXCEPTION_INVALID_HANDLE',
];

const signalsUnix = [
	'SIGINT', 'SIGILL', 'SIGABRT', 'SIGFPE', 'SIGSEGV', 'SIGTERM', 'SIGHUP', 'SIGQUIT',
	'SIGTRAP', 'SIGBUS', 'SIGKILL', 'SIGUSR1', 'SIGUSR2', 'SIGPIPE', 'SIGALRM',
	'SIGCHLD', 'SIGCONT', 'SIGSTOP', 'SIGTSTP', 'SIGTTIN', 'SIGTTOU', 'SIGURG',
	'SIGXCPU', 'SIGXFSZ', 'SIGVTALRM', 'SIGPROF', 'SIGWINCH', 'SIGIO', 'SIGSYS',
];


describe('Segfault', () => {
	it('exports an object', () => {
		expect(typeof Segfault).toBe('object');
	});
	
	it('contains `causeSegfault` function', () => {
		expect(typeof Segfault.causeSegfault).toBe('function');
	});
	it('contains `causeDivisionInt` function', () => {
		expect(typeof Segfault.causeDivisionInt).toBe('function');
	});
	it('contains `causeOverflow` function', () => {
		expect(typeof Segfault.causeOverflow).toBe('function');
	});
	it('contains `causeIllegal` function', () => {
		expect(typeof Segfault.causeIllegal).toBe('function');
	});
	it('contains `setSignal` function', () => {
		expect(typeof Segfault.setSignal).toBe('function');
	});
	
	(platform === 'windows' ? signalsWindows : signalsUnix).forEach((name) => {
		it(`contains the \`${name}\` constant`, () => {
			expect(typeof Segfault[name]).toBe('number');
		});
	});
});
