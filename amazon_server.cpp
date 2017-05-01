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
//#include <set>
#include <deque>

#include "./utility.hpp"
#include <google/protobuf/message_lite.h>
#include "./amazon.pb.h"
//#include "./internalcom.pb.h"
#include "nonBlockingIO/sleepyProtobuff.h"

#include "nonBlockingIO/ups_comm.h"


using namespace std;


std::map<unsigned long, int> shipmentStatus; // a cache over the TrackingNumbers table in DB
//std::map<int, unsigned long> socksToShipments; // map used by internal server mapping socket FDs to shipment numbers


//int UPS_FD = -1;

int connectToSim() {  
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = "10.236.48.19";
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
  std::cout << "About to connect\n";
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    cerr << "Error: cannot connect to socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  }
  std::cout << "Successfully connected to sim on  socket " << socket_fd << "\n";
  int sim_sock = socket_fd;
  AConnect test;
  test.set_worldid(WORLD_ID);

  // TO - DO : Make number and/or position of warehouses configurable with command-line args?
  /*
  AInitWarehouse wh;
  wh.set_x(-664);
  wh.set_y(-1081);
  AInitWarehouse * newWh = test.add_initwh();
  *newWh = wh;
  */
  sendMsgToSocket(test, socket_fd);
  AConnected aConn;
  //google::protobuf::Message * rcvMessage = (google::protobuf::Message *)&aConn;
  //bool response = recvMsgFromSocket(*rcvMessage, socket_fd);
  bool response = recvMsgFromSocket(aConn, socket_fd);
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

  // Set sim socket to be non-blocking
  int argp = 1;
  if (ioctl(sim_sock, FIONBIO, (char*)&argp) < 0) {
      perror("ioctl() on sim sock failed");
      close(sim_sock);
      return -1;
   }
  std::cout << "Set sim socket to be non-blocking!\n";
  return sim_sock;
}


// Need to iterate through 
int handleArrived(ACommands * aCommands, AResponses * aResponses, int * result) {
  if (aResponses->arrived_size() == 0) {
    std::cout << "This AResponse has no 'arrived' messages\n";
    return 0;
  }
  APurchaseMore aPurchaseMore;
  AProduct aProduct;
  unsigned long product_id;
  int count;
  for (int num = 0; num < aResponses->arrived_size(); num ++) {
    aPurchaseMore = aResponses->arrived(num);
    for (int product = 0; product < aPurchaseMore.things_size(); product ++) {
      aProduct = aPurchaseMore.things(product);
      product_id = aProduct.id();
      count = aProduct.count();
      // Update DB
      if (updateInventory(product_id, count) < 0) {
        std::cout << "Error updating inventory for product with id " << product_id << "\n";
        *result = 0; // Set error flag
      }
      else {
        std::cout << "Successfully updated inventory of product with id " << product_id << "\n";
        // Check which shipids can now be sent for packing
      }
    }
  }
  std::cout << "Done handling 'arrived' responses from sim\n";
  return aResponses->arrived_size();
}

// Increment FSM_STATE of shipids in TrackingNumbers table and send APutOnTruck message 
int handleReady(ACommands * aCommands, AResponses * aResponses, int * result, int sim_sock) { 
  if (aResponses->ready_size() == 0) {
    std::cout << "This AResponse has no 'ready' messages\n";
    return 0;
  }
  unsigned long shipid;
  //int whnum; 
  //int truckid; TEMP - JUST 1 FOR NOW; SOON NEED TO TALK TO UPS TEAM FOR COORDINATION
  APutOnTruck load;
  /*
  load.set_whnum(1); // TEMP
  //load.set_truckid(1); // TEMP
  load.set_truckid(0); // TEMP
  */
  std::vector<std::tuple<int, int, unsigned long>> loadInfo;
  std::tuple<int, int, unsigned long> shipInfo;
  for (int num = 0; num < aResponses->ready_size(); num ++) {
    shipid = aResponses->ready(num);
    // increment FSM_STATE
    /*
    if (incrementShipmentState(shipid) < 0) { // TEMP - MIGHT HAVE TO REPLACE BY CHECKING OF CURRENT STATE AND USING SET SHIPMENT STATE, depending on whether UPS truck has arrived
      std::cout << "Error incrementing FSM_STATE of shipid " << shipid << " from packing to loading\n"; // TO-DO : Error-Recovery follow-up action?
      *result = 0; // flag error
      continue; // TBD - Skip this shipment but continue processing other shiments?
    }
    
    // ONLY EXECUTE THE BELOW IF FSM_STATE == (READY AND TRUCK_ARRIVED)
    // shipment state for this shipid is set as loading
    // Construct APutOnTruck message and add to aCommands
    load.set_shipid(shipid);
    APutOnTruck* newLoad = aCommands->add_load();
    *newLoad = load;
  }
  */
    // Following method will change fsm state from WAITING_READY_TRUCK to WAITING_TRUCK and from WAITING_READY to WAITING_LOAD, returning a vector of info for packages that are now WAITING_LOAD
    shipInfo = setReady(shipid);
    if (std::get<1>(shipInfo) < 0) { // no truck at the warehouse of this shipment
      std::cout << "Shipment " << shipid << " not ready for loading as truck has not arrived to its warehouse.\n";  
    }   
    else {
      loadInfo.push_back(shipInfo);
      std::cout << "Shipment " << shipid << " ready for loading as truck has already arrived to warehouse " << std::get<0>(shipInfo) << "\n";
    }
  }
  if (loadInfo.size() > 0) {
    std::cout << "Package " << shipid << " already had truck and is now packed, making load request to sim!\n";
    if (!makeLoadRequest(loadInfo, sim_sock)) { 
      std::cout << "Error making load request for ready packages upon truck arrival!\n";
    }
    else { // load request sent to sim successfully
      std::cout << "Successfully made load request for ready packages upon truck arrival!\n";
    }
  }
  else {
    std::cout << "No packages ready for loading as trucks have not arrived to their warehouses!\n";
  }
  std::cout << "Done handling 'ready' responses from sim\n";
  return aResponses->ready_size();
}

