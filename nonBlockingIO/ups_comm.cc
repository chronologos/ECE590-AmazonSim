#include "./ups_comm.h"
#include <google/protobuf/text_format.h>

std::deque<AmazontoUPS> outgoing;
std::mutex queueMutex;

int UPS_FD = -1;

// Stores outgoing messages generated before UPS connected to Amazon
void addToUPSMsgQueue(AmazontoUPS msg) {
	// acquire lock
	std::lock_guard<std::mutex> guard(queueMutex); // automatically released when out of scope
	outgoing.push_back(msg);
}

// Retrieves outgoing messages generated before UPS connected to Amazon
int pollFromUPSMsgQueue(AmazontoUPS* holder) {
	std::lock_guard<std::mutex> guard(queueMutex); // automatically released when out of scope
	if (!outgoing.empty()) {
		*holder = outgoing.front();
		outgoing.pop_front();
		return 1;
	}
	return 0;
}


// Called whenever shipments are being packed at a warehouse with no truck in that warehouse
//int requestTruck(int whid, int * upsFD) {
//int requestTruck(int whid) {
int requestTruck(int whid, unsigned long shipid, int delX, int delY, bool hasPackages) {
	// construct message
	AmazontoUPS out;
	sendTruck truckRequest;
	sendTruck * holder = out.add_send_truck();
	// Get truck id of truck in warehouse, if -1 then need to request truck
	//std::cout << ""; 
	//if (shipid >= 0) {
	if (hasPackages) {
		//std::cout << "Shipid is valid; it is " << shipid << "\n";
		std::cout << "Adding package info!";
		pkgInfo info;
		info.set_packageid(shipid);
		info.set_delx(delX);
		info.set_dely(delY);
		pkgInfo * infoHolder = truckRequest.add_packages();
		unsigned long upsAcc = getUPSAccount(shipid);
		if (upsAcc < 0) {
			std::cout << "UPS Account missing, not sending upsAccount to UPS";
		}
		else {
			info.set_upsaccount(upsAcc);
			std::cout << "Set UPSAccount to be " << upsAcc << "\n";
		}
		*infoHolder = info;
	}
	else {
		std::cout << "IGNORING INVALID PLACEHOLDER PKG INFO\n";
	}


	int truckid;
//	if ((truckid = getTruckIDForWarehouse(whid)) < 0) {
//		std::cout << "No trucks in this warehouse, requesting truck from UPS\n";
		truckRequest.set_whid(whid);
		*holder = truckRequest;
		//if (*upsFD < 0) { // UPS not yet connected to Amazon, enqueue
		std::string string;
		google::protobuf::TextFormat::PrintToString(out, &string);
		std::cout << "Outgoing message: " << string << "\n";
		if (UPS_FD < 0) {
			std::cout << "UPS not yet connected, adding sendTruck message to queue!\n";
			addToUPSMsgQueue(out);
			return 0;
		}
		else {
			//if (!sendMsgToSocket(out, *upsFD)) {
			if (!sendMsgToSocket(out, UPS_FD)) {
				std::cout << "unable to request truck\n";
				return -1;
			}
			else {
				std::cout << "Successfully requested truck!\n";
				return 1;
			}
		}
//	}
//	else {
//		std::cout << "This warehouse already has a truck of id " << truckid << "\n";
//		return 0;
//	}
}

