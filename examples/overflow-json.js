'use strict';

// JSON output example for stack overflow
const { causeOverflow, setOutputFormat } = require('..');

console.log('Enabling JSON output format...');
setOutputFormat(true);

console.log('Triggering stack overflow with JSON output...');
causeOverflow();