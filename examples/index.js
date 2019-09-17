'use strict';

// require('segfault-raub')
const { causeSegfault } = require('..');

// Simulate a buggy native module that dereferences NULL
causeSegfault();