// Shipment info can be retrieved from DB using truck_id and fsm_state of LOADED
//int requestDispatch(int whid, int * upsFD) { 
int requestDispatch(int whid) { // ONLY CALLED WHEN ENSURED THAT WAREHOUSE IS READY FOR DISPATCH
	// construct message
	AmazontoUPS out;
	dispatchTruck requestDispatch;
	dispatchTruck * holder = out.add_dispatch_truck();

	// Retrieve truckid and list of all shipments (with info) that are loaded on a truck for this warehouse
	// ASSUMPTION : MAX OF ONLY 1 TRUCKID @ A WAREHOUSE AT A GIVEN TIME
	/*
	required int64 packageid = 1;
	required int32 delX = 2;
	required int32 delY = 3;
	*/
	std::tuple<int, std::vector<std::tuple<unsigned long, int, int>>> packagesToDeliver = getLoadedShipmentsInfoForWarehouse(whid);
	int truckid = std::get<0>(packagesToDeliver);
	std::vector<std::tuple<unsigned long, int, int>> packages = std::get<1>(packagesToDeliver);
	if (truckid < 0) {
		std::cout << "Invalid truckid, unable to request dispatch!\n";
		return -1;
	}
	requestDispatch.set_truckid(truckid);
	unsigned long packageid;
	int delX;
	int delY;
	pkgInfo packageInfo;
	pkgInfo* pkgHolder;
	std::tuple<unsigned long, int, int> packageTup;
	for (std::vector<std::tuple<unsigned long, int, int>>::iterator it = packages.begin(); it < packages.end(); it ++) {
		packageTup = *it;
		packageid = std::get<0>(packageTup);
		delX = std::get<1>(packageTup);
		delY = std::get<2>(packageTup);
		packageInfo.set_packageid(packageid);
		packageInfo.set_delx(delX);
		packageInfo.set_dely(delY);
		pkgHolder = requestDispatch.add_packages();
		*pkgHolder = packageInfo;
	}
	std::cout << "Constructed dispatchTruck message with " << requestDispatch.packages_size() << " packages!\n";
	*holder = requestDispatch;
	//if (*upsFD < 0) { 
	if (UPS_FD < 0) { // UPS not yet connected to Amazon, enqueue
		std::cout << "UPS not yet connected, adding dispatchTruck message to queue!\n";
		addToUPSMsgQueue(out);
		return 0;
	}
	else {
		//if (!sendMsgToSocket(out, *upsFD)) {
		if (!sendMsgToSocket(out, UPS_FD)) {
			std::cout << "unable to request truck\n";
			return -1;
		}
		else {
			std::cout << "Successfully requested truck!\n";
			return 1;
		}
	}
}


int handleUConnectedToSim(AmazontoUPS* out, UPStoAmazon* in, int* result) {
	if (in->uconnected_size() == 0) {
		return 0;
	}
	// Connected, just return ack that we are also connected?
	AConnectedToSim conn;
	conn.set_worldid(WORLD_ID);
	AConnectedToSim* outHolder = out->add_aconnected();
	*outHolder = conn;
	return 1;
}

int handleTruckArrived(AmazontoUPS* out, UPStoAmazon* in, int* result, int sim_sock) { // may need to talk to sim here for APutOnTruck request
	// Iterate through 'arrived' messages, extract whid and truckid for each of them, set truckid of corresponding warehouse
	if (in->truck_arrived_size() == 0) {
		return 0;
	}
	truckArrived truckArr;
	int whid;
	int truckid;
	for (int arrival = 0; arrival < in->truck_arrived_size(); arrival ++) {
		truckArr = in->truck_arrived(arrival);
		whid = truckArr.whid();
		truckid = truckArr.truckid();
		std::cout << "Truck " << truckid << " has arrived at warehouse " << whid << "\n";
		std::vector<std::tuple<int, int, unsigned long>> loadInfo = setTruckForWarehouse(whid, truckid); // also transitions fsm_state for all shipments in the warehouse which were waiting on truck arrival
		if (loadInfo.size() > 0) {
			std::cout << "Some packages at warehouse " << whid << " are already packed and ready for loading, upsMessenger making load request to sim!\n";
			if (!makeLoadRequest(loadInfo, sim_sock)) { 
				std::cout << "Error making load request for ready packages upon truck arrival!\n";
			}
			else { // load request sent to sim successfully
				std::cout << "Successfully made load request for ready packages upon truck arrival!\n";
			}
		}
		else {
			std::cout << "No ready packages to load upon truck arrival!\n";
		}
	}
	return in->truck_arrived_size();
}


int handlePackageDelivered(AmazontoUPS* out, UPStoAmazon* in, int* result) { // Just need to incrementShipmentState or set state to DELIVERED
	if (in->delivered_size() == 0) {
		return 0;
	}
	// Iterate through 'delivered' messages, extract packageid for each of them, increment shipment state for them
	unsigned long shipid;
	packageDelivered pkgDel;
	//int errors = 0;
	for (int delivery = 0; delivery < in->delivered_size(); delivery ++) {
		pkgDel = in->delivered(delivery);
		shipid = pkgDel.packageid();
		if (incrementShipmentState(shipid) < 0) {
			std::cout << "Error updating fsm_state of shipid " << shipid << "!\n";
			//errors ++;
		} 
		std::cout << "SUCCESSFULLY DELIVERED PACKAGE " << shipid << "!!!\n";
	}
	//return 0;
	return in->delivered_size();
}

// Could be UConnectedToSim, truckArrived or packageDelivered
AmazontoUPS handleUPSMessage(UPStoAmazon* in, int* result, int sim_sock) {
	AmazontoUPS out;
	int total = 0;
	total += handleUConnectedToSim(&out, in, result);
	total += handleTruckArrived(&out, in, result, sim_sock);
	total += handlePackageDelivered(&out, in, result);
	if (total == 0) { // Nothing happened, don't write back
		*result = 0; // failure
	}
	return out;
}

//int listenToUPS(int * upsFD) {
int listenToUPS(int sim_sock) {
	int    i, len, rc, on = 1;
   int    listen_sd, max_sd, new_sd;
   int    desc_ready = 0;//, end_server = 0;
   int    close_conn;
   char   buffer[80];
   struct sockaddr_in   addr;
   //struct timeval       timeout;
   struct fd_set        master_set, reading_set;//, writing_set;
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
   addr.sin_port        = htons(UPS_COMM_PORT);
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

   std::cout << "UPS comm server about to enter sleep loop\n";
   std::deque<AmazontoUPS> outgoing;
   do {
   	memcpy(&reading_set, &master_set, sizeof(master_set));
   	//memcpy(&writing_set, &master_set, sizeof(master_set));

   	desc_ready = 0;
   	rc = select(max_sd + 1, &reading_set, NULL, NULL, NULL);
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
	                  //*upsFD = new_sd; 
	                  UPS_FD = new_sd; // should synchronize?
	                  if (new_sd > max_sd) {
	                     max_sd = new_sd;	                  	
	                  }

	                  // Write any messages accumulated so far
	                  AmazontoUPS outSoFar;
	                  int numOutSoFar;
	                  if (!(numOutSoFar = pollFromUPSMsgQueue(&outSoFar))) { // no messages accumulated so far
	                  	std::cout << "No messages accumulated from before connection to UPS\n";
	                  }
	                  else {
	                  	do {
	                  		if (!sendMsgToSocket(outSoFar, i)) {
	                  			std::cout << "Unable to send accumulated message to UPS!\n";
	                  			break;
	                  		}
	                  		else {
	                  			std::cout << "Wrote an enqueued message to UPS!\n";
	                  		}
	                  	}
	                  	while ((numOutSoFar = pollFromUPSMsgQueue(&outSoFar))); // write out each messag
	                  	std::cout << "No more accumulated messages to write!\n";
	                  }
	                  /* Loop back up and accept another incoming   */
	                  /* connection                                 */
	               } while (new_sd != -1);
   				}

   				else { // Not listen fd - perform read
   					//int numRead;
                	UPStoAmazon msg; // Message received from UPS
                	//if (!recvMsgFromSocket(*((google::protobuf::Message*)&order), i)) {
                	if (!recvMsgFromSocket(msg, i)) {
                	  
                     std::cout << "Unable to read UPS proto message\n";
                     continue;

                  	}
                  	else {
                    	int success = 1;
                    	AmazontoUPS out = handleUPSMessage(&msg, &success, sim_sock);
                    	if (!success) {
                    		std::cout << "Nothing to do for this message from UPS!\n";
                    	}
                    	else { // write message
                    		if (!sendMsgToSocket(out, i)) {
                    			std::cout << "Error writing outgoing message to socket!\n";
                    		}
                    		else {
                    			/*
                    			std::cout << "Successfully wrote message to UPS!\n";
                    			// OK TO CLOSE?
                    			//close(*upsFD);
                    			close(UPS_FD);
                    			//FD_CLR(*upsFD, &master_set);
                    			FD_CLR(UPS_FD, &master_set);
                    			*/
                    		}
                    	}
                  	}
   				}
   			}
   		}
   	}
   } while (1);
   //if (*upsFD >= 0) {
   if (UPS_FD >= 0) {
   	//close(*upsFD);
   	close(UPS_FD);
   }
   return 0;
}

//std::thread * launchUPSMessenger(int * upsFD) {
std::thread * launchUPSMessenger(int sim_sock) {
	//std::thread * upsMessenger = new std::thread(listenToUPS, upsFD);
	std::thread * upsMessenger = new std::thread(listenToUPS, sim_sock);
	return upsMessenger;
}
