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
  txn.commit();
  //std::cout << "Txn committed!\n";
  return products;
}

/*
int main() {
	std::vector<std::tuple<unsigned long, std::string, int>> posResults = getShipmentProducts(1324);
	std::cout << "No. of results for existing key: " << posResults.size() << "\n";
	std::vector<std::tuple<unsigned long, std::string, int>> negResults = getShipmentProducts(234234);
	std::cout << "No. of results for non-existent key: " << negResults.size() << "\n";
	return 0;
}
*/
