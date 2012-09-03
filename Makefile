TARGET?=Debug
TARGET_FLAGS?=

ifeq (${TARGET}, Debug)
TARGET_FLAGS+="--debug"
endif

all: rebuild test

build:
	./configure ${TARGET_FLAGS} --arch=x64 --shared-cunit-includes=$(HOME)/homebrew/include --shared-cunit-libpath=$(HOME)/homebrew/lib
	make -C ./out

rebuild: clean build

test:
	./out/${TARGET}/test

clean:
	-rm -rf out/Makefile
	-rm -rf out/**/lev


.PHONY: all test build rebuild clean
