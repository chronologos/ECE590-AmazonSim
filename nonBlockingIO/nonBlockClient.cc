#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <list>
#include <sstream>
#include <stdexcept>


/* ADAPTED FROM IBM Knowledge Center - "https://www.ibm.com/support/knowledgecenter/en/ssw_i5_54/rzab6/xnonblock.htm" */

#define SERVER_PORT  12345

#define TRUE             1
#define FALSE            0

using namespace std;
int main (int argc, char *argv[])
{
   int    i, len, rc, on = 1;
   int sockfd;
   int    close_conn;
   char   buffer[80];
   const char * testMessage = "booyakasha!\n\0";
   //struct sockaddr_in   addr;
   struct timeval timeout;
   struct fd_set master_set, read_set, write_set;

   struct addrinfo host_info;
   struct addrinfo *host_info_list;
   const char *hostname = "127.0.0.1";
   const char *port     = "12345";

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  int status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    std::cerr << "Error: cannot get address info for host" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } //if

  sockfd = socket(host_info_list->ai_family,
    host_info_list->ai_socktype,
    host_info_list->ai_protocol);
    if (sockfd == -1) {
      std::cerr << "Error: cannot create socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      return -1;
    } 

    // CONNECT
    status = connect(sockfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      std::cerr << "Error: cannot connect to socket" << std::endl;
      std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
      return -1;
    } 

    // Successfully connected

   /* Set socket to be nonblocking */
   rc = ioctl(sockfd, FIONBIO, (char *)&on);
   if (rc < 0)
   {
      perror("ioctl() failed");
      close(sockfd);
      exit(-1);
   }

  

   /* Initialize the master set */
   int max_sd = sockfd;

   FD_ZERO(&master_set);
   FD_SET(sockfd, &master_set);

   /* Initialize the read fd_set                              */
  
   FD_ZERO(&read_set);
   FD_SET(sockfd, &read_set);

   
   /* Initialize the write fd_set                              */
  
   FD_ZERO(&write_set);
   FD_SET(sockfd, &write_set);

   /*************************************************************/
   /* Initialize the timeval struct to 3 minutes.  If no        */
   /* activity after 3 minutes this program will end.           */
   /*************************************************************/
   timeout.tv_sec  = 10;
   timeout.tv_usec = 0;

   memset(buffer, 0, 80);

   strcpy(buffer, testMessage);

   std::cout << "About to call select\n";


   int done = 0;
   int hasWritten = 0;
   int found;

   do {
      found = 0;
      std::cout << "About to wait on select!\n";
      read_set = master_set;
      write_set = master_set;
      rc = select(max_sd + 1, &read_set, &write_set, NULL, &timeout);
      if (rc > 0) {
         //for (i = 0; i <= max_sd && found < rc; i++) {
            if (!hasWritten && FD_ISSET(sockfd, &write_set)) {
               std::cout << "Socket " << sockfd << " Ready for writing!\n";
               int written = write(sockfd, buffer, strlen(testMessage));
               if (written < 0) {
                  std::cout << "Writing would block, will try again later!\n";
               } 
               else {
                  std::cout << "Wrote " << written << "bytes!\n";
                  hasWritten = 1;
               }
            }
            else if (!hasWritten){
               std::cout << "Socket " << sockfd << " not yet ready for writing!\n";
            }  
            if (FD_ISSET(sockfd, &read_set)) { // assumes a single write and then a single read has to be done
               std::cout << "Read data has arrived on socket " << sockfd << "!\n";
              int recvd = read(sockfd, buffer, 80);
              std::cout << "Bytes read: " << recvd << "\n";
              std::cout << "Buffer:\n" << buffer << "\n"; 
              done = 1;
              break;
            }
            else {
               std::cout << "Socket " << sockfd << " not yet ready for reading!\n";
            }
         //}
      }
      else {
        std::cout << "Timed out!\n";
        done = 1;
      }
   } while (!done);

   std::cout << "Exiting!\n";
   close(sockfd);
}
