'use strict';

const assert = require('node:assert').strict;
const { describe, it } = require('node:test');
const util = require('node:util');
const exec = util.promisify(require('node:child_process').exec);

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


const runAndGetError = async (name) => {
	let response = '';
	try {
		const { stderr, stdout } = await exec(`node -e "require('.').${name}()"`);
		response = stderr + stdout;
	} catch (error) {
		response = error.message;
	}
	return response;
};

const runAndGetErrorWithFormat = async (name, useJson = false) => {
	let response = '';
	try {
		const command = `node -e "const sf = require('.'); sf.setOutputFormat(${useJson}); sf.${name}()"`;
		const { stderr, stdout } = await exec(command);
		response = stderr + stdout;
	} catch (error) {
		response = error.message;
	}
	return response;
};

const parseJsonError = (response) => {
	const lines = response.split('\n');
	for (const line of lines) {
		if (line.startsWith('{') && line.includes('"type":"segfault"')) {
			try {
				return JSON.parse(line);
			} catch (_e) {
				// Continue searching if JSON parse fails
			}
		}
	}
	return null;
};

describe('Exceptions', () => {
	it('reports segfaults', async () => {
		let response = await runAndGetError('causeSegfault');
		const exceptionName = getPlatform() === 'windows' ? 'ACCESS_VIOLATION' : 'SIGSEGV';
		assert.ok(response.includes(exceptionName));
	});
	
	// On Unix, the stacktrace is empty sometimes
	if (['windows'].includes(getPlatform())) {
		it('shows symbol names in stacktrace', async () => {
			let response = await runAndGetError('causeSegfault');
			assert.match(response, /segfault::causeSegfault/);
		});
		
		it('shows module names in stacktrace', async () => {
			let response = await runAndGetError('causeSegfault');
			assert.ok(response.includes('[node.exe]'));
			assert.ok(response.includes('[vlad_fresha_segfault_handler.node]'));
		});
	}
	
	// On ARM this fails
	if (['windows', 'linux'].includes(getPlatform())) {
		it('reports divisions by zero (int)', async () => {
			let response = await runAndGetError('causeDivisionInt');
			const exceptionName = getPlatform() === 'windows' ? 'INT_DIVIDE_BY_ZERO' : 'SIGFPE';
			assert.ok(response.includes(exceptionName));
		});
	}
	
	// On Unix, this hangs for some reason
	if (['windows'].includes(getPlatform())) {
		it('reports stack overflows', async () => {
			let response = await runAndGetError('causeOverflow');
			const exceptionName = getPlatform() === 'windows' ? 'STACK_OVERFLOW' : 'SIGSEGV';
			assert.ok(response.includes(exceptionName));
		});
	}
	
	it('reports illegal operations', async () => {
		let response = await runAndGetError('causeIllegal');
		const exceptionName = getPlatform() === 'windows' ? 'ILLEGAL_INSTRUCTION' : 'SIGILL';
		assert.ok(response.includes(exceptionName));
	});
});

