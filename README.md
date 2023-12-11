# mtime
A simple (**but still WIP**) time measurment utility.

## Features
- a tool to measure the runtime and resource usage of a command
- a static library containing the profiling functions (currently **only** `profile_cmd()`)

## Build and Installation
The tool and the static library can be built and installed using the following command: `make && make DESTDIR=<dir> install`
