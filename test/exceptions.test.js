'use strict';

const util = require('node:util');
const exec = util.promisify(require('node:child_process').exec);

const { getPlatform } = require('addon-tools-raub');

jest.setTimeout(30000);


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

describe('Exceptions', () => {
	it('reports segfaults', async () => {
		let response = await runAndGetError('causeSegfault');
		const exceptionName = getPlatform() === 'windows' ? 'ACCESS_VIOLATION' : 'SIGSEGV';
		expect(response).toContain(exceptionName);
	});
	
	// it('shows symbol names in stacktrace', async () => {
	// 	let response = await runAndGetError('causeSegfault');
	// 	expect(response).toEqual(expect.stringMatching(/segfault(\w|:)+causeSegfault/));
	// });
	
	// it('shows module names in stacktrace', async () => {
	// 	let response = await runAndGetError('causeSegfault');
	// 	expect(response).toContain('node');
	// 	expect(response).toContain('segfault.node');
	// });
	
	// These don't work properly on ARM
	if (['windows', 'linux'].includes(getPlatform())) {
		it('reports divisions by zero (int)', async () => {
			let response = await runAndGetError('causeDivisionInt');
			const exceptionName = getPlatform() === 'windows' ? 'INT_DIVIDE_BY_ZERO' : 'SIGFPE';
			expect(response).toContain(exceptionName);
		});
		
		it('reports stack overflows', async () => {
			let response = await runAndGetError('causeOverflow');
			const exceptionName = getPlatform() === 'windows' ? 'STACK_OVERFLOW' : 'SIGSEGV';
			expect(response).toContain(exceptionName);
		});
	}
	
	it('reports illegal operations', async () => {
		let response = await runAndGetError('causeIllegal');
		const exceptionName = getPlatform() === 'windows' ? 'ILLEGAL_INSTRUCTION' : 'SIGILL';
		expect(response).toContain(exceptionName);
	});
});
