# Segfault handler for Node.js

This is a part of [Node3D](https://github.com/node-3d) project.

[![NPM](https://nodei.co/npm/segfault-raub.png?compact=true)](https://www.npmjs.com/package/segfault-raub)

[![Build Status](https://api.travis-ci.com/node-3d/segfault-raub.svg?branch=master)](https://travis-ci.com/node-3d/segfault-raub)
[![CodeFactor](https://www.codefactor.io/repository/github/node-3d/segfault-raub/badge)](https://www.codefactor.io/repository/github/node-3d/segfault-raub)

> npm i webaudio-raub


## Synopsis

If a **SIGSEGV** signal is raised, the module will print a native stack trace to both
STDERR and to a timestamped file. This module does nothing (zero perf impact) as long
as Node is well-behaved.

> Note: this **addon uses N-API**, and therefore is ABI-compatible across different
Node.js versions. Addon binaries are precompiled and **there is no compilation**
step during the `npm i` command.


## Usage

Just require the module and that's it. You may require it as many times you want,
but the **SIGSEGV** hook will only be set once. There are no calls required, and
no options.

```javascript
require('segfault-raub');
```

There is one exported function though:

```javascript
const { causeSegfault } = require('segfault-raub');
causeSegfault();
```

In doing so, you will cause a segfault (accessing 0x01 pointer), and see how it goes.


## Legal notice

This is a fork of [segfault-handler](https://github.com/ddopson/node-segfault-handler).
The original licensing rules seem to apply, presumably (LICENSE is kept untouched).

This software is licensed for use under the BSD license.

Also this project uses [callstack walker](https://github.com/JochenKalmbach/StackWalker)
which is also BSD licensed.
