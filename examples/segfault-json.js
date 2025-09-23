'use strict';

// JSON output example for segfault
const { causeSegfault, setOutputFormat } = require('..');

console.log('Enabling JSON output format...');
setOutputFormat(true);

console.log('Triggering segfault with JSON output...');
causeSegfault();