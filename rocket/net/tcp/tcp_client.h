#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include <memory>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/timer_event.h"


namespace rocket {

class TcpClient {
 public:
  typedef std::shared_ptr<TcpClient> s_ptr;

  TcpClient(NetAddr::s_ptr peer_addr);

  ~TcpClient();

  // Asynchronously perform connection.
// If the connection is successful, the 'done' function will be executed.
void TcpClient::connect(std::function<void()> done) {
  // Code for connecting to the server...
}

// Asynchronously send a message.
// If sending the message is successful, the 'done' function will be called with the message object as an argument.
void TcpClient::writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
  // Code for writing the message to the connection's buffer and listening for write events...
}

// Asynchronously read a message.
// If reading the message is successful, the 'done' function will be called with the message object as an argument.
void TcpClient::readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
  // Code for listening for read events and decoding the message object from the buffer...
}


  void stop();

  int getConnectErrorCode();

  std::string getConnectErrorInfo();

  NetAddr::s_ptr getPeerAddr();

  NetAddr::s_ptr getLocalAddr();

  void initLocalAddr();

  void addTimerEvent(TimerEvent::s_ptr timer_event);


 private:
  NetAddr::s_ptr m_peer_addr;
  NetAddr::s_ptr m_local_addr;

  EventLoop* m_event_loop {NULL};

  int m_fd {-1};
  FdEvent* m_fd_event {NULL};

  TcpConnection::s_ptr m_connection;

  int m_connect_error_code {0};
  std::string m_connect_error_info;

};  
}

#endif