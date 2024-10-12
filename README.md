# yash
Yash (Yet Another Shell) is a simple program executor that replicates basic functionality present in most shells. It can handle file redirects, pipes, and has job control. The shell can be run locally or within a client-server application.

## yashd
A daemonized version of yash withc client and server programs that mimic the sshd daemon and ssh client. 
The following diagram outlines how the server handles client connections in unique threads and logs return values. 
![yashd-server 2](https://github.com/user-attachments/assets/fb24d3bb-007a-4ae8-9a3c-67e5d0de9cc9)

