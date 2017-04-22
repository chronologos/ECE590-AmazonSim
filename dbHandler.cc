#include "./dbHandler.h"

std::vector<std::tuple<unsigned long, std::string, int>> getShipmentProducts(unsigned long shipid) {
  pqxx::connection c{"dbname=amazonsim user=radithya"};
  pqxx::work txn{c};
  std::string queryString("SELECT * FROM shipmentitems WHERE ship_id = " + std::to_string(shipid));
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
  //std::cout << "About to commit txn!\n";
  // Not needed for SELECT queries, but just a practice
  //txn.commit();
  //std::cout << "Txn committed!\n";
  return products;
}


// Retrieve FSM state of shipment from database
int getShipmentState(unsigned long shipid) {
	pqxx::connection c{"dbname=amazonsim user=radithya"};
  	pqxx::work txn{c};
  	std::string queryString("SELECT fsm_sate FROM trackingnumbers WHERE ship_id = " + std::to_string(shipid));
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
	pqxx::connection c{"dbname=amazonsim user=radithya"};
  	pqxx::work txn{c};
  	std::string queryString("INSERT INTO trackingnumbers (ship_id, fsm_state) VALUES (" + std::to_string(shipid) + ", " + std::to_string(1) + ")");
  	
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

// Increment FSM state of shipment in database
int incrementShipmentState(unsigned long shipid) {
	pqxx::connection c{"dbname=amazonsim user=radithya"};
  	pqxx::work txn{c};
  	int currentState = getShipmentState(shipid);
  	if (currentState < 0) {
  		std::cout << "Unable to retrieve fsm state for shipid " << shipid << "; aborting increment!\n";
  		return -1;
  	}
  	int newState = currentState + 1;
  	std::string queryString("UPDATE trackingnumbers\nSET fsm_state=" + std::to_string(newState) + " WHERE ship_id=" + std::to_string(shipid));
  	pqxx::result r = txn.exec(queryString);
  	txn.commit();
  	std::cout << "Succesfully updated fsm_state of shipid " << shipid << " to " << newState << "\n";
  	return newState;
}

// Retrieve current inventory of a product
int getInventory(unsigned long productid) {
	pqxx::connection c{"dbname=amazonsim user=radithya"};
  	pqxx::work txn{c};
  	std::string queryString("SELECT count FROM inventory WHERE product_id=" + std::to_string(productid));
  	pqxx::result r = txn.exec(queryString);
  	if (r.size() == 0) {
  		std::cout << "Product with id " << productid << " not found in inventory!\n";
  		return -1;
  	}
  	if (r.size() > 1) { // Each productid should appear only once in the table
  		std::cout << "ERROR! Product with id " << productid << " appears multiple times in inventory!\n";
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
int updateInventory(unsigned long productid, int toAdd) {
  int oldAmount = getInventory(productid);
  if (oldAmount < 0) {
    std::cout << "Missing item with productid " << productid << " detected while trying to update inventory; skipping\n"; 
    return -1;
  }
  int newAmount = oldAmount + toAdd;
  pqxx::connection c{"dbname=amazonsim user=radithya"};
  pqxx::work txn{c};
  std::string queryString("INSERT INTO inventory (product_id, count, warehouse) VALUES ("  + std::to_string(productid) + ", " + std::to_string(newAmount) + ")");
  pqxx::result r = txn.exec(queryString);
  txn.commit();
  std::cout << "Updated amount of product_id " << productid << " to " << newAmount << "\n";
  return newAmount;
}


/*
int updateInventory(unsigned long shipid) {
	std::vector<std::tuple<unsigned long, std::string, int>> productsBought = getShipmentProducts(shipid);
	if (productsBought.size() == 0) {
		std::cout << "No products associated with shipid " << shipid << "; not updating inventory\n";
		return 0;
	}
	pqxx::connection c{"dbname=amazonsim user=radithya"};
  	pqxx::work txn{c};
  	std::string queryString("INSERT INTO inventory (product_id, count, warehouse) VALUES\n");
  	unsigned long productid;
  	int count;
  	int oldAmount;
  	std::tuple<unsigned long, std::string, int> product;
  	for (std::vector<std::tuple<unsigned long, std::string, int>>::iterator it = productsBought.begin(); it < productsBought.end() - 1; it ++) {
  		
      // incrementing the inventory of a single product
      product = *it;
  		productid = std::get<0>(product);
  		count = std::get<2>(product);
  		oldAmount = getInventory(productid);
  		if (oldAmount < 0) {
  			std::cout << "Missing item with productid " << productid << " detected while trying to update inventory; skipping\n"; // TBD - Should abort whole update or just skip this product?
  			//continue;
  			return -1;
  		}
  		queryString += "(" + std::to_string(productid) + ", " + std::to_string(oldAmount + count) + "),\n"; // Should bother checking for overflow?
  	}
  	product = *productsBought.end();
	productid = std::get<0>(product);
	count = std::get<2>(product);
	oldAmount = getInventory(productid);
	if (oldAmount < 0) {
		std::cout << "Missing item with productid " << productid << " detected while trying to update inventory; skipping\n"; // TBD - Should abort whole updatef instead?
		return -1;
	}
  	queryString += "(" + std::to_string(productid) + ", " + std::to_string(oldAmount + count) + ")\n";
  	std::cout << "Query string for update: " << queryString << "\n";
  	pqxx::result r = txn.exec(queryString);
  	txn.commit();
  	std::cout << "Succesfully updated inventory for shipid " << shipid << "\n";
  	return 0;
}
*/

/*
int main() {
	std::vector<std::tuple<unsigned long, std::string, int>> posResults = getShipmentProducts(1324);
	std::cout << "No. of results for existing key: " << posResults.size() << "\n";
	std::vector<std::tuple<unsigned long, std::string, int>> negResults = getShipmentProducts(234234);
	std::cout << "No. of results for non-existent key: " << negResults.size() << "\n";
	return 0;
}
*/
