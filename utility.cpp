#include "utility.hpp"

void ReadXBytes(int socket, uint64_t x, void* buffer)
{
  uint64_t bytesRead = 0;
  uint64_t result;
  while (bytesRead < x)
  {
    result = read(socket, buffer + bytesRead, x - bytesRead);
    if (result < 1 )
    {
      std::cerr << "socket error" << std::endl;
    }
    else{
      bytesRead += result;
    }
  }
  // std::cout << bytesRead << std::endl;
}

void WriteBytes(int socket, std::string str) {
  uint64_t bytesToWrite = str.size();
  char *buffer = new char[bytesToWrite + 8];
  *(uint64_t*)buffer = htobe64(bytesToWrite);
  strcpy(buffer+8, str.c_str());
  bytesToWrite += 8;
  uint64_t bytesWritten = 0;
  uint64_t result;

  while (bytesToWrite > bytesWritten){
    result = send(socket, buffer + bytesWritten, bytesToWrite- bytesWritten, 0);
    if (result < 1 )
    {
      std::cerr << "socket error" << std::endl;
    }
    else{
      bytesWritten += result;
    }
  }
}
