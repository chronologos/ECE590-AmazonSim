#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <iostream>
#include <sys/socket.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/message.h>


void WriteBytes(int socket, std::string str);
void ReadXBytes(int socket, uint64_t x, void* buffer);
//template<typename T> bool sendMsgToSocket(const T & message, int fd);
bool sendMsgToSocket(const ::google::protobuf::Message & message, int fd);
bool recvMsgFromSocket(google::protobuf::Message & message, int fd);
//template<typename T> bool recvMsgFromSocket(const T & message, int fd);


#endif
