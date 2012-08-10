#!/bin/sh

git submodule init >kyotocabinet_configure.out 2>&1
git submodule update >>kyotocabinet_configure.out 2>&1
cd deps/kyotocabinet
make clean >>../../kyotocabinet_configure.out 2>&1
sh configure --enable-devel >>../../kyotocabinet_configure.out 2>&1
cd ../..
