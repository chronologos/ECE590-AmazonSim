#include "./dbHandler.h"

std::vector<std::tuple<unsigned long, std::string, int>> getShipmentProducts(unsigned long shipid) {
  //pqxx::connection c{"dbname=amazonsim user=radithya"};
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  std::string queryString("SELECT * FROM store_shipmentitem WHERE tracking_number_id =" + std::to_string(shipid));
  //std::cout << "Query String: " << queryString << "\n";
  pqxx::result r = txn.exec(queryString);
  
  
  std::vector<std::tuple<unsigned long, std::string, int>> products;
  //std::cout << "About to present rows of table\n";
  if (r.size() == 0) {
    std::cout << "Null result for lookup of shipment id!\n";
    //return r;
  	return products;
  }
  std::cout << "Found " << r.size() << " products for ship_id " << shipid << "!\n";
  unsigned long product_id;
  std::string description;
  int count;
  for (int num = 0; num < r.size(); num ++) {
  	if (r[num]["product_id"].is_null() || !r[num]["product_id"].to(product_id)) {
  		std::cout << "Unable to retrieve product id of product into unsigned long, error in DB!\n";
  		// skip this product
  		continue;
  	}
  	if (r[num]["description"].is_null() || !r[num]["description"].to(description)) {
  		std::cout << "Unable to retrieve description of product into string, error in DB!\n";
  		// skip this product
  		continue;
  	}
  	if (r[num]["count"].is_null() || !r[num]["count"].to(count)) {
  		std::cout << "Unable to retrieve count of product into int, error in DB!\n";
  		// skip this product
  		continue;
  	}
  	std::tuple<unsigned long, std::string, int> purchase(product_id, description, count);
  	products.push_back(purchase);
  }
  return products;
}


// Retrieve FSM state of shipment from database
int getShipmentState(unsigned long shipid) {
	//pqxx::connection c{"dbname=amazonsim user=radithya"};
    pqxx::connection c{"dbname=amazon_db user=postgres"};
  	pqxx::work txn{c};
  	std::string queryString("SELECT fsm_state FROM store_trackingnumber WHERE id = " + std::to_string(shipid));
  	//std::cout << "Query String: " << queryString << "\n";
  	pqxx::result r = txn.exec(queryString);
  	if (r.size() == 0) {
    std::cout << "Null result for lookup of shipment id!\n";
    //return r;
  	return -1;
  }
  if (r.size() > 1) {
  	std::cout << "Error! Found duplicate entries for shipid " << shipid << "; total of " << r.size() << " items!\n";
  }
  int fsm_state;
  if (r[0]["fsm_state"].is_null() || !r[0]["fsm_state"].to(fsm_state)) {
  	std::cout << "Error! FSM State column for shipid " << shipid << " either missing or malformatted!\n";
  	return -1; 
  }
  std::cout << "Retrieved FSM state for shipid " << shipid << " to be " << fsm_state << "\n";
  return fsm_state;
  //std::cout << "Found " << r.size() << " products for ship_id " << shipid << "!\n";
}

// Seperate from below method for error detection
int initShipmentState(unsigned long shipid) { // TO-DO : Catch exceptions?
	//pqxx::connection c{"dbname=amazonsim user=radithya"};
    pqxx::connection c{"dbname=amazon_db user=postgres"};
  	pqxx::work txn{c};
  	std::string queryString("INSERT INTO store_trackingnumber (id, fsm_state) VALUES (" + std::to_string(shipid) + ", " + std::to_string(1) + ")");
  	
    try {
      pqxx::result r = txn.exec(queryString);
    	txn.commit();
    	std::cout << "Succesfully inserted shipid " << shipid << " into trackingnumbers table\n";
  	 return 0; 
    }
    catch (const pqxx::pqxx_exception &e) { // Most likely a retry by Django client on same shipid
      std::cout << "DB ERROR trying to insert ship_id into trackingnumbers table:\n";
      std::cerr << e.base().what();
      return -1;
    }
}

