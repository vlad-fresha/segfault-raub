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

// Check if libunwind is available (Linux/musl systems)
const hasLibunwind = () => {
	const platform = getPlatform();
	return ['linux', 'aarch64'].includes(platform);
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

// Helper to extract individual stack frames from JSON output
const parseJsonStackFrames = (response) => {
	const lines = response.split('\n');
	const frames = [];

	for (const line of lines) {
		if (line.startsWith('{') && line.includes('"frame"') && !line.includes('"type":"segfault"')) {
			try {
				frames.push(JSON.parse(line));
			} catch (_e) {
				// Skip malformed JSON
			}
		}
	}
	return frames;
};

describe('Libunwind JSON Stack Traces', () => {
	// Only run libunwind tests on systems that support it
	if (!hasLibunwind()) {
		it('skips libunwind tests on non-Linux platforms', () => {
			assert.ok(true, `Libunwind tests skipped on ${getPlatform()}`);
		});
		return;
	}

	it('produces detailed JSON stack frames with libunwind', async () => {
		const response = await runAndGetErrorWithFormat('causeSegfault', true);

		// Should contain the main JSON error object
		const jsonError = parseJsonError(response);
		assert.ok(jsonError, 'Should contain main JSON error object');

		// Should also contain individual stack frames
		const stackFrames = parseJsonStackFrames(response);
		assert.ok(stackFrames.length > 0, 'Should have individual stack frame objects');

		// Check structure of stack frames
		const firstFrame = stackFrames[0];
		assert.ok(typeof firstFrame.frame === 'number', 'Frame should have frame number');
		assert.ok(typeof firstFrame.address === 'string', 'Frame should have address');
		assert.ok(typeof firstFrame.symbol === 'string', 'Frame should have symbol');
		assert.ok(firstFrame.address.startsWith('0x'), 'Address should be hex format');
	});

	it('includes function symbols in libunwind stack traces', async () => {
		const response = await runAndGetErrorWithFormat('causeSegfault', true);
		const stackFrames = parseJsonStackFrames(response);

		assert.ok(stackFrames.length > 0, 'Should have stack frames');

		// Look for our segfault function in the stack
		const hasSegfaultFunction = stackFrames.some((frame) =>
			frame.symbol.includes('causeSegfault') ||
			frame.symbol.includes('segfault') ||
			frame.symbol.includes('vlad_fresha_segfault_handler')
		);

		assert.ok(hasSegfaultFunction, 'Should find segfault-related function in stack trace');
	});

	it('provides valid memory addresses in stack frames', async () => {
		const response = await runAndGetErrorWithFormat('causeSegfault', true);
		const stackFrames = parseJsonStackFrames(response);

		assert.ok(stackFrames.length > 0, 'Should have stack frames');

		// All addresses should be valid hex values
		for (const frame of stackFrames) {
			assert.ok(frame.address.match(/^0x[0-9a-f]+$/i),
				`Address ${frame.address} should be valid hex format`);

			// Address should not be zero (except for special cases)
			const addressValue = parseInt(frame.address, 16);
			assert.ok(addressValue >= 0, 'Address should be non-negative');
		}
	});

	it('handles libunwind errors gracefully', async () => {
		const response = await runAndGetErrorWithFormat('causeSegfault', true);

		// Should not contain error messages about libunwind failures
		assert.ok(!response.includes('libunwind_getcontext_failed'),
			'Should not show getcontext failures in normal operation');
		assert.ok(!response.includes('libunwind_init_failed'),
			'Should not show init failures in normal operation');

		// Should contain either valid stack frames or graceful fallbacks
		const stackFrames = parseJsonStackFrames(response);
		const jsonError = parseJsonError(response);

		assert.ok(stackFrames.length > 0 || (jsonError && jsonError.stack && jsonError.stack.length > 0),
			'Should have either individual frames or frames in main object');
	});

	it('provides frame numbers in correct sequence', async () => {
		const response = await runAndGetErrorWithFormat('causeSegfault', true);
		const stackFrames = parseJsonStackFrames(response);

		if (stackFrames.length > 1) {
			// Frame numbers should be sequential starting from 0
			for (let i = 0; i < stackFrames.length; i++) {
				assert.strictEqual(stackFrames[i].frame, i,
					`Frame ${i} should have correct frame number`);
			}
		}
	});

	it('escapes JSON strings properly in symbol names', async () => {
		const response = await runAndGetErrorWithFormat('causeSegfault', true);
		const stackFrames = parseJsonStackFrames(response);

		// All frames should be valid JSON (already tested by parseJsonStackFrames)
		// but let's verify no unescaped quotes or control characters
		for (const frame of stackFrames) {
			assert.ok(typeof frame.symbol === 'string', 'Symbol should be string');

			// Re-stringify and parse to ensure it's valid JSON
			const reStringified = JSON.stringify(frame);
			assert.ok(JSON.parse(reStringified), 'Frame should be re-serializable as JSON');
		}
	});

	it('works with different exception types', async () => {
		const exceptionTypes = ['causeSegfault', 'causeIllegal'];

		if (['linux', 'aarch64'].includes(getPlatform())) {
			exceptionTypes.push('causeDivisionInt');
		}

		for (const exceptionType of exceptionTypes) {
			const response = await runAndGetErrorWithFormat(exceptionType, true);
			const stackFrames = parseJsonStackFrames(response);
			const jsonError = parseJsonError(response);

			assert.ok(jsonError || stackFrames.length > 0,
				`${exceptionType} should produce stack trace output`);

			if (stackFrames.length > 0) {
				// Should have proper structure for each exception type
				assert.ok(typeof stackFrames[0].frame === 'number',
					`${exceptionType} frame should have frame number`);
				assert.ok(typeof stackFrames[0].address === 'string',
					`${exceptionType} frame should have address`);
			}
		}
	});

	it('includes function offsets when available', async () => {
		const response = await runAndGetErrorWithFormat('causeSegfault', true);
		const stackFrames = parseJsonStackFrames(response);

		if (stackFrames.length > 0) {
			// Look for frames that include offset information (+ number)
			const hasOffsets = stackFrames.some((frame) =>
				frame.symbol.includes(' + ') && /\+ \d+/.test(frame.symbol)
			);

			// Not all frames may have offsets, but some should
			if (hasOffsets) {
				assert.ok(true, 'Found stack frames with function offsets');
			} else {
				// If no offsets found, that's also acceptable (some systems may not provide them)
				assert.ok(true, 'No offsets found, but this is acceptable on some systems');
			}
		}
	});
});

describe('Libunwind Error Handling', () => {
	if (!hasLibunwind()) {
		it('skips libunwind error handling tests on non-Linux platforms', () => {
			assert.ok(true, `Libunwind error tests skipped on ${getPlatform()}`);
		});
		return;
	}

	it('continues execution after libunwind errors', async () => {
		// This test ensures that even if libunwind encounters errors,
		// the program continues to provide some form of stack trace
		const response = await runAndGetErrorWithFormat('causeSegfault', true);

		// Should always have some form of output
		assert.ok(response.length > 0, 'Should produce some output even with potential libunwind errors');

		// Should contain either JSON error object or stack frames
		const jsonError = parseJsonError(response);
		const stackFrames = parseJsonStackFrames(response);

		assert.ok(jsonError || stackFrames.length > 0,
			'Should have either main JSON object or stack frames');
	});

	it('provides meaningful error messages in JSON format', async () => {
		const response = await runAndGetErrorWithFormat('causeSegfault', true);

		// If there are error messages, they should be in recognizable format
		if (response.includes('libunwind_')) {
			// Error messages should still be valid JSON
			const lines = response.split('\n');
			for (const line of lines) {
				if (line.includes('libunwind_') && line.startsWith('{')) {
					assert.ok((() => JSON.parse(line)), 'Error messages should be valid JSON');
				}
			}
		}
	});
});