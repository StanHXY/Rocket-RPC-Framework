#ifndef ROCKET_COMMON_ERROR_CODER_H
#define ROCKET_COMMON_ERROR_CODER_H

#ifndef SYS_ERROR_PREFIX
#define SYS_ERROR_PREFIX(xx) 1000##xx
#endif


const int ERROR_PEER_CLOSED = SYS_ERROR_PREFIX(0000);              // Connection closed by peer during connection
const int ERROR_FAILED_CONNECT = SYS_ERROR_PREFIX(0001);           // Connection failed
const int ERROR_FAILED_GET_REPLY = SYS_ERROR_PREFIX(0002);         // Failed to retrieve reply packet
const int ERROR_FAILED_DESERIALIZE = SYS_ERROR_PREFIX(0003);       // Deserialization failed
const int ERROR_FAILED_SERIALIZE = SYS_ERROR_PREFIX(0004);         // Serialization failed

const int ERROR_FAILED_ENCODE = SYS_ERROR_PREFIX(0005);           // Encoding failed
const int ERROR_FAILED_DECODE = SYS_ERROR_PREFIX(0006);           // Decoding failed

const int ERROR_RPC_CALL_TIMEOUT = SYS_ERROR_PREFIX(0007);        // RPC call timed out

const int ERROR_SERVICE_NOT_FOUND = SYS_ERROR_PREFIX(0008);       // Service not found
const int ERROR_METHOD_NOT_FOUND = SYS_ERROR_PREFIX(0009);        // Method not found
const int ERROR_PARSE_SERVICE_NAME = SYS_ERROR_PREFIX(0010);      // Failed to parse service name
const int ERROR_RPC_CHANNEL_INIT = SYS_ERROR_PREFIX(0011);        // RPC channel initialization failed
const int ERROR_RPC_PEER_ADDR = SYS_ERROR_PREFIX(0012);           // Peer address exception during RPC call



#endif