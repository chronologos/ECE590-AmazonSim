#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <tuple>
#include <string>


std::vector<std::tuple<unsigned long, std::string, int>> getShipmentProducts(unsigned long shipid);
int getShipmentState(unsigned long shipid);
int initShipmentState(unsigned long shipid);
int setShipmentState(unsigned long shipid, int state); // To handle branch from packing to (packed, truck not arrived) and (not packed, truck arrived)
int incrementShipmentState(unsigned long shipid);
int getInventory(unsigned long productid);
int updateInventory(unsigned long productid, int toAdd); 
//int updateInventory(unsigned long shipid);
