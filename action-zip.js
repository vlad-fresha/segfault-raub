'use strict';

const util = require('node:util');
const exec = util.promisify(require('node:child_process').exec);

const { bin } = require('addon-tools-raub');


(async () => {
	try {
		await exec(`cd ${bin} && tar -acf ../${bin}.zip *`);
		console.log(`zip=${bin}.zip`);
	} catch (error) {
		console.error(error);
		process.exit(-1);
	}
})();
