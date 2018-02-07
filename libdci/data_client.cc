
#include "dci/data_client.h"
#include "bbque/res/resource_type.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <sstream>
#include <string>
#include <iostream>

// Boost libraries
#include <boost/serialization/access.hpp>
#include <boost/serialization/list.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/asio.hpp>

using namespace bbque::stat;
using namespace boost::asio;
using namespace boost::archive;
using namespace bbque::res;

namespace bbque {


DataClient::DataClient(std::string serverIP, uint32_t serverPort, uint32_t clientPort):
	serverIP(serverIP),
	serverPort(serverPort),
	clientPort(clientPort){

}

DataClient::ExitCode_t DataClient::Connect(){
	
	receiver_started = true;
	client_thread = std::thread(&DataClient::ClientReceiver, this);

	return DataClient::ExitCode_t::OK;
}

DataClient::ExitCode_t DataClient::Disconnect(){
	
	receiver_started = false;
	client_thread.join();

	return DataClient::ExitCode_t::OK;
}

void DataClient::ClientReceiver(){

	printf("Starting the client receiver...\n");
	status_message_t temp_stat;

	io_service ios;
	ip::tcp::endpoint endpoint = 
		ip::tcp::endpoint(ip::tcp::v4(), clientPort);

	/* Listening cycle */
	while(receiver_started) {

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
	status_filter_t filter, 
	status_event_t event, 
	uint16_t period, 
	DataClient::SubMode_t mode){
	
	/* Subscription message initialization */
	subscription_message_t newSub = {clientPort, 
		static_cast<sub_bitset_t>(filter), 
		static_cast<sub_bitset_t>(event), 
		period, 
		mode};

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
	memset(&serverAddr, 0, sizeof(serverAddr));                /* Zero out structure */
	serverAddr.sin_family = AF_INET;                           /* Internet addr family */
	serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());  /* Server IP address */
	serverAddr.sin_port   = htons(serverPort);                 /* Server port */

	int tempint = 0;

	/* Send the subscription message to the server */
	tempint = sendto(sock, (subscription_message_t*)&newSub, 
		(1024+sizeof(newSub)), 0, 
		(struct sockaddr *) &serverAddr, sizeof(serverAddr)); 

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

	const char * DataClient::GetResourcePathString(res_bitset_t bitset){
	std::string resource_path, unit_type_str;
	std::ostringstream os;
	std::bitset<BITSET_LEN_RES> res_bitset(bitset);
	/* Retrieving the system ID */
	std::bitset<BITSET_LEN_RES> sys_bitset = 
		RangeBitset<BITSET_OFFSET_SYS,BITSET_OFFSET_SYS+BITSET_LEN_SYS>(res_bitset);
	sys_bitset = (sys_bitset>>BITSET_OFFSET_SYS);
	os << GetResourceTypeString(ResourceType::SYSTEM) << std::to_string(sys_bitset.to_ulong());

	/* Retrieving unit TYPE */
	std::bitset<BITSET_LEN_RES> unit_bitset =
		RangeBitset<BITSET_OFFSET_UNIT_TYPE,BITSET_OFFSET_UNIT_TYPE+BITSET_LEN_UNIT_TYPE>(res_bitset);
	unit_bitset = (unit_bitset>>BITSET_OFFSET_UNIT_TYPE);
	switch(unit_bitset.to_ulong()){
		case static_cast<uint64_t>(ResourceType::CPU):
		unit_type_str = GetResourceTypeString(ResourceType::CPU);
		break;
		case static_cast<uint64_t>(ResourceType::GPU):
		unit_type_str = GetResourceTypeString(ResourceType::GPU);
		break;
		case static_cast<uint64_t>(ResourceType::ACCELERATOR):
		unit_type_str = GetResourceTypeString(ResourceType::ACCELERATOR);
		break;
		case static_cast<uint64_t>(ResourceType::MEMORY):
		unit_type_str = GetResourceTypeString(ResourceType::MEMORY);
		break;
		case static_cast<uint64_t>(ResourceType::NETWORK_IF):
		unit_type_str = GetResourceTypeString(ResourceType::NETWORK_IF);
		break;
		default:
		resource_path = os.str();
		return resource_path.c_str();
	}

	/* Retrieving unit ID */
	std::bitset<BITSET_LEN_RES> unit_id_bitset = 
		RangeBitset<BITSET_OFFSET_UNIT_ID,BITSET_OFFSET_UNIT_ID+BITSET_LEN_UNIT_ID>(res_bitset);
	unit_id_bitset = (unit_id_bitset>>BITSET_OFFSET_UNIT_ID);
	os << "." << unit_type_str << std::to_string(unit_id_bitset.to_ulong());
	
	/* Retrieving process element TYPE */
	std::bitset<BITSET_LEN_RES> pe_bitset = 
		RangeBitset<BITSET_OFFSET_PE_TYPE,BITSET_OFFSET_PE_TYPE+BITSET_LEN_PE_TYPE>(res_bitset);
	pe_bitset = (pe_bitset>>BITSET_OFFSET_PE_TYPE);
	
	/* Retrieving process element ID */
	if(pe_bitset.to_ulong() != 0){
		std::bitset<BITSET_LEN_RES> pe_id_bitset = 
			RangeBitset<BITSET_OFFSET_PE_ID,BITSET_OFFSET_PE_ID+BITSET_LEN_PE_ID>(res_bitset);
		pe_id_bitset = (pe_id_bitset>>BITSET_OFFSET_PE_ID);
		os << "." << GetResourceTypeString(ResourceType::PROC_ELEMENT)
			<< std::to_string(unit_id_bitset.to_ulong());
	}

	resource_path = os.str();
	return resource_path.c_str();
}

} // namespace bbque