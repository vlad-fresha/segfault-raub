'use strict';

const assert = require('node:assert').strict;
const { describe, it } = require('node:test');

// Inline platform detection
const getPlatform = () => {
	const platformAndArch = `${process.platform}-${process.arch}`;
	const platformNames = {
		'win32-x64': 'windows',
		'linux-x64': 'linux',
		'darwin-x64': 'osx',
		'linux-arm64': 'aarch64',
	};
	return platformNames[platformAndArch] || platformAndArch;
};
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
	'SIGXCPU', 'SIGXFSZ', 'SIGVTALRM', 'SIGPROF', 'SIGWINCH', 'SIGSYS',
];


describe('Segfault', () => {
	it('exports an object', () => {
		assert.strictEqual(typeof Segfault, 'object');
	});
	
	it('contains `causeSegfault` function', () => {
		assert.strictEqual(typeof Segfault.causeSegfault, 'function');
	});
	it('contains `causeDivisionInt` function', () => {
		assert.strictEqual(typeof Segfault.causeDivisionInt, 'function');
	});
	it('contains `causeOverflow` function', () => {
		assert.strictEqual(typeof Segfault.causeOverflow, 'function');
	});
	it('contains `causeIllegal` function', () => {
		assert.strictEqual(typeof Segfault.causeIllegal, 'function');
	});
	it('contains `setSignal` function', () => {
		assert.strictEqual(typeof Segfault.setSignal, 'function');
	});
	
	(getPlatform() === 'windows' ? signalsWindows : signalsUnix).forEach((name) => {
		it(`contains the \`${name}\` constant`, () => {
			assert.strictEqual(typeof Segfault[name], 'number');
		});
	});
});
