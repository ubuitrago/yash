# yash
Yash (Yet Another Shell) is a simple program executor that replicates basic functionality present in most shells. It can handle file redirects, pipes, and has job control. The shell can be run locally or within a client-server application.

## Usage
To run locally:

Start the yashd server in **terminal_1:**
`
./yashd
`

Start the yash client in **terminal_2:**
`
./yash 127.0.1.1
`

The server accepts client connections on port 3820, so the client only needs to provide the IPV4 or hostname on which the server is running. 

### Daemon (yashd)
A server program that mimics the sshd daemon and ssh client. 
The following diagram outlines how the server handles client connections in unique threads and logs return values. 
![yashd-server 2](https://github.com/user-attachments/assets/fb24d3bb-007a-4ae8-9a3c-67e5d0de9cc9)

## yash Client

This is a place holder for the client section