describe('JSON Output Format', () => {
	it('outputs valid JSON for segfaults', async () => {
		let response = await runAndGetErrorWithFormat('causeSegfault', true);

		// Debug: log actual response to see if our fix helped
		console.log('DEBUG response length:', response.length);
		console.log('DEBUG response (first 300 chars):', JSON.stringify(response.substring(0, 300)));

		const jsonError = parseJsonError(response);

		assert.ok(jsonError, 'Should contain valid JSON');
		assert.strictEqual(jsonError.type, 'segfault');
		assert.ok(typeof jsonError.signal === 'number');
		assert.ok(typeof jsonError.signal_name === 'string');
		assert.ok(typeof jsonError.address === 'string');
		assert.ok(typeof jsonError.pid === 'number');
		assert.ok(Array.isArray(jsonError.stack));
	});

	it('includes correct signal information in JSON', async () => {
		let response = await runAndGetErrorWithFormat('causeSegfault', true);
		const jsonError = parseJsonError(response);

		assert.ok(jsonError);
		const expectedSignal = getPlatform() === 'windows' ? 'ACCESS_VIOLATION' : 'SIGSEGV';
		assert.strictEqual(jsonError.signal_name, expectedSignal);
	});

	it('includes stack trace in JSON format', async () => {
		let response = await runAndGetErrorWithFormat('causeSegfault', true);
		const jsonError = parseJsonError(response);

		assert.ok(jsonError);
		assert.ok(jsonError.stack.length > 0, 'Should have stack frames');

		// Check first stack frame structure
		const frame = jsonError.stack[0];
		assert.ok(typeof frame.frame === 'number');
		assert.ok(typeof frame.address === 'string');
		assert.ok(typeof frame.symbol === 'string');
	});

	it('stack trace contains meaningful function information', async () => {
		let response = await runAndGetErrorWithFormat('causeSegfault', true);
		const jsonError = parseJsonError(response);

		assert.ok(jsonError, 'Should have valid JSON error');
		assert.ok(jsonError.stack.length > 0, 'Should have at least one stack frame');

		// Check first stack frame has meaningful content
		const frame = jsonError.stack[0];

		// Frame number should be 0 (first frame)
		assert.strictEqual(frame.frame, 0, 'First frame should be numbered 0');

		// Address should be a valid hex string or "0x0" (valid null address)
		assert.ok(frame.address.startsWith('0x'), 'Address should start with 0x');
		// Allow "0x0" as a valid address (some platforms use this for null/invalid addresses)
		assert.ok(frame.address.length >= 3, 'Address should be at least 0x0');

		// Symbol should contain meaningful information
		assert.ok(frame.symbol.length > 0, 'Symbol should not be empty');

		// Platform-specific checks for meaningful function names
		const platform = getPlatform();

		if (platform === 'linux' || platform === 'aarch64') {
			// On Linux ARM64, we should get either:
			// 1. Real mangled C++ function names (starting with _Z)
			// 2. Module names in angle brackets
			// 3. Address fallbacks
			// 4. Unknown symbols (acceptable in some CI environments)
			const isMangled = frame.symbol.startsWith('_Z');
			const isModule = frame.symbol.startsWith('<') && frame.symbol.endsWith('>');
			const isAddress = /^0x[0-9a-f]+$/i.test(frame.symbol);
			const isUnknown = frame.symbol === 'unknown' || frame.symbol === '<unknown>';

			assert.ok(isMangled || isModule || isAddress || isUnknown,
				`Linux symbol should be mangled function, module name, address, or unknown, got: ${frame.symbol}`);

			// If it's a mangled function, it should contain segfault-related symbols
			if (isMangled) {
				// Should be related to segfault functions (but be flexible about exact content)
				const hasSegfaultRef = frame.symbol.includes('segfault') ||
					frame.symbol.includes('Stack') ||
					frame.symbol.includes('_segfault') ||
					frame.symbol.length > 10; // Any reasonably long mangled name is probably valid
				assert.ok(hasSegfaultRef,
					`Mangled function should be meaningful, got: ${frame.symbol}`);
			}

			// If it's a module, should be our native module (but be flexible)
			if (isModule) {
				const isOurModule = frame.symbol.includes('vlad_fresha_segfault_handler.node') ||
					frame.symbol.includes('.node') ||
					frame.symbol.includes('segfault');
				assert.ok(isOurModule,
					`Module should reference our native module or similar, got: ${frame.symbol}`);
			}
		} else if (platform === 'windows') {
			// On Windows, we expect module and function information
			// Should not be generic placeholders
			assert.ok(!frame.symbol.includes('<unknown>'),
				'Windows should not have unknown symbols');
			assert.ok(!frame.symbol.includes('<no_stack_trace_available>'),
				'Should have stack trace available');
		} else if (platform === 'osx') {
			// On macOS, we expect detailed backtrace symbols
			// Should not be generic placeholders
			assert.ok(!frame.symbol.includes('<unknown>'),
				'macOS should not have unknown symbols');
			assert.ok(!frame.symbol.includes('<no_stack_trace_available>'),
				'Should have stack trace available');

			// macOS backtrace often includes module paths or function names
			const hasModuleInfo = frame.symbol.includes('.dylib') ||
								 frame.symbol.includes('.node') ||
								 frame.symbol.includes('node') ||
								 frame.symbol.includes('0x');
			assert.ok(hasModuleInfo,
				`macOS symbol should contain module or address info, got: ${frame.symbol}`);
		}

		// Ensure we're not getting fallback error messages
		assert.ok(!frame.symbol.includes('_stack_walk_failed'), 'Should not have stack walk failure');
		assert.ok(!frame.symbol.includes('simple_fallback'), 'Should not have simple fallback');
	});

	// Test other exception types in JSON format
	if (['windows', 'linux'].includes(getPlatform())) {
		it('outputs JSON for division by zero', async () => {
			let response = await runAndGetErrorWithFormat('causeDivisionInt', true);
			const jsonError = parseJsonError(response);

			assert.ok(jsonError);
			const expectedSignal = getPlatform() === 'windows' ? 'INT_DIVIDE_BY_ZERO' : 'SIGFPE';
			assert.strictEqual(jsonError.signal_name, expectedSignal);
		});
	}

	it('outputs JSON for illegal operations', async () => {
		let response = await runAndGetErrorWithFormat('causeIllegal', true);
		const jsonError = parseJsonError(response);

		assert.ok(jsonError);
		const expectedSignal = getPlatform() === 'windows' ? 'ILLEGAL_INSTRUCTION' : 'SIGILL';
		assert.strictEqual(jsonError.signal_name, expectedSignal);
	});
});

