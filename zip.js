'use strict';

const util = require('node:util');
const exec = util.promisify(require('node:child_process').exec);

const { platform, bin } = require('addon-tools-raub');


(async () => {
	try {
		if (platform === 'windows') {
			await exec(`powershell Compress-Archive -Path ./${bin}/* -DestinationPath ./${bin}.zip`);
		} else {
			await exec(`cd ${bin} && tar -acf ../${bin}.zip *`);
		}
		
		console.log(`zip=${bin}.zip`);
	} catch (error) {
		console.error(error);
		process.exit(-1);
	}
})();
