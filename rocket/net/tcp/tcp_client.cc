#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/common/error_code.h"
#include "rocket/net/tcp/net_addr.h"

namespace rocket {

TcpClient::TcpClient(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
  m_event_loop = EventLoop::GetCurrentEventLoop();
  m_fd = socket(peer_addr->getFamily(), SOCK_STREAM, 0);

  if (m_fd < 0) {
    ERRORLOG("TcpClient::TcpClient() error, failed to create fd");
    return;
  }

  m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(m_fd);
  m_fd_event->setNonBlock();

  m_connection = std::make_shared<TcpConnection>(m_event_loop, m_fd, 128, peer_addr, nullptr, TcpConnectionByClient);
  m_connection->setConnectionType(TcpConnectionByClient);
 
}

TcpClient::~TcpClient() {
  DEBUGLOG("TcpClient::~TcpClient()");
  if (m_fd > 0) {
    close(m_fd);
  }
}

// Asynchronously establish the connection.
// If the connection is successful, the 'done' function will be executed.
void TcpClient::connect(std::function<void()> done) {
  int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
  if (rt == 0) {
    DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str());
    m_connection->setState(Connected);
    initLocalAddr();
    if (done) {
      done();
    }
  } else if (rt == -1) {
    if (errno == EINPROGRESS) {
      // Listen for write events using epoll and then check the error code.
      m_fd_event->listen(FdEvent::OUT_EVENT,
        [this, done]() {
          int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
          if ((rt < 0 && errno == EISCONN) || (rt == 0)) {
            DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str());
            initLocalAddr();
            m_connection->setState(Connected);
          } else {
            if (errno == ECONNREFUSED) {
              m_connect_error_code = ERROR_PEER_CLOSED;
              m_connect_error_info = "connection refused, sys error = " + std::string(strerror(errno));
            } else {
              m_connect_error_code = ERROR_FAILED_CONNECT;
              m_connect_error_info = "connection unknown error, sys error = " + std::string(strerror(errno));
            }
            ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno));
            close(m_fd);
            m_fd = socket(m_peer_addr->getFamily(), SOCK_STREAM, 0);
          }

          // Remove the write event listener after connection is complete to prevent continuous triggering.
          m_event_loop->deleteEpollEvent(m_fd_event);
          DEBUGLOG("now begin to done");
          // Execute the callback function only when the connection is completed.
          if (done) {
            done();
          }
        }
      );
      m_event_loop->addEpollEvent(m_fd_event);

      if (!m_event_loop->isLooping()) {
        m_event_loop->loop();
      }
    } else {
      ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno));
      m_connect_error_code = ERROR_FAILED_CONNECT;
      m_connect_error_info = "connect error, sys error = " + std::string(strerror(errno));
      if (done) {
        done();
      }
    }
  }
}



void TcpClient::stop() {
  if (m_event_loop->isLooping()) {
    m_event_loop->stop();
  }
}

// Asynchronously send a message.
// If sending the message is successful, the 'done' function will be called with the message object as an argument.
void TcpClient::writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
  // 1. Write the message object to the Connection's buffer, and also write the 'done' function.
  // 2. Start listening for connection's write event.
  m_connection->pushSendMessage(message, done);
  m_connection->listenWrite();
}


// Asynchronously read a message.
// If reading the message is successful, the 'done' function will be called with the message object as an argument.
void TcpClient::readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
  // 1. Listen for read events.
  // 2. Decode the message object from the buffer, check if the msg_id matches. If matched, the read is successful, and its callback will be executed.
  m_connection->pushReadMessage(msg_id, done);
  m_connection->listenRead();
}


int TcpClient::getConnectErrorCode() {
  return m_connect_error_code;
}

std::string TcpClient::getConnectErrorInfo() {
  return m_connect_error_info;

}

NetAddr::s_ptr TcpClient::getPeerAddr() {
  return m_peer_addr;
}

NetAddr::s_ptr TcpClient::getLocalAddr() {
  return m_local_addr;
}

void TcpClient::initLocalAddr() {
  sockaddr_in local_addr;
  socklen_t len = sizeof(local_addr);

  int ret = getsockname(m_fd, reinterpret_cast<sockaddr*>(&local_addr), &len);
  if (ret != 0) {
    ERRORLOG("initLocalAddr error, getsockname error. errno=%d, error=%s", errno, strerror(errno));
    return;
  }

  m_local_addr = std::make_shared<IPNetAddr>(local_addr);

}


void TcpClient::addTimerEvent(TimerEvent::s_ptr timer_event) {
  m_event_loop->addTimerEvent(timer_event);
}

}