int setShipmentState(unsigned long shipid, int state) {
  //pqxx::connection c{"dbname=amazonsim user=radithya"};
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  std::string queryString("UPDATE store_trackingnumber\nSET fsm_state=" + std::to_string(state) + " WHERE id=" + std::to_string(shipid));
  pqxx::result r = txn.exec(queryString);
  txn.commit();
  std::cout << "Succesfully updated fsm_state of shipid " << shipid << " to " << state << "\n";
  return state;
}


// Increment FSM state of shipment in database
int incrementShipmentState(unsigned long shipid) {
	//pqxx::connection c{"dbname=amazonsim user=radithya"};
    pqxx::connection c{"dbname=amazon_db user=postgres"};
  	pqxx::work txn{c};
  	int currentState = getShipmentState(shipid);
  	if (currentState < 0) {
  		std::cout << "Unable to retrieve fsm state for shipid " << shipid << "; aborting increment!\n";
  		return -1;
  	}
  	int newState = currentState + 1;
  	std::string queryString("UPDATE store_trackingnumber\nSET fsm_state=" + std::to_string(newState) + " WHERE id=" + std::to_string(shipid));
  	pqxx::result r = txn.exec(queryString);
  	txn.commit();
  	std::cout << "Succesfully updated fsm_state of shipid " << shipid << " to " << newState << "\n";
  	return newState;
}

// Retrieve current inventory of a product
int getInventory(unsigned long productid, int whid) {
	//pqxx::connection c{"dbname=amazonsim user=radithya"};
    pqxx::connection c{"dbname=amazon_db user=postgres"};
  	pqxx::work txn{c};
  	std::string queryString("SELECT count FROM store_inventory WHERE product_id=" + std::to_string(productid) + " AND warehouse_id=" + std::to_string(whid));
  	pqxx::result r = txn.exec(queryString);
  	if (r.size() == 0) {
  		std::cout << "Product with id " << productid << " not found in inventory!\n";
  		return -1;
  	}
  	
    if (r.size() > 1) { // Each productid should appear only once in the table
  		//std::cout << "ERROR! Product with id " << productid << " appears multiple times in inventory!\n";
  		return -1;
  	}
    
  	int count;
  	if (r[0]["count"].is_null() || !r[0]["count"].to(count)) {
  		std::cout << "Error getting inventory of product with id " << productid << "; unable to get count as integer\n";
  		return -1;
  	}
  	return count;
}


// Add amount toAdd to current amount of item in DB, update DB, return updated amount
int updateInventory(unsigned long productid, int toAdd, int whid) {
  int oldAmount = getInventory(productid, whid);
  if (oldAmount < 0) {
    std::cout << "Missing item with productid " << productid << " detected while trying to update inventory; skipping\n"; 
    return -1;
  }
  int newAmount = oldAmount + toAdd;
  //pqxx::connection c{"dbname=amazonsim user=radithya"};
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  //std::string queryString("INSERT INTO store_inventory (product_id, count) VALUES ("  + std::to_string(productid) + ", " + std::to_string(newAmount) + ") WHERE warehouse_id=" + std::to_string(whid));
  std::string queryString("UPDATE store_inventory\nSET count=" + std::to_string(newAmount) + "\nWHERE warehouse_id=" + std::to_string(whid) + " AND product_id=" + std::to_string(productid));
  pqxx::result r = txn.exec(queryString);
  txn.commit();
  std::cout << "Updated amount of product_id " << productid << " to " << newAmount << "\n";
  return newAmount;
}