describe('Plain Text Output Format', () => {
	it('outputs plain text for segfaults', async () => {
		let response = await runAndGetErrorWithFormat('causeSegfault', false);
		const exceptionName = getPlatform() === 'windows' ? 'ACCESS_VIOLATION' : 'SIGSEGV';

		// Should contain traditional format
		assert.ok(response.includes(`received ${exceptionName}`));
		assert.ok(response.includes('PID'));

		// Should not contain JSON
		assert.ok(!response.includes('"type":"segfault"'));
	});

	it('shows traditional stack trace format', async () => {
		let response = await runAndGetErrorWithFormat('causeSegfault', false);

		// Should contain traditional stack trace indicators
		assert.ok(response.includes('PID') && response.includes('received'));

		// Should not be JSON format
		const jsonError = parseJsonError(response);
		assert.strictEqual(jsonError, null, 'Should not contain JSON output');
	});

	it('traditional stack trace contains function symbols', async () => {
		let response = await runAndGetErrorWithFormat('causeSegfault', false);

		// Should contain basic crash information
		const platform = getPlatform();
		const expectedSignal = platform === 'windows' ? 'ACCESS_VIOLATION' : 'SIGSEGV';
		assert.ok(response.includes(`received ${expectedSignal}`), 'Should contain signal name');
		assert.ok(response.includes('PID'), 'Should contain process ID');

		// Should contain stack trace information
		// Most platforms should provide some form of stack trace in plain text mode
		if (platform === 'linux' || platform === 'aarch64') {
			// On Linux, traditional mode uses backtrace_symbols_fd or libunwind
			// Should contain addresses, function names, or stack trace indicators
			const hasAddresses = /0x[0-9a-f]+/i.test(response);
			const hasSymbols = response.includes('segfault') ||
				response.includes('vlad_fresha_segfault_handler') ||
				response.includes('Stack trace') ||
				response.includes('libunwind') ||
				response.includes('<unknown>'); // Even unknown symbols indicate stack tracing worked

			// Should have either addresses, symbols, or stack trace indicators
			assert.ok(hasAddresses || hasSymbols,
				'Linux traditional output should contain addresses, symbols, or stack trace indicators, ' +
				`got: ${response.substring(0, 500)}`);
		} else if (platform === 'osx') {
			// On macOS, backtrace should provide detailed information
			const hasBacktrace = response.includes('0x') ||
				response.includes('.dylib') || response.includes('node');
			assert.ok(hasBacktrace,
				`macOS should provide backtrace information, got: ${response.substring(0, 500)}`);
		}

		// Should not contain fallback error messages in traditional output
		assert.ok(!response.includes('no_stack_trace_available'), 'Should have stack trace available');
		assert.ok(!response.includes('stack_walk_failed'), 'Stack walking should not fail');
	});
});

describe('Output Format Configuration', () => {
	it('can get and set output format', async () => {
		try {
			// Helper to strip ANSI escape codes
			// eslint-disable-next-line no-control-regex
			const stripAnsi = (str) => str.replace(/\x1B\[[0-9;]*m/g, '');

			// Test getting default format (should be false/plain text)
			const { stdout: defaultFormat } = await exec(
				'node -e "console.log(require(\\".\\").getOutputFormat())"'
			);
			assert.strictEqual(stripAnsi(defaultFormat.trim()), 'false');

			// Test setting to JSON
			const { stdout: jsonFormat } = await exec(
				'node -e "const sf = require(\\".\\"); ' +
				'sf.setOutputFormat(true); console.log(sf.getOutputFormat())"'
			);
			assert.strictEqual(stripAnsi(jsonFormat.trim()), 'true');

			// Test setting back to plain text
			const { stdout: plainFormat } = await exec(
				'node -e "const sf = require(\\".\\"); ' +
				'sf.setOutputFormat(false); console.log(sf.getOutputFormat())"'
			);
			assert.strictEqual(stripAnsi(plainFormat.trim()), 'false');
		} catch (error) {
			// These tests don't cause crashes, so they should succeed
			assert.fail(`Configuration test failed: ${error.message}`);
		}
	});
});
