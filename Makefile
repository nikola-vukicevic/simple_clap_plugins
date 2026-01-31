CC      = gcc
CCFLAGS = -shared -O3 -Wall
FLDEBUG = -shared -O0 -g -Wall

TARGET     = ./build_linux/clap-gain-staging.clap
TARGET_DBG = ./build_linux/clap-gain-staging-debug.clap
SRCS       = ./src/*.c

install:
	$(CC) $(CCFLAGS) $(SRCS) -o $(TARGET)

debug:
	$(CC) $(FLDEBUG) $(SRCS) -o $(TARGET_DBG)

