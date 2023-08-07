#ifndef ROCKET_NET_CODER_TINYPB_CODER_H
#define ROCKET_NET_CODER_TINYPB_CODER_H

#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"

namespace rocket {

class TinyPBCoder : public AbstractCoder {

 public:

  TinyPBCoder() {}
  ~TinyPBCoder() {}

  // Convert message objects to byte stream and write them into the buffer.
  void TinyPBCoder::encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer);

  // Convert byte stream in the buffer to message objects.
  void TinyPBCoder::decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer);



 private:
  const char* encodeTinyPB(std::shared_ptr<TinyPBProtocol> message, int& len);

};


}


#endif