'use strict';

/* eslint-env node */
/* global setTimeout */

// Example demonstrating signal configuration with JSON output
const {
	setSignal, setOutputFormat,
	SIGSEGV, SIGILL, SIGTRAP,
	EXCEPTION_ACCESS_VIOLATION, EXCEPTION_ILLEGAL_INSTRUCTION,
	EXCEPTION_BREAKPOINT,
	causeIllegal
} = require('..');

console.log('=== Signal Configuration with JSON Output Example ===\n');

// Enable JSON output for structured error reporting
console.log('Enabling JSON output format...');
setOutputFormat(true);

// Configure signals based on platform
console.log('Configuring signals...');

// Disable some default signals
setSignal(SIGSEGV, false);                    // Unix
setSignal(EXCEPTION_ACCESS_VIOLATION, false); // Windows

// Enable debugging/tracing signals
setSignal(SIGTRAP, true);                     // Unix
setSignal(EXCEPTION_BREAKPOINT, true);        // Windows

setSignal(SIGILL, true);                      // Unix
setSignal(EXCEPTION_ILLEGAL_INSTRUCTION, true); // Windows

console.log('Signal configuration complete. Available signals:');
console.log('SIGSEGV:', SIGSEGV);
console.log('SIGILL:', SIGILL);
console.log('SIGTRAP:', SIGTRAP);
console.log('EXCEPTION_ACCESS_VIOLATION:', EXCEPTION_ACCESS_VIOLATION);
console.log('EXCEPTION_ILLEGAL_INSTRUCTION:', EXCEPTION_ILLEGAL_INSTRUCTION);
console.log('EXCEPTION_BREAKPOINT:', EXCEPTION_BREAKPOINT);

console.log('\nTriggering illegal instruction (should produce JSON output)...');
setTimeout(() => {
	causeIllegal();
}, 2000);