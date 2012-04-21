#!/bin/bash

git submodule init
git submodule update
alias waf=`pwd`/third_party/waf/waf-light
waf configure
