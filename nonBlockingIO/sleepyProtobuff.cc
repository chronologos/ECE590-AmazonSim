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


/* ADAPTED FROM IBM Knowledge Center - "https://www.ibm.com/support/knowledgecenter/en/ssw_i5_54/rzab6/xnonblock.htm" */

#define SLEEPY_SERVER_PORT  23456
#define LISTEN_BACKLOG 128
#define SERVICE_QUOTA 10

int running = 1;
const char* response = "OK!\n\0";

//using namespace std;


int sleepyListen() {
	int    i, len, rc, on = 1;
   int    listen_sd, max_sd, new_sd;
   int    desc_ready = 0;//, end_server = 0;
   int    close_conn;
   char   buffer[80];
   struct sockaddr_in   addr;
   //struct timeval       timeout;
   struct fd_set        master_set, reading_set, writing_set;
   int serviced = 0;
   listen_sd = socket(AF_INET, SOCK_STREAM, 0);
   if (listen_sd < 0)
   {
      perror("socket() failed");
      //exit(-1);
  		return -1;
   }

   rc = ioctl(listen_sd, FIONBIO, (char *)&on);
   if (rc < 0)
   {
      perror("ioctl() failed");
      close(listen_sd);
      //exit(-1);
   		return -1;
   }

   /* Bind the socket                                           */
   memset(&addr, 0, sizeof(addr));
   addr.sin_family      = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port        = htons(SLEEPY_SERVER_PORT);
   rc = bind(listen_sd, (struct sockaddr *)&addr, sizeof(addr));
   if (rc < 0)
   {
      perror("bind() failed");
      close(listen_sd);
      //exit(-1);
      return -1;
   }

   /* Set the listen back log                                   */
   rc = listen(listen_sd, LISTEN_BACKLOG);
   if (rc < 0)
   {
      perror("listen() failed");
      close(listen_sd);
      //exit(-1);
      return -1;
   }

   /* Initialize the master fd_set                              */
   FD_ZERO(&master_set);
   max_sd = listen_sd;
   FD_SET(listen_sd, &master_set);

   /*
   timeout.tv_sec  = 3 * 60;
   timeout.tv_usec = 0;
   */

   std::set<int> mustWrite;
   std::map<int, unsigned long> shipids; // to support multiple front-end servers

   std::cout << "About to enter sleep loop\n";


   do {
   	memcpy(&reading_set, &master_set, sizeof(master_set));
   	memcpy(&writing_set, &master_set, sizeof(master_set));

   	desc_ready = 0;
   	rc = select(max_sd + 1, &reading_set, &writing_set, NULL, NULL);
   	if (rc < 0) {
   		perror("Error in select by listener: ");
   		return -1;
   	}
   	else if (rc > 0) {
   		//std::cout << "Listener has work to do! " << rc << " sockets are ready\n";
   		desc_ready = rc;
   		for (i = 0; i <= max_sd && desc_ready > 0; i ++) {
   			if (FD_ISSET(i, &reading_set)) {
   				desc_ready --;
   				if (i == listen_sd) { // New connection, must handle specially
   					std::cout << "New connection on listen socket!\n";

   					do
	               {
	                  /* Accept each incoming connection.  If       */
	                  /* accept fails with EWOULDBLOCK, then we     */
	                  /* have accepted all of them.  Any other      */
	                  /* failure on accept will cause us to skip that connection */
	                  new_sd = accept(listen_sd, NULL, NULL);
	                  if (new_sd < 0)
	                  {
	                     if (errno != EWOULDBLOCK)
	                     {
	                        perror(" Error accepting new connection!");
	                        //end_server = TRUE;
	                     	continue;
	                     }
	                     break;
	                  }

	                  /* Add the new incoming connection to the     */
	                  /* master read set                            */
	                  std::cout << " Accept socket: " << new_sd << "\n";
	                  FD_SET(new_sd, &master_set);
	                  if (new_sd > max_sd) {
	                     max_sd = new_sd;	                  	
	                  }
	                  /* Loop back up and accept another incoming   */
	                  /* connection                                 */
	               } while (new_sd != -1);
   				}

   				else { // Not listen fd - perform read
   					int numRead;
                  /*
   					char readBuf[100]; // TEMP - JUST FOR TESTING; NOT SAFE
   					memset(readBuf, 0, 100);
   					*/
                  Order order;
                  /*
                  if ((numRead = read(i, readBuf, 100)) < 0) {
   						if (errno != EWOULDBLOCK) {
   							perror("Error reading client request");
   						}
   						else {
   							std::cout << "Read data not available on socket " << i << ", will try again later\n";
   						}
   					}
   					else {
   						std::cout << "Successfully read " << numRead << " bytes from client\n";
   						std::cout << "Buffer:\n" << readBuf << "\n";
   						// push to set for writing
   						mustWrite.emplace(i);
   					}
                  */
                  if (!recvMsgFromSocket(*((google::protobuf::Message*)&order), i)) {
                     std::cout << "Unable to read internal proto message\n";
                     // close socket and remove from list
                     close(i);
                     FD_CLR(i, &master_set);

                  }
                  else {
                     // Extract shipid, add to map
                     unsigned long shipid = (unsigned long) order.shipid();
                     std::cout << "Extracted shipid: " << shipid << "\n";
                     shipids.insert(std::pair<int, unsigned long>(i, shipid));
                     // push to set for writing
                     mustWrite.emplace(i);
                  }
   				}
   			}
   			if (mustWrite.find(i) != mustWrite.end() && FD_ISSET(i, &writing_set)) {
   				// Need to write and ready for writing
   				desc_ready --;
   				std::cout << "Socket " << i << " is ready for writing\n";
   				/*
               int numWritten;
   				if ((numWritten = write(i, response, strlen(response))) < 0) { // Not necessary?
   					if (errno == EWOULDBLOCK) {
   						std::cout << "Will try writing to socket " << i << " later\n";
   						continue;   		
   						perror("Error writing to client");
   						close(i);
   						FD_CLR(i, &master_set);
   						mustWrite.erase(i);
   						
   					}
   				
   					else {
   						std::cout << "Socket " << i << " closed?\n";   						
   					}
   					
   				}
   				else {
   					std::cout << "Successfully wrote " << numWritten << " bytes to socket " << i << "\n";
   				}
               */
               // Retrieve shipid for this connection
               std::map<int, unsigned long>::iterator it = shipids.find(i);
               std::string errorStr;
               unsigned long shipid;
               if (it == shipids.end()) {
                  std::cout << "Error! No shipid found for this socket!\n";
                  shipid = -1;
                  errorStr = "Missing shipid, please try again.\n";
               }
               else {
                  shipid = it->second;
                  std::cout << "Shipid for socket " << i << " : " << shipid << "\n";
               }

               OrderReply orderReply;
               orderReply.set_shipid(shipid);
               if (shipid == -1) {
                  orderReply.set_error(errorStr);
               }
               // Serialize and write protobuff
               if (!sendMsgToSocket(*((google::protobuf::Message*)&orderReply), i)) {
                  std::cout << "Error writing orderReply to client\n";
               }
               else {
                  std::cout << "Successfully wrote orderReply to client\n";
               }

      			close(i);
   				FD_CLR(i, &master_set);
   				mustWrite.erase(i);
               shipids.erase(i);
   				serviced ++;
   			}
   		}
   	}
//   } while (running);
	} while (serviced < SERVICE_QUOTA);
	std::cout << "Sleepy server has hit work quota, retiring\n";
   return 0;
}

int main() {
	std::cout << "Main thread launching sleepy server thread\n";
	std::thread sleepyServer(sleepyListen);
	std::cout << "Main thread waiting for sleepy server to be done\n";
	sleepyServer.join();
	std::cout << "Sleepy server done, main thread also exiting\n";
	//sleepyListen();
	exit(0);
}