// Retrieve the truckid for a given warehouse id
int getTruckIDForWarehouse(int whid) {
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  std::string queryString("SELECT truck_id FROM store_warehouse WHERE id=" + std::to_string(whid));
  pqxx::result r = txn.exec(queryString);
  //txn.commit();
  if (r.size() == 0) {
    std::cout << "Error! Non-existent warehouse of id " << whid << "\n";
    return -1;
  }
  int truckid;
  if (!r[0]["truck_id"].to(truckid)) {
    std::cout << "Error! Unable to cast truckid to integer\n";
    return -1;
  }
  return truckid;
}


pqxx::result getLoadedShipmentsForWarehouse(int whid) {
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  std::string queryString("SELECT id, x_address, y_address FROM store_trackingnumber WHERE warehouse_id= " + std::to_string(whid) + " AND fsm_state= " + std::to_string(WAITING_TRUCK_DISPATCH));
  pqxx::result r = txn.exec(queryString);
  if (r.size() == 0) {
    std::cout << "No shipments loaded and ready for dispatch in this warehouse!\n";
    return r;
  }
  std::cout << r.size() << " shipments ready for dispatch!\n";
  return r;
}

// Retrieve truckID and all info of all loaded shipments for a given warehouse
std::tuple<int, std::vector<std::tuple<unsigned long, int, int>>> getLoadedShipmentsInfoForWarehouse(int whid) {
  // First retrieve truckid
  int truckid = getTruckIDForWarehouse(whid);
  std::vector<std::tuple<unsigned long, int, int>> loadedShipments;
  if (truckid < 0) {
    std::cout << "No truck at this warehouse, no packages could have been loaded; not ready for dispatch\n";
    std::tuple<int, std::vector<std::tuple<unsigned long, int, int>>> result(truckid, loadedShipments);
    return result;
  }
  std::cout << "About to call getLoadedShipmentsForWarehouse\n";
  pqxx::result r = getLoadedShipmentsForWarehouse(whid);
  unsigned long shipid;
  int delX;
  int delY;
  //std::tuple<unsigned long, int, int> shipInfo;
  std::cout << "About to extract shipment info in getLoadedShipmentsForWarehouse\n";
  try {
    for (int shipment = 0; shipment < r.size(); shipment ++) {
      if (!r[shipment]["id"].to(shipid)) {
      //if (!r[0][0].to(shipid)) { 
        std::cout << "Error! unable to cast shipid to unsigned long!\n";
        continue; // Fail silently on this shipment?
      }
      std::cout << "Extracted and cast shipid to be " << shipid << "\n";
      if (!r[shipment]["x_address"].to(delX)) {
        std::cout << "Error! Unable to cast delivery x-coordinate to int!\n";
        continue;
      }
      if (!r[shipment]["y_address"].to(delY)) {
        std::cout << "Error! Unable to cast delivery y-coordinate to int!\n";
        continue;
      }
      std::tuple<unsigned long, int, int> shipInfo(shipid, delX, delY);
      loadedShipments.push_back(shipInfo);
    }
  }
  catch (const pqxx::pqxx_exception &e)
  {
    std::cerr << e.base().what() << std::endl;
    const pqxx::sql_error *s=dynamic_cast<const pqxx::sql_error*>(&e.base());
    if (s) std::cerr << "Crashed on query: " << s->query() << std::endl;
  }
  std::tuple<int, std::vector<std::tuple<unsigned long, int, int>>> result(truckid, loadedShipments);
  return result;
}

// Returns true if warehouse whid has any packages in states WAITING_READY_TRUCK, WAITING_TRUCK, WAITING_READY or WAITING_LOAD
bool hasPackagesAwaitingLoad(int whid) {
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  std::string queryString("SELECT id FROM store_trackingnumber WHERE warehouse_id= " + std::to_string(whid) + " AND (fsm_state=" + std::to_string(WAITING_READY_TRUCK) + " OR fsm_state=" + std::to_string(WAITING_TRUCK) + " OR fsm_state=" + std::to_string(WAITING_READY) + " OR fsm_state=" + std::to_string(WAITING_LOAD) + ")");  
  pqxx::result r = txn.exec(queryString);
  if (r.size() == 0) {
    std::cout << "No packages awaiting load for this warehouse!\n";
    return false;
  }
  std::cout << "This warehouse has " << r.size() << " packages awaiting load!\n";
  txn.commit();
  return true;
}


