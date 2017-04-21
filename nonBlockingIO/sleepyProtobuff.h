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

#include "../utility.hpp"
#include <google/protobuf/message_lite.h>
#include "../amazon.pb.h"
#include "../internalcom.pb.h"
#include "../amazon.pb.h"

#include "../dbHandler.h"

#define SLEEPY_SERVER_PORT  23456
#define LISTEN_BACKLOG 128
#define SERVICE_QUOTA 10

int sleepyListen(int sim_sock); 

std::thread * launchInternalServer(int sim_sock); 
