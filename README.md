# myFTP

Simple asynchronous FTP server with browse/download functions written using boost::asio
## Features
* FTP passive mode 
* Filesystem browsing commands (pwd, cd, ls, rm,)
* Downloading/uploading files in binary stream mode
* Usability tested with netkit-ftp client
## Requirements
- C++17
- Boost asio
- cmake
- Ninja
## Build
1. cmake -G Ninja .
2. ninja
## Usage
* ./myFTP "port number" "number of threads"