int readyForDispatch(unsigned long shipid, int * mustSendTruck) {
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  // First retrieve warehouseid
  std::string queryString1("SELECT warehouse_id FROM store_trackingnumber WHERE id=" + std::to_string(shipid));
  pqxx::result r = txn.exec(queryString1);
  if (r.size() == 0) {
    std::cout << "Error! Missing warehouse id for shipment " << shipid << "\n";
    return -1; // can't dispatch if don't know which warehouse and hence which truck
  }
  int whid;
  if (!r[0]["warehouse_id"].to(whid)) {
    std::cout << "Error! Unable to cast warehouse id to integer!";
    return -1;
  }
  if (hasPackagesAwaitingLoad(whid)) {
    std::cout << "Checking if truck has been filled to at least minimum load\n";
    int numLoaded = getLoadedShipmentsForWarehouse(whid).size();
    if (numLoaded >= MIN_TRUCK_LOAD) {
      std::cout << "Warehouse has packages awaiting load but truck has reached minimum load, dispatching truck!\n";
      // need to make a new sendTruck request for this warehouse and update the truck_id field when truckArrived is received
      *mustSendTruck = 1; // Caller can make sendTruck after ack to dispatchTruck request upon seeing this
      return whid;
    }
    std::cout << "Warehouse has packages awaiting load and truck has not reached minimum load, NOT dispatching truck!\n"; // don't need to resendTruck
    return -1;
  }
  std::cout << "This warehouse has no packages awaiting load, dispatching truck!\n";
  txn.commit();
  return whid;
}

int setDispatched(std::vector<unsigned long> dispatchedShips) {
  if (dispatchedShips.size() == 0) return 0;
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  std::string queryString("UPDATE store_trackingnumber\nSET fsm_state=" + std::to_string(DISPATCHED) + "\nWHERE id=");
  for (std::vector<unsigned long>::iterator it = dispatchedShips.begin(); it < dispatchedShips.end() - 1; it ++) {
    queryString += std::to_string(*it) + " OR id=";
  }
  queryString += std::to_string(dispatchedShips.back()) + "\n";
  std::cout << "Query string is " << queryString << "\n";
  try {
    pqxx::result r = txn.exec(queryString);
    std::cout << "Successfully updated " << dispatchedShips.size() << " shipids' fsm states to " << DISPATCHED << "!\n";
    return dispatchedShips.size();
  }
  catch (const pqxx::pqxx_exception &e) { // Most likely a retry by Django client on same shipid
      std::cout << "DB ERROR trying to update fsm_state of trackingnumbers to " << DISPATCHED << "\n";
      std::cerr << e.base().what();
      return -1;
    }
  txn.commit();
}

