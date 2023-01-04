'use strict';

const util = require('node:util');
const exec = util.promisify(require('node:child_process').exec);

const { getPlatform } = require('addon-tools-raub');


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
	
	// On Unix, the stacktrace is empty sometimes
	if (['windows'].includes(getPlatform())) {
		it('shows symbol names in stacktrace', async () => {
			let response = await runAndGetError('causeSegfault');
			expect(response).toEqual(expect.stringMatching(/segfault::causeSegfault/));
		});
		
		it('shows module names in stacktrace', async () => {
			let response = await runAndGetError('causeSegfault');
			expect(response).toContain('[node.exe]');
			expect(response).toContain('[segfault.node]');
		});
	}
	
	// On ARM this fails
	if (['windows', 'linux'].includes(getPlatform())) {
		it('reports divisions by zero (int)', async () => {
			let response = await runAndGetError('causeDivisionInt');
			const exceptionName = getPlatform() === 'windows' ? 'INT_DIVIDE_BY_ZERO' : 'SIGFPE';
			expect(response).toContain(exceptionName);
		});
	}
	
	// On Unix, this hangs for some reason
	if (['windows'].includes(getPlatform())) {
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
