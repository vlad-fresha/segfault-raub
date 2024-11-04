'use strict';

const { ensuredir, write } = require('addon-tools-raub');
const { setLogPath, causeSegfault } = require('..');

(async () => {
	await ensuredir('c:/tmp');
	await write('c:/tmp/segfault.txt', '');
	
	setLogPath('c:/tmp/segfault.txt');
	causeSegfault();
})();

