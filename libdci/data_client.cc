
#include "dci/data_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

// Boost libraries
#include <boost/serialization/access.hpp>
#include <boost/serialization/list.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/asio.hpp>

using namespace bbque::stat;
using namespace boost::asio;
using namespace boost::archive;

namespace bbque {


DataClient::DataClient(std::string serverIP, int serverPort, int clientPort){
	this->serverIP=serverIP;
	this->serverPort=serverPort;
	this->clientPort=clientPort;
}

DataClient::ExitCode_t DataClient::Connect(){
	
	client_thread = std::thread(&DataClient::ClientReceiver, this);

	return DataClient::ExitCode_t::OK;
}

DataClient::ExitCode_t DataClient::Disconnect(){
  client_thread.join();

  return DataClient::ExitCode_t::OK;
}

std::function<void()> DataClient::ClientReceiver(){

  printf("Starting the client receiver...\n");
  status_message_t temp_stat;

  io_service ios;
  ip::tcp::endpoint endpoint = 
    ip::tcp::endpoint(ip::tcp::v4(), clientPort);

  /* Listening cycle */
  for(;;) {

    boost::asio::ip::tcp::acceptor acceptor(ios);
    boost::asio::ip::tcp::iostream stream;

    /* TCP Socket setup */
    printf("Socket setup...\n");

    try{
    acceptor.open(endpoint.protocol());
    acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
    acceptor.bind(endpoint);
    }catch(boost::exception const& ex){
      perror("Exception during socket setup: %s");
      break;
    }

    /* Incoming connection management */
    printf("Receiving TCP packets...\n");
    try{
    acceptor.listen();
    acceptor.accept(*stream.rdbuf()); 
    }catch(boost::exception const& ex){
      perror("Exception waiting incoming replies: %s");
      break;
    }

    /* Receiving update */
    try{
      boost::archive::text_iarchive archive(stream);
      archive >> temp_stat;
    }catch(boost::exception const& ex){
      perror("Archive exception: %s");
      break;
    }

    /* Send update to the client calling its callback function */
    client_callback(temp_stat);

    try{
      acceptor.close();
    }catch(boost::exception const& ex){
      perror("Exception closing the socket: %s");
      break;
    }
  }
}

DataClient::ExitCode_t DataClient::Subscribe(
	sub_bitset_t filter, 
	sub_bitset_t event, 
	uint16_t rate, 
	DataClient::SubMode mode){
	
	/* Subscription message initialization */
	subscription_message_t newSub = {clientPort, filter, event, rate, mode};

	/* Socket descriptor */
	int sock;

	/* Server address */
	struct sockaddr_in serverAddr;

	printf("Subscription: \n");
 	printf("\t - Reply port: %d\n",newSub.port_num);
 	printf("\t - Filter: %d\n",newSub.filter);
  	printf("\t - Event: %d\n",newSub.event);
  	printf("\t - Rate: %d\n",newSub.rate_ms);
  	printf("\t - Mode: %d\n",newSub.mode);

  	printf("Size struct: %ld\n", sizeof(newSub));

  	/* Create a datagram/UDP socket */
  	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
    	return DataClient::ExitCode_t::ERR_SERVER_COMM;
    }

  	/* Construct the server address structure */
  	memset(&serverAddr, 0, sizeof(serverAddr));       /* Zero out structure */
  	serverAddr.sin_family = AF_INET;                  /* Internet addr family */
  	serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());   /* Server IP address */
  	serverAddr.sin_port   = htons(serverPort);        /* Server port */

  	int tempint = 0;

  	/* Send the subscription message to the server */
  	tempint = sendto(sock, (subscription_message_t*)&newSub, (1024+sizeof(newSub)), 0, (struct sockaddr *)
    	&serverAddr, sizeof(serverAddr)); 

  	if (tempint == -1 ) {
    	printf("Sent struct size: %d\n", tempint);
      return DataClient::ExitCode_t::ERR_UNKNOWN;
  	}else{
    	printf("Sending complete!\n");
	}

  	/* Closing subscription socket */
  	close(sock);

  	return DataClient::ExitCode_t::OK;
}

} // namespace bbque