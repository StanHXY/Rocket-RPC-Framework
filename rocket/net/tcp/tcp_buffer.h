#ifndef ROCKET_NET_TCP_TCP_BUFFER_H
#define ROCKET_NET_TCP_TCP_BUFFER_H

#include <vector>
#include <memory>

namespace rocket {

class TcpBuffer {

 public:

  typedef std::shared_ptr<TcpBuffer> s_ptr;

  TcpBuffer(int size);

  ~TcpBuffer();

  
  int readAble();

  
  int writeAble();

  int readIndex();

  int writeIndex();

  void writeToBuffer(const char* buf, int size);

  void readFromBuffer(std::vector<char>& re, int size);

  void resizeBuffer(int new_size);

  void adjustBuffer();

  void moveReadIndex(int size);

  void moveWriteIndex(int size);

 private:
  int m_read_index {0};
  int m_write_index {0};
  int m_size {0};

 public:
  std::vector<char> m_buffer;

};


}


#endif