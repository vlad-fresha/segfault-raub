'use strict';

const util = require('node:util');
const exec = util.promisify(require('node:child_process').exec);

const { platform } = require('addon-tools-raub');


const runAndGetError = async (name) => {
	let response = '';
	try {
		const { stderr, stdout } = await exec(`node -e "require('.').${name}()"`);
		response = stderr + stdout;
	} catch (error) {
		response = error.message;
	}
	console.log('\n\n\n\n', response, '\n\n\n\n');
	return response;
};

describe('Exceptions', () => {
	it('reports segfaults', async () => {
		let response = await runAndGetError('causeSegfault');
		const exceptionName = platform === 'windows' ? 'ACCESS_VIOLATION' : 'SIGSEGV';
		expect(response).toContain(exceptionName);
	});
	
	// These don't work properly on ARM
	if (platform === 'windows') {
		it('shows module names in Windows stacktrace', async () => {
			let response = await runAndGetError('causeSegfault');
			
			expect(response).toContain('[node.exe]');
			expect(response).toContain('[segfault.node]');
		});
		it('shows symbol names in Windows stacktrace', async () => {
			let response = await runAndGetError('causeSegfault');
			expect(response).toContain('segfault::causeSegfault');
		});
	}
	
	// These don't work properly on ARM
	if (['windows', 'linux'].includes(platform)) {
		it('reports divisions by zero (int)', async () => {
			let response = await runAndGetError('causeDivisionInt');
			const exceptionName = platform === 'windows' ? 'INT_DIVIDE_BY_ZERO' : 'SIGFPE';
			expect(response).toContain(exceptionName);
		});
		
		it('reports stack overflows', async () => {
			let response = await runAndGetError('causeOverflow');
			const exceptionName = platform === 'windows' ? 'STACK_OVERFLOW' : 'SIGSEGV';
			expect(response).toContain(exceptionName);
		});
	}
	
	it('reports illegal operations', async () => {
		let response = await runAndGetError('causeIllegal');
		const exceptionName = platform === 'windows' ? 'ILLEGAL_INSTRUCTION' : 'SIGILL';
		expect(response).toContain(exceptionName);
	});
});