// Increment FSM_STATE of shipids in TrackingNumbers table and send custom 'Deliver' message to UPS
int handleLoaded(ACommands * aCommands, AResponses * aResponses, int * result) {
  if (aResponses->loaded_size() == 0) {
    std::cout << "This AResponse has no 'loaded' messages\n";
    return 0;
  }
  unsigned long shipid;
  for (int num = 0; num < aResponses->loaded_size(); num ++) {
    shipid = aResponses->loaded(num);
    // increment FSM_STATE
    if (incrementShipmentState(shipid) < 0) { // increments from 4 (wait for load) to 5 (loaded)
      std::cout << "Error incrementing FSM_STATE of shipid " << shipid << " from loading to loaded\n"; // TO-DO : Error-Recovery follow-up action?
      *result = 0; // flag error
      continue; // TBD - Skip this shipment but continue processing other shiments?
    }
    // TO-DO : Construct custom Message for sending to UPS for commencing delivery? Need truckid, whid, shipid?
    // Check if that warehouse is ready for loading
    int mustSendTruck = 0;
    int whid;
    if ((whid = readyForDispatch(shipid, &mustSendTruck)) < 0) { // not ready to dispatch truck, nothing to do
      std::cout << "Warehouse for shipment " << shipid << " not ready for truck dispatch, not doing anything now\n";
      continue;
    }
    else {
      std::cout << "Warehouse id for " << shipid << " is " << whid << ", it is ready for truck dispatch!\n";
      int requested;
      //if ((requested = requestDispatch(whid, &UPS_FD)) < 0) {
      if ((requested = requestDispatch(whid)) < 0) {
        std::cout << "Unable to dispatch truck despite warehouse being ready!\n";
        continue;
      }
      else if (requested == 0) { // Logically not possible, only possible when requesting sendTruck
        std::cout << "UPS not yet connected, dispatch queued?\n";
      }
      else {
        std::cout << "Successfully dispatched";
        // request for truck to be sent again if necessary
        if (mustSendTruck) {
          std::cout << "Warehouse has shipments waiting to be loaded, requesting another truck for this warehouse!\n";
          int sendTruckRequest;
          if ((sendTruckRequest = requestTruck(whid)) < 0) {
            std::cout << "Unable to request truck to warehouse " << whid << "!\n";
            continue;
          }
          else if (sendTruckRequest == 0) { // Also shouldn't be happening
            std::cout << "UPS not yet connected, sendTruck queued?\n";
            continue;
          }
          else {
            std::cout << "Successfully requested another truck to be sent to warehouse " << whid << "!\n";
          }
        }
        else {
          std::cout << "Warehouse " << whid << " does not need another truck right now\n";
        }
      }
    }
  }
  std::cout << "Done handling 'loaded' responses from sim\n";
  return aResponses->loaded_size();
}

