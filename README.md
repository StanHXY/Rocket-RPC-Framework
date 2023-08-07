# Rocket RPC Framework #


## Overview ##
Rocket is an asynchronous RPC framework developed using C++11. It aims to be efficient and concise while maintaining top-notch performance.

Rocket is built on the main-sub Reactor architecture, with the underlying layer using epoll for IO multiplexing. The application layer is based on a custom RPC communication protocol using protobuf. It also aims to support basic HTTP protocols.


### 1 Key Technologies ###
C++11
Protobuf
RPC
Reactor
HTTP


### 2 Environment ###
Development Environment: Linux, setup is on Centos8 virtual machine with GCC/G++ (supporting at least C++11 syntax).

Development Tool: VsCode, connecting remotely to the Linux machine via SSH.


### 3 Logging Module Features ###


LogModule:
```
Print to file with date-based naming and log rotation.
C-style formatting
Thread safety
```

LogLevel:
```
Debug
Info
Error
```

LogEvent:
```
Filename and line number
Message number
Process ID
Thread ID
Date and time (accurate to ms)
Custom messages
```

Log Format:
```perl
[Level][%y-%m-%d %H:%M:%s.%ms]\t[pid:thread_id]\t[file_name:line][%msg]

Logger:

1. Provides methods for logging.
2. Sets the path for log output.

###