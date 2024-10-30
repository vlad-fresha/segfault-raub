'use strict';

const assert = require('node:assert').strict;
const { describe, it, after } = require('node:test');
const util = require('node:util');
const exec = util.promisify(require('node:child_process').exec);

const { rm, exists, write, read } = require('addon-tools-raub');

const PATH_STD = `${__dirname.replaceAll('\\', '/')}/../segfault.log`;
const PATH_CUSTOM = `${__dirname.replaceAll('\\', '/')}/custom.log`;


const causeSegfault = async (isCustom = false) => {
	try {
		if (isCustom) {
			await exec([
				`node -e "require('.').setLogPath('${PATH_CUSTOM}')`,
				'require(\'.\').causeSegfault()"'
			].join('||'));
			return;
		}
		await exec('node -e "require(\'.\').causeSegfault()"');
	} catch (_e) {
		// do nothing
	}
};


describe('Logging', async () => {
	await rm(PATH_STD);
	await rm(PATH_CUSTOM);
	
	after(async () => {
		await rm(PATH_STD);
		await rm(PATH_CUSTOM);
	});
	
	it('does not write log if there is no file', async () => {
		await causeSegfault();
		
		const isLogWritten = await exists(PATH_STD);
		assert.ok(!isLogWritten);
	});
	
	it('writes standard log if it exists', async () => {
		await write(PATH_STD, 'test\n');
		await causeSegfault();
		
		const content = await read(PATH_STD);
		assert.ok(content.length > 10);
	});
	
	it('does not write custom log if there is no file', async () => {
		await rm(PATH_STD);
		await causeSegfault(true);
		
		const isLogWritten = await exists(PATH_STD);
		const isLogWrittenCustom = await exists(PATH_CUSTOM);
		assert.ok(!isLogWritten);
		assert.ok(!isLogWrittenCustom);
	});
	
	it('writes custom log if it exists', async () => {
		await rm(PATH_STD);
		await write(PATH_CUSTOM, 'test\n');
		await causeSegfault(true);
		
		const isLogWritten = await exists(PATH_STD);
		assert.ok(!isLogWritten);
		
		const content = await read(PATH_CUSTOM);
		assert.ok(content.length > 10);
	});
});
