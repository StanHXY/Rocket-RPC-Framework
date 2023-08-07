#include <unistd.h>
#include "rocket/common/log.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/tinypb_coder.h"

namespace rocket {

TcpConnection::TcpConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, NetAddr::s_ptr local_addr, TcpConnectionType type /*= TcpConnectionByServer*/)
    : m_event_loop(event_loop), m_local_addr(local_addr), m_peer_addr(peer_addr), m_state(NotConnected), m_fd(fd), m_connection_type(type) {
    
  m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
  m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

  m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);
  m_fd_event->setNonBlock();

  m_coder = new TinyPBCoder();

  if (m_connection_type == TcpConnectionByServer) {
    listenRead();
  }

}

TcpConnection::~TcpConnection() {
  DEBUGLOG("~TcpConnection");
  if (m_coder) {
    delete m_coder;
    m_coder = NULL;
  }
}

void TcpConnection::onRead() {

  if (m_state != Connected) {
    ERRORLOG("onRead error, client has already disconneced, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
    return;
  }

  bool is_read_all = false;
  bool is_close = false;
  while(!is_read_all) {
    if (m_in_buffer->writeAble() == 0) {
      m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
    }
    int read_count = m_in_buffer->writeAble();
    int write_index = m_in_buffer->writeIndex(); 

    int rt = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
    DEBUGLOG("success read %d bytes from addr[%s], client fd[%d]", rt, m_peer_addr->toString().c_str(), m_fd);
    if (rt > 0) {
      m_in_buffer->moveWriteIndex(rt);
      if (rt == read_count) {
        continue;
      } else if (rt < read_count) {
        is_read_all = true;
        break;
      }
    } else if (rt == 0) {
      is_close = true;
      break;
    } else if (rt == -1 && errno == EAGAIN) {
      is_read_all = true;
      break;
    }
  }

  if (is_close) {
    //TODO: 
    INFOLOG("peer closed, peer addr [%s], clientfd [%d]", m_peer_addr->toString().c_str(), m_fd);
    clear();
    return;
  }

  if (!is_read_all) {
    ERRORLOG("not read all data");
  }
 
  excute();

}

void TcpConnection::execute() {
  if (m_connection_type == TcpConnectionByServer) {
    // Execute business logic for RPC requests, get RPC responses, and send them back
    std::vector<AbstractProtocol::s_ptr> result;
    m_coder->decode(result, m_in_buffer);
    for (size_t i = 0; i < result.size(); ++i) {
      // 1. For each request, call the RPC method to get the response message.
      // 2. Put the response message into the send buffer and listen for write events to send the response.
      INFOLOG("Successfully received request[%s] from client[%s]", result[i]->m_msg_id.c_str(), m_peer_addr->toString().c_str());

      std::shared_ptr<TinyPBProtocol> message = std::make_shared<TinyPBProtocol>();
      // message->m_pb_data = "hello. this is rocket rpc test data";
      // message->m_msg_id = result[i]->m_msg_id;

      RpcDispatcher::GetRpcDispatcher()->dispatch(result[i], message, this);
    }

  } else {
    // Decode message objects from the buffer and execute their callbacks.
    std::vector<AbstractProtocol::s_ptr> result;
    m_coder->decode(result, m_in_buffer);

    for (size_t i = 0; i < result.size(); ++i) {
      std::string msg_id = result[i]->m_msg_id;
      auto it = m_read_dones.find(msg_id);
      if (it != m_read_dones.end()) {
        it->second(result[i]);
        m_read_dones.erase(it);
      }
    }
  }
}



void TcpConnection::reply(std::vector<AbstractProtocol::s_ptr>& replay_messages) {
  m_coder->encode(replay_messages, m_out_buffer);
  listenWrite();
}

void TcpConnection::onWrite() {
  // Send all the data in the current out_buffer to the client.

  if (m_state != Connected) {
    ERRORLOG("onWrite error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
    return;
  }

  if (m_connection_type == TcpConnectionByClient) {
    // 1. Encode messages to byte streams.
    // 2. Put the byte streams into the buffer and send them all.

    std::vector<AbstractProtocol::s_ptr> messages;

    for (size_t i = 0; i < m_write_dones.size(); ++i) {
      messages.push_back(m_write_dones[i].first);
    }

    m_coder->encode(messages, m_out_buffer);
  }

  bool is_write_all = false;
  while (true) {
    if (m_out_buffer->readAble() == 0) {
      DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
      is_write_all = true;
      break;
    }
    int write_size = m_out_buffer->readAble();
    int read_index = m_out_buffer->readIndex();

    int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);

    if (rt >= write_size) {
      DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
      is_write_all = true;
      break;
    } if (rt == -1 && errno == EAGAIN) {
      // The send buffer is full, cannot send more data.
      // We will wait and send data again when the fd becomes writable.
      ERRORLOG("write data error, errno==EAGAIN and rt == -1");
      break;
    }
  }
  if (is_write_all) {
    m_fd_event->cancel(FdEvent::OUT_EVENT);
    m_event_loop->addEpollEvent(m_fd_event);
  }

  if (m_connection_type == TcpConnectionByClient) {
    for (size_t i = 0; i < m_write_dones.size(); ++i) {
      m_write_dones[i].second(m_write_dones[i].first);
    }
    m_write_dones.clear();
  }
}

void TcpConnection::setState(const TcpState state) {
  m_state = state;
}

TcpState TcpConnection::getState() {
  return m_state;
}


void TcpConnection::clear() {
  // Perform cleanup actions after closing the connection.
  if (m_state == Closed) {
    return;
  }
  m_fd_event->cancel(FdEvent::IN_EVENT);
  m_fd_event->cancel(FdEvent::OUT_EVENT);

  m_event_loop->deleteEpollEvent(m_fd_event);

  m_state = Closed;
}

void TcpConnection::shutdown() {
  if (m_state == Closed || m_state == NotConnected) {
    return;
  }

  // In a half-closed state.
  m_state = HalfClosing;

  // Call shutdown to close read and write operations on the socket,
  // which means the server will no longer read from or write to this fd.
  // It sends a FIN packet, triggering the first phase of the four-way handshake.
  // When the fd has a readable event, but the readable data is 0, it means the peer sent a FIN.
  ::shutdown(m_fd, SHUT_RDWR);
}



void TcpConnection::setConnectionType(TcpConnectionType type) {
  m_connection_type = type;
}


void TcpConnection::listenWrite() {

  m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
  m_event_loop->addEpollEvent(m_fd_event);
}


void TcpConnection::listenRead() {

  m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
  m_event_loop->addEpollEvent(m_fd_event);
}


void TcpConnection::pushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
  m_write_dones.push_back(std::make_pair(message, done));
}

void TcpConnection::pushReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
  m_read_dones.insert(std::make_pair(msg_id, done));
}


NetAddr::s_ptr TcpConnection::getLocalAddr() {
  return m_local_addr;
}

NetAddr::s_ptr TcpConnection::getPeerAddr() {
  return m_peer_addr;
}


int TcpConnection::getFd() {
  return m_fd;
}

}