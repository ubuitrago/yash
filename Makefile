# Compiler and flags
CC = gcc
CFLAGS = -I. -g
LIBS = -lpthread

# Source files
SERVER_SRCS = Server/yashd.c
CLIENT_SRCS = Client/yash.c

# Object files
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# Target Executables
SERVER = yashd
CLIENT = yash

# Default target: Build both server and client executables
all: $(SERVER) $(CLIENT)

# Build the server executable
$(SERVER): $(SERVER_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Build the client executable
$(CLIENT): $(CLIENT_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

# Compile source files into object files
%.o: %.c
	$(CC) -g -c -o $@ $< $(CFLAGS)

# Clean up generated files
.PHONY: clean
clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER) $(CLIENT)
