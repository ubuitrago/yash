# and how to build it
# build all

all: yash yashd


# yash
yash: yash.c 
	gcc -g -o yash yash.c -lreadline

yashd: yashd.c 
	gcc -g -o yashd yashd.c -lreadline

# What to do if make is run as:
#   make clean
# remove all executables
clean:
	rm yash yashd
