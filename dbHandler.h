#include <iostream>
#include <pqxx/pqxx>
#include <vector>
#include <tuple>
#include <string>


std::vector<std::tuple<unsigned long, std::string, int>> getShipmentProducts(unsigned long shipid);
