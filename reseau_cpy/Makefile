# See gcc/clang manual to understand all flags
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

# We have no libraries to link against except libc, but we want to keep
# the symbols for debugging
LDFLAGS= -rdynamic
LDFLAGS+= -lz

# Default target
all: clean sender receiver

# If we run `make debug` instead, keep the debug symbols for gdb
# and define the DEBUG macro.
debug: CFLAGS += -g -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer
debug: clean chat

# We use an implicit rule to build an executable named 'chat'
sender: sender.o connection_transfer/write_loop.o connection_transfer/create_socket.o connection_transfer/real_address.o connection_transfer/wait_for_client.o connection_transfer/write_loop.o packet/packet_implem.o

receiver: receiver.o connection_transfer/write_loop.o connection_transfer/create_socket.o connection_transfer/real_address.o connection_transfer/wait_for_client.o connection_transfer/write_loop.o packet/packet_implem.o
.PHONY: clean

clean:
	@rm -f receiver.o sender.o sender receiver connection_transfer/write_loop.o connection_transfer/create_socket.o connection_transfer/real_address.o connection_transfer/wait_for_client.o connection_transfer/write_loop.o packet/packet_implem.o 
