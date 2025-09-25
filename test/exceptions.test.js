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