// TO-DO
// Called when truckid arrived at whid
std::vector<std::tuple<int, int, unsigned long>> setTruckForWarehouse(int whid, int truckid) { // Just overwrite? Since truckid not reset for dispatchTruck
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  // Update truckid of warehouse
  std::string queryString("UPDATE store_warehouse\nSET truck_id=" + std::to_string(truckid) + "\nWHERE id=" + std::to_string(whid));
  std::vector<std::tuple<int, int, unsigned long>> loadInfo;
  try { // Atomically update all or none
    pqxx::result r = txn.exec(queryString);
    // Update fsm_state and truck_id for all shipments with WAITING_READY_TRUCK to WAITING_READY
    queryString = "UPDATE store_trackingnumber\nSET fsm_state=" + std::to_string(WAITING_READY) + ", truck_id=" + std::to_string(truckid) + "\nWHERE fsm_state=" + std::to_string(WAITING_READY_TRUCK) + " AND warehouse_id=" + std::to_string(whid);
    r = txn.exec(queryString);
    // Update fsm_state and truck_id for all shipments with WAITING_TRUCK to WAITING_LOAD
    queryString = "UPDATE store_trackingnumber\nSET fsm_state=" + std::to_string(WAITING_LOAD) + ", truck_id=" + std::to_string(truckid) + "\nWHERE fsm_state=" + std::to_string(WAITING_TRUCK) + " AND warehouse_id=" + std::to_string(whid);
    //queryString = "UPDATE store_trackingnumber\nSET fsm_state=" + std::to_string(WAITING_LOAD) + "\nWHERE fsm_state=" + std::to_string(WAITING_TRUCK) + " AND warehouse_id=" + std::to_string(whid);
    r = txn.exec(queryString);
    //txn.commit();
    std::cout << "Successfully updated fsm states for shipments waiting for truck in warehouse\n";
    // NEED TO SEND LOAD REQUEST TO SIM FOR EACH OF THESE PACKAGES!
    //queryString = "SELECT id FROM store_trackingnumber WHERE fsm_state=" + std::to_string(WAITING_LOAD) + " AND warehouse_id=" + std::to_string(whid) + " AND truck_id=" + std::to_string(truckid);
    queryString = "SELECT id FROM store_trackingnumber WHERE fsm_state=" + std::to_string(WAITING_LOAD) + " AND warehouse_id=" + std::to_string(whid);
    r = txn.exec(queryString);
    if (r.size() == 0) {
      std::cout << "No packages to be loaded immediately for this warehouse.\n";
    }
    else {
      unsigned long shipid;
      std::cout << r.size() << " packages to be loaded from warehouse " << whid << " now!\n";
      for (int package = 0; package < r.size(); package ++) {
        if (!r[package]["id"].to(shipid)) {
          std::cout << "Error casting id of package to unsigned long!\n";
          continue;
        }
        std::cout << "Extracted and cast shipid to be " << shipid << "\n";
        std::cout << "Shipid for package about to be packed is " << shipid << "\n";
        loadInfo.push_back(std::tuple<int, int, unsigned long>(whid, truckid, shipid));
      }
    }
    txn.commit(); // Commit at the end
  }
  catch (const pqxx::pqxx_exception &e) {
    std::cout << "DB ERROR trying to update truck_id of warehouse whid " << whid << " to " << truckid << "\n";
    std::cerr << e.base().what();
    //return false;
    //return loadInfo;
  }
  //return true;
  return loadInfo;
}

int getTruckForShipment(unsigned long shipid) {
  // First retrieve whid of shipment
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  std::string queryString("SELECT warehouse_id FROM store_trackingnumber WHERE id=" + std::to_string(shipid));
  pqxx::result r = txn.exec(queryString);
  if (r.size() == 0) {
    std::cout << "Missing shipment " << shipid << " from database!\n";
    return -1;
  }
  int whid;
  if (!r[0]["warehouse_id"].to(whid)) {
    std::cout << "Error casting warehouse id to integer!\n";
    return -1;
  }
  std::cout << "Warehouse number of shipment " << shipid << " is found to be " << whid;
  return getTruckIDForWarehouse(whid);
}


