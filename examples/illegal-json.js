'use strict';

// JSON output example for illegal instruction
const { causeIllegal, setOutputFormat } = require('..');

console.log('Enabling JSON output format...');
setOutputFormat(true);

console.log('Triggering illegal instruction with JSON output...');
causeIllegal();