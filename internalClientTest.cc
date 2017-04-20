#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include "./utility.hpp"
#include <google/protobuf/message_lite.h>
#include "./internalcom.pb.h"

using namespace std;


int connectToBackend() {  
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = "127.0.0.1";
  const char *port     = "23456";
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } 
  socket_fd = socket(host_info_list->ai_family,
    host_info_list->ai_socktype,
    host_info_list->ai_protocol);
    if (socket_fd == -1) {
      cerr << "Error: cannot create socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      return -1;
    } 
    status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      cerr << "Error: cannot connect to socket" << endl;
      cerr << "  (" << hostname << "," << port << ")" << endl;
      return -1;
    }

    Order order;
    order.set_shipid(273);
    std::cout << "Shipid of order: " << order.shipid() << "\n";
    sendMsgToSocket(*((google::protobuf::Message*)&order), socket_fd); 

    OrderReply orderReply;
    google::protobuf::Message * rcvMessage = (google::protobuf::Message *)&orderReply;
    bool response = recvMsgFromSocket(*rcvMessage, socket_fd);
    if (!response) {
      puts("Error reading!");
      //close(socket_fd);
      return -1;
    }
    if (orderReply.has_error()) {
      puts("Server returned error!");
      std::cout << "Error from server : " << orderReply.error() << "\n";
      //close(socket_fd);
      return -1;
    }
    puts("Success!");
    std::cout << "Shipid returned from server: " << orderReply.shipid() << "\n";
    //close(socket_fd);
    return 0;

//    return 0;
}



int main(int argc, char* argv[]) {
  if (connectToBackend()) {
    std::cout << "Unable to connect to sim!\n";
    exit(1);
  }
  std::cout << "Client exiting!\n";
  return 0;
}
