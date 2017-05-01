#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <tuple>
#include <string>
#include "./utility.hpp"

std::vector<std::tuple<unsigned long, std::string, int>> getShipmentProducts(unsigned long shipid);
int getShipmentState(unsigned long shipid);
int initShipmentState(unsigned long shipid);
int setShipmentState(unsigned long shipid, int state); // To handle branch from packing to (packed, truck not arrived) and (not packed, truck arrived)
int incrementShipmentState(unsigned long shipid);
int getInventory(unsigned long productid);
int updateInventory(unsigned long productid, int toAdd); 

int getTruckIDForWarehouse(int whid);

pqxx::result getLoadedShipmentsForWarehouse(int whid);
std::tuple<int, std::vector<std::tuple<unsigned long, int, int>>> getLoadedShipmentsInfoForWarehouse(int whid); 
bool hasPackagesAwaitingLoad(int whid); 
int readyForDispatch(unsigned long shipid, int * mustSendTruck); // return whid if ready, else -1
//int updateInventory(unsigned long shipid);
int setDispatched(std::vector<unsigned long> dispatchedShips);
std::vector<std::tuple<int, int, unsigned long>> setTruckForWarehouse(int whid, int truckid);
std::tuple<int, int, unsigned long> setReady(unsigned long shipid);