int handleError(ACommands * aCommands, AResponses * aResponses, int * result) {
  if (aResponses->has_error()) {
    std::cout << "Error message sent by sim! :";
    std::cout << aResponses->error();
    return -1;
  }
  else {
    std::cout << "No error messages sent by sim in this AResponse!\n";
    return 0;
  }
}
// Decipher message, retrieve FSM state of shipment from DB, update FSM state, construct message to send, return next message to be written
ACommands handleAResponses(AResponses * aResponses, int * result, int sim_sock) { 
  ACommands aCommands;
  if (handleError(&aCommands, aResponses, result)) { // TO - DO : Ok to just abort here if error detected?
    std::cout << "Aborting response\n";
    *result = 0;
    return aCommands;
  }
  /*
  if (!aResponses->has_finished()) { // nothing to do
    std::cout << "This AResponse has no 'finished' flag, treating as mere ack\n";
    *result = 0;
    return aCommands;

  }
  */
  if (aResponses->has_finished()) { // can proceed to close socket
    *result = 0;
    return aCommands;
  }
  int totalHandled = 0;
  totalHandled += handleArrived(&aCommands, aResponses, result);
  totalHandled += handleReady(&aCommands, aResponses, result, sim_sock);
  totalHandled += handleLoaded(&aCommands, aResponses, result);

  aCommands.set_simspeed(SIM_SPEED);

  if (totalHandled == 0) {
    std::cout << "Nothing to do for this response, treating as mere ack\n";
    *result = 0;
  }
  return aCommands;
}

bool makeLoadRequest(std::vector<std::tuple<int, int, unsigned long>> loadInfo, int sim_sock) {
  if (loadInfo.size() == 0) return true;
  ACommands aCommand;
  APutOnTruck aPutOnTruck;
  APutOnTruck* holder;
  int whnum;
  int truckid;
  unsigned long shipid;
  std::tuple<int, int, unsigned long> info;
  // iterate through, make an APutOnTruck message for each
  for (std::vector<std::tuple<int, int, unsigned long>>::iterator it = loadInfo.begin(); it < loadInfo.end(); it ++) {
    info = *it;
    whnum = std::get<0>(info);
    truckid = std::get<1>(info);
    shipid = std::get<2>(info);
    aPutOnTruck.set_whnum(whnum);
    aPutOnTruck.set_truckid(truckid);
    aPutOnTruck.set_shipid(shipid);
    holder = aCommand.add_load();
    *holder = aPutOnTruck;
  }
  if (!sendMsgToSocket(aCommand, sim_sock)) {
    std::cout << "Unable to send request to sim for loading of packages upon arrival of truck!\n";
    return false;
  }
  std::cout << "Successfully sent request to sim for loading of packages upon arrival of truck!\n";
  return true; // TEMP
}


// Select loop of main server
void talkToSim(int sim_sock) { // Already connected to the socket by now
  struct fd_set master_set, read_set;//, write_set;
  int available;
  //struct timeval timeout; // Use no timeout?
  FD_ZERO(&master_set);
  FD_SET(sim_sock, &master_set);
  std::deque<ACommands> outgoing;
  while (1) { // TO-DO : Exit Condition? Signal Handler?
    memcpy(&read_set, &master_set, sizeof(read_set));
    //memcpy(&write_set, &master_set, sizeof(write_set));
    //available = select(sim_sock + 1, &read_set, &write_set, NULL, NULL);
    available = select(sim_sock + 1, &read_set, NULL, NULL, NULL);
    std::cout << "Awoke from slumber!\n";
    if (available > 0) {
      //std::cout << "Data available on non-blocking sim socket!\n";
      if (FD_ISSET(sim_sock, &read_set)) {
        std::cout << "Data available on non-blocking sim socket!\n";
        // The actual read operation can be done in a blocking way using the protobuff library methods
        AResponses aResponses;
        //if (!recvMsgFromSocket(*((google::protobuf::Message*)&aResponses), sim_sock)) {
        if (!recvMsgFromSocket(aResponses, sim_sock)) {
          std::cout << "ERROR receiving AResponse from sim!\n";
        }
        else {
          int success = 1;
          ACommands nextCommand = handleAResponses(&aResponses, &success, sim_sock);
          if (!success) {
            std::cout << "No follow-up command generated for this sim response!\n";
          }
          else {
            std::cout << "Successfully processed sim response!\n";
            //outgoing.emplace(nextCommand);
            
            //outgoing.push_back(nextCommand);
            if (!sendMsgToSocket(nextCommand, sim_sock)) {
              std::cout << "ERROR sending ACommand to sim!\n";
            }
            else {
            std::cout << "Successfully sent ACommand!\n";
            }
          }
        }
      }
    }
  }
  close(sim_sock);
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
  
  
  std::cout << "About to launch UPS comm server\n";
  //std::thread * upsMessenger = launchUPSMessenger(&UPS_FD); // will be set to valid fd once connection received from UPS
  std::thread * upsMessenger = launchUPSMessenger(sim_sock); // will be set to valid fd once connection received from UPS
  std::cout << "Successfully launched UPS messenger!\n";

  std::cout << "Main thread about to enter select loop on sim socket\n";
  talkToSim(sim_sock);
  
  std::cout << "Main thread waiting on internal server to be done\n";
  (*internalServer).join();
  (*upsMessenger).join();
  delete internalServer;
  delete upsMessenger;
  //close(sim_sock);
  std::cout << "Main thread exiting\n";
  return 0;
}

