#include "utility.hpp"
//#include <google/protobuf/io/coded_stream.h>
//#include <google/protobuf/io/zero_copy_stream_impl.h>

/*
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
*/


//this is adpated from code that a google engineer posted online                                  
template<typename T> bool sendMesgTo(const T & message,
                                     google::protobuf::io::FileOutputStream *out) {
  {//extra scope: make output go away before out->Flush()                                         
    // We create a new coded stream for each message.  Don't worry, this is fast.                 
    google::protobuf::io::CodedOutputStream output(out);
    // Write the size.                                                                            
    const int size = message.ByteSize();
    output.WriteVarint32(size);
    uint8_t* buffer = output.GetDirectBufferForNBytesAndAdvance(size);
    if (buffer != NULL) {
      // Optimization:  The message fits in one buffer, so use the faster                         
      // direct-to-array serialization path.                                                      
      message.SerializeWithCachedSizesToArray(buffer);
    } else {
      // Slightly-slower path when the message is multiple buffers.                               
      message.SerializeWithCachedSizes(&output);
      if (output.HadError()) {
        return false;
      }
    }
  }
  //std::cout << "flushing...\n";                                                                 
  out->Flush();
  return true;
}


template<typename T> bool recvMesgFrom(T & message,
                                       google::protobuf::io::FileInputStream * in ){
  // We create a new coded stream for each message.  Don't worry, this is fast,                   
  // and it makes sure the 64MB total size limit is imposed per-message rather                    
  // than on the whole stream.  (See the CodedInputStream interface for more                      
  // info on this limit.)                                                                         
  google::protobuf::io::CodedInputStream input(in);
  // Read the size.                                                                               
  uint32_t size;
  if (!input.ReadVarint32(&size)) {
    return false;
  }
  // Tell the stream not to read beyond that size.                                                
  ::google::protobuf::io::CodedInputStream::Limit limit = input.PushLimit(size);
  // Parse the message.                                                                           
  if (!message.MergeFromCodedStream(&input)) {
    return false;
  }
  if (!input.ConsumedEntireMessage()) {
    return false;
  }
  // Release the limit.                                                                           
  input.PopLimit(limit);
  return true;
}


//template<typename T> bool sendMsgToSocket(const T & message, int fd) {
bool sendMsgToSocket(const ::google::protobuf::Message & message, int fd) {
  google::protobuf::io::FileOutputStream fs(fd);
  return sendMesgTo(message, &fs);
}

//template<typename T> bool recvMsgFromSocket(const T & message, int fd) {
bool recvMsgFromSocket(google::protobuf::Message & message, int fd) {
  google::protobuf::io::FileInputStream fs(fd);
  return recvMesgFrom(message, &fs);
}


