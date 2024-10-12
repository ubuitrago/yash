CC   = gcc
CFLAGS = -I.
LIBS = -lpthread
DEPS = serveTCPsocks.h
SRCS = yashd.c serveTCPsocks.c
OBJS = $(SRCS:.c=.o)
TARGET = yashd

# Build the target executable
$(TARGET): $(OBJS) $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

# Compile source files into object files
%.o: %.c $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

client: tcpClient.c
	gcc -g -o client tcpClient.c
# Clean up generated files
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) client tcpClient.o
