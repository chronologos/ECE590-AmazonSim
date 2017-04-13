#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <iostream>
#include <sys/socket.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void WriteBytes(int socket, std::string str);
void ReadXBytes(int socket, uint64_t x, void* buffer);

#endif
