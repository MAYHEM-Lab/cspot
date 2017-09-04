#!/bin/bash

ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)" < /dev/null 2> /dev/null
brew install czmq
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
