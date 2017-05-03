#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <list>
#include <sstream>
#include <stdexcept>

#include <thread>
#include <set>
#include <map>
#include <deque>

#include <thread>
#include <mutex>


#include "../utility.hpp"
#include <google/protobuf/message_lite.h>
#include "../amazon.pb.h"
#include "../internalcom.pb.h"
#include "../amazon.pb.h"
#include "../ig.pb.h"

#include "../dbHandler.h"

#define UPS_COMM_PORT  34567
#define LISTEN_BACKLOG 128

//int sleepyListen(int sim_sock); 

//std::thread * launchUPSMessenger(int * upsFD);
std::thread * launchUPSMessenger(int sim_sock); 
//std::thread * launchUPSMessenger(int ups_sock); 
void addToUPSMsgQueue(AmazontoUPS msg);
int requestTruck(int whnum, unsigned long shipid, int delX, int delY, bool hasPackages);
//int requestTruck(int whid, int * upsFD);
//int requestTruck(int whid);
//int requestDispatch(int truckid, * upsFD); 
int requestDispatch(int truckid); 

//std::deque<AmazontoUPS> outgoing;
//std::mutex queueMutex;