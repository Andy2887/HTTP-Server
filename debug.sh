#!/bin/bash
set -e

# Build the project in Debug mode
./build.sh

# Launch LLDB with the server executable and pass any arguments
lldb ./build/server -- "$@"