std::tuple<int, int, unsigned long> setReady(unsigned long shipid) {
  pqxx::connection c{"dbname=amazon_db user=postgres"};
  pqxx::work txn{c};
  
  // check if truck is already at warehouse
  int truckForShipment = getTruckForShipment(shipid); // Update truckid of shipment before continuing
  std::string queryString("UPDATE store_trackingnumber\nSET truck_id=" + std::to_string(shipid) + " WHERE id=" + std::to_string(truckForShipment));
  pqxx::result r = txn.exec(queryString);
  int currentShipmentState = getShipmentState(shipid);
  if (truckForShipment != -1) { 
  //&& ((currentShipmentState == WAITING_READY_TRUCK) || (currentShipmentState == WAITING_TRUCK))) { // already has truck
    if (currentShipmentState == WAITING_READY_TRUCK) {
      setShipmentState(shipid, WAITING_READY);
      std::cout << "Updated state of shipment " << shipid << " to be truck arrived, waiting for ready\n";
    }
    else if (currentShipmentState == WAITING_TRUCK) { // shouldn't happen
      std::cout << "WARNING: WEIRD LOGIC CONDITION IN setReady!\n";
      setShipmentState(shipid, WAITING_LOAD);
    } 
    //incrementShipmentState()
  }
  // change fsm state from WAITING_READY_TRUCK to WAITING_TRUCK 
  queryString = ("UPDATE store_trackingnumber\nSET fsm_state=" + std::to_string(WAITING_TRUCK) + "\nWHERE id=" + std::to_string(shipid) + " AND fsm_state=" + std::to_string(WAITING_READY_TRUCK));
  r = txn.exec(queryString);
  std::tuple<int, int, unsigned long> shipInfo;
  shipInfo = std::tuple<int, int, unsigned long>(-1, -1, shipid);
  if (r.affected_rows() > 0) { // Not ready for loading
    std::cout << "Shipment " << shipid << " now packed but no truck yet, not ready for loading\n";
    txn.commit();
    return shipInfo;
  }
  // change fsm_state from WAITING_READY to WAITING_LOAD
  queryString = "UPDATE store_trackingnumber\nSET fsm_state=" + std::to_string(WAITING_LOAD) + "\nWHERE id=" + std::to_string(shipid) + " AND fsm_state=" + std::to_string(WAITING_READY);
  r = txn.exec(queryString);
  if (r.affected_rows() == 0) { // Either shipment does not exist or fsm is in wrong state
    std::cout << "ERROR! Package " << shipid << " either missing from trackingnumbers table or in wrong FSM State!\n";
    txn.commit();
    return shipInfo;
  }
  // Package ready for loading
  std::cout << "Package " << shipid << " ready for loading!\n";
  // Retrieve warehouse_id and truck_id for package
  queryString = "SELECT warehouse_id, truck_id FROM store_trackingnumber WHERE id=" + std::to_string(shipid);
  r = txn.exec(queryString);
  if (r.size() == 0) { // Shouldn't happen
    std::cout << "ERROR! Package " << shipid << " disappeared in the middle of setReady txn?\n";
    txn.commit();
    return shipInfo;
  }
  int whnum;
  int truckid;
  if (!r[0]["warehouse_id"].to(whnum)) {
    std::cout << "Error casting whnum of package " << shipid << " to integer!\n";
    txn.commit();
    return shipInfo;
  }
  if (!r[0]["truck_id"].to(truckid)) {
    std::cout << "Error casting truckid of package " << shipid << " to integer!\n";
    txn.commit();
    return shipInfo;
  }
  shipInfo = std::tuple<int, int, unsigned long>(whnum, truckid, shipid);
  std::cout << "Successfully extracted information for shipment " << shipid << " which is ready for loading!\n";
  txn.commit();
  return shipInfo;
}

unsigned long getUPSAccount(unsigned long shipid) {
   pqxx::connection c{"dbname=amazon_db user=postgres"};
   pqxx::work txn{c};
   std::string queryString("SELECT ups_num FROM store_trackingnumber WHERE id=" + std::to_string(shipid));
   pqxx::result r = txn.exec(queryString);
   if (r.size() == 0) {
    std::cout << "Error! Missing package " << shipid << "\n";
    return -1;
   }
   unsigned long upsAcc;
   if (!r[0]["ups_num"].to(upsAcc)) {
    std::cout << "Error! Unable to cast ups acccount to unsigned long!\n";
    return -1;
   }
   std::cout << "Extracted and cast UPSAccount to be " << upsAcc << "\n";
   return upsAcc;
}
