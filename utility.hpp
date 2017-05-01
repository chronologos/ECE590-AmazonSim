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
#include "./amazon.pb.h"
#include "./internalcom.pb.h"
#include "./ig.pb.h"

#define WAITING_ARRIVED 0
#define WAITING_READY_TRUCK 1
#define WAITING_TRUCK 2 // Ready, Truck not arrived
#define WAITING_READY 3 // Truck arrived, not Ready
#define WAITING_LOAD 4
#define WAITING_TRUCK_DISPATCH 5
#define DISPATCHED 6
#define DELIVERED 7

#define MIN_TRUCK_LOAD 3
#define NUM_WAREHOUSES 10

#define WORLD_ID 1007
#define SIM_SPEED 120


void WriteBytes(int socket, std::string str);
void ReadXBytes(int socket, uint64_t x, void* buffer);
template<typename T> bool sendMsgToSocket(const T & message, int fd);
//bool sendMsgToSocket(const ::google::protobuf::Message & message, int fd);
//bool recvMsgFromSocket(google::protobuf::Message & message, int fd);
template<typename T> bool recvMsgFromSocket(T & message, int fd);
bool makeLoadRequest(std::vector<std::tuple<int, int, unsigned long>> loadInfo, int sim_sock);
//template bool sendMsgToSocket<AConnect>(AConnect& message, int fd);

#endif

