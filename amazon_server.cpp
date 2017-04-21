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

#include <map>

#include "./utility.hpp"
#include <google/protobuf/message_lite.h>
#include "./amazon.pb.h"
//#include "./internalcom.pb.h"
#include "nonBlockingIO/sleepyProtobuff.h"

using namespace std;


std::map<unsigned long, int> shipmentStatus; // a cache over the TrackingNumbers table in DB
//std::map<int, unsigned long> socksToShipments; // map used by internal server mapping socket FDs to shipment numbers


int connectToSim() {  
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = "10.236.48.19";
  const char *port     = "12345";
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
  std::cout << "About to connect\n";
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  std::cout << "Successfully connected to socket\n";
  int sim_sock = status;
  AConnect test;
  test.set_worldid(1);
  std::cout << "World id of connect message: " << test.worldid() << "\n";
  sendMsgToSocket(*((google::protobuf::Message*)&test), socket_fd); 
  
  AConnected aConn;
  google::protobuf::Message * rcvMessage = (google::protobuf::Message *)&aConn;
  bool response = recvMsgFromSocket(*rcvMessage, socket_fd);
  if (!response) {
    puts("Error reading!");
    //close(socket_fd);
    return -1;
  }
  if (aConn.has_error()) {
    puts("Server returned error!");
    std::cout << "Error from server : " << aConn.error() << "\n";
    //close(socket_fd);
    return -1;
  }
  puts("Success!");
  //close(socket_fd);
  return sim_sock;
}



int main(int argc, char* argv[]) {
  int sim_sock;
  if ((sim_sock = connectToSim()) < 0) {
    std::cout << "Unable to connect to sim!\n";
    exit(1);
  }
  std::cout << "About to launch internal comm server\n";
  std::thread * internalServer = launchInternalServer(sim_sock);
  std::cout << "Successfully launched internal server\n";

  while(1) { // select loop on socket with sim server

  }

  std::cout << "Main thread waiting on internal server to be done\n";
  (*internalServer).join();
  std::cout << "Main thread exiting\n";
  return 0;
}

