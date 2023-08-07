# Rocket - RPC Framework #


## Overview ##
Rocket is an asynchronous RPC framework developed using C++11. It aims to be efficient and concise while maintaining top-notch performance.
Rocket is built on the main-sub Reactor architecture, with the underlying layer using epoll for IO multiplexing. The application layer is based on a custom RPC communication protocol using protobuf. It also aims to support basic HTTP protocols.

 å

### 1. Key Technologies ###
C++11
Protobuf
RPC
Reactor
HTTP

 

### 2. Environment ###
Development Environment: Linux, setup is on Centos8 virtual machine with GCC/G++ (supporting at least C++11 syntax).

Development Tool: VsCode, connecting remotely to the Linux machine via SSH.

 

### 3. Logging Module Features ###

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
```

Logger:

1. Provides methods for logging.
2. Sets the path for log output.

 

### 4. Reactor ###

The Reactor, also known as EventLoop, is essentially an event loop model.
The core logic of Reactor (or EventLoop) is a loop, described in pseudocode as follows:

```cpp
void loop() {
  while(!stop) {
      foreach (task in tasks) {
        task();
      }

      // 1. Get the time of the next scheduled task and compare it with the set timeout. 
      // If the next scheduled task time exceeds 1s, take the next scheduled task time as the timeout, otherwise, take 1s.
      int time_out = Max(1000, getNextTimerCallback());
      
      // 2. Call Epoll to wait for an event to occur, with the timeout set to the above time_out.
      int rt = epoll_wait(epfd, fds, ...., time_out); 
      if(rt < 0) {
          // epoll call failed...
      } else {
          if (rt > 0 ) {
            foreach (fd in fds) {
              // Add tasks to be executed to the execution queue.
              tasks.push(fd);
            }
          }
      }
  }
}
```

In rocket, the Master-Slave Reactor model is used, as shown in the diagram below:
[image](/img/reactors.png)

mainReactor:

Operated by the main thread.
Its primary function is to monitor the readable events of listenfd using epoll.
When a readable event occurs, it calls the accept function to obtain the clientfd.
It then randomly selects a subReactor and registers the read and write events of clientfd to the epoll of this subReactor.
In essence, the mainReactor is only responsible for establishing connection events. It doesn't handle business logic or care about the IO events of connected sockets.

 

subReactor:

There are typically multiple subReactors.
Each subReactor is operated by a separate thread.
The epoll in the subReactor registers the read and write events of clientfd.
When an IO event occurs, it requires business processing.

 

### 5. Timer ###
The Timer serves as a collection of TimerEvent objects and inherits from FdEvent.

```cpp
addTimerEvent(): Adds a timer event.
deleteTimerEvent(): Removes a timer event.
onTimer(): Method to be executed after an IO event occurs.
resetArriveTime(): Resets the timer's arrival time.
```

Storage:
The Timer uses a multimap to store TimerEvent objects with the structure: <key(arriveTime), TimerEvent>.


### 6. IO Thread ###
The IO thread is responsible for:

Creating a new thread (pthread_create).
Initializing an EventLoop within the new thread.
Starting the loop.

```cpp
class {
    pthread_t m_thread;
    pid_t m_thread_id;
    EventLoop event_loop;
}
```

### 7. RPC Server Workflow ###
Upon startup, the OrderService object is registered.

1. Read data from the buffer and decode it to obtain the TinyPBProtocol request object. From this request, retrieve the method_name. Then, based on service.method_name, locate the corresponding method func.

2. Identify the appropriate request type and response type.

3. Deserialize the pb_data from the TinyPBProtocol request into an object of the request type. Declare an empty object of the response type.

4. Execute func(request, response).

5. Serialize the response object into pb_data. Insert it into the TinyPBProtocol structure, encode it, and then place it into the buffer.