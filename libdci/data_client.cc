/*
 * Copyright (C) 2018  Politecnico di Milano
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dci/data_client.h"
#include "bbque/res/resource_type.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>

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

DataClient::DataClient(
	std::string ip_addr,
	uint32_t server_port,
	uint32_t client_port,
	std::function<void(status_message_t)> callback_fn):
		server_ip(ip_addr),
		server_port(server_port),
		client_port(client_port),
		client_callback(callback_fn) {
	client_thread_tid = 0;
	Connect();
}

DataClient::~DataClient() {
	Disconnect();
}


DataClient::ExitCode_t DataClient::Connect() {
	if (client_thread_tid != 0) {
		fprintf(stderr, "Client may be already connected\n");
		return DataClient::ExitCode_t::OK;
	}

	fprintf(stderr, "Connecting... \n");
	unsigned short nr_attempts = BBQUE_DCI_CONNECT_NR_ATTEMPTS;
	client_thread = std::thread(&DataClient::ClientReceiver, this);
	std::unique_lock<std::mutex> lck(mtx_connection);
	while (!is_connected && nr_attempts > 0) {
		cv_connection.wait_for(lck, std::chrono::seconds(BBQUE_DCI_CONNECT_WAIT_S));
		nr_attempts--;
		fprintf(stderr, "Connecting... [attempts left: %d]\n", nr_attempts);
	}

	if (!is_connected) {
		fprintf(stderr, "Connection failed!\n");
		return DataClient::ExitCode_t::ERR_SERVER_UNREACHABLE;
	}
	fprintf(stderr, "Connected\n");
	return DataClient::ExitCode_t::OK;
}

DataClient::ExitCode_t DataClient::Disconnect() {
	fprintf(stderr, "Disconnecting...\n");
	assert(client_thread_tid != 0);
	if (client_thread_tid != 0)
		return DataClient::ExitCode_t::ERR_MISSING_CONNECTION;

	{
		std::unique_lock<std::mutex> lck(mtx_connection);
		is_connected = false;
		cv_connection.notify_one();
	}

	// Kill the connection thread
	::kill(client_thread_tid, SIGUSR1);
	fprintf(stderr, "Disconnected\n");
	return DataClient::ExitCode_t::OK;
}


bool DataClient::IsConnected() {
	std::unique_lock<std::mutex> lck(mtx_connection);
	return is_connected;
}

void DataClient::ClientReceiver() {
	status_message_t stat_msg;
	client_thread_tid = syscall(SYS_gettid);

	io_service ios;
	ip::tcp::endpoint endpoint =
		ip::tcp::endpoint(ip::tcp::v4(), client_port);
	ip::tcp::acceptor acceptor(ios);
	ip::tcp::iostream stream;

	// TCP Socket setup
	try {
		acceptor.open(endpoint.protocol());
		acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(endpoint);
	} catch(boost::exception const& ex){
		fprintf(stderr,"Exception during socket setup\n");
		client_thread_tid = 0;
		return;
	}

	{ // Set connection ready
		std::unique_lock<std::mutex> lck(mtx_connection);
		is_connected = true;
		cv_connection.notify_one();
	}

	while (IsConnected()) {
		// Incoming connection management
		try {
			acceptor.listen();
			acceptor.accept(*stream.rdbuf());
		} catch(boost::exception const& ex) {
			fprintf(stderr,"Exception waiting for incoming replies\n");
			break;
		}

		// Receiving update
		try {
			boost::archive::text_iarchive archive(stream);
			archive >> stat_msg;
		} catch(boost::exception const& ex){
			fprintf(stderr,"Exception while sending message\n");
			break;
		}
		// Execute callback function
		client_callback(stat_msg);
	}

	fprintf(stderr, "Closing incoming connections...\n");
	try {
		acceptor.close();
	} catch(boost::exception const& ex){
		fprintf(stderr,"Exception closing the socket\n");
	}
	client_thread_tid = 0;
}

DataClient::ExitCode_t DataClient::Subscribe(
		status_filter_t filter,
		status_event_t event,
		uint16_t period,
		DataClient::subscription_mode_t mode) {

	// Subscription message initialization
	subscription_message_t new_subscript = {
		client_port,
		static_cast<sub_bitset_t>(filter),
		static_cast<sub_bitset_t>(event),
		period,
		mode
	};

	int sock_fd;
	struct sockaddr_in server_addr;

	// Open a datagram/UDP socket
	if ((sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
		return DataClient::ExitCode_t::ERR_SERVER_UNREACHABLE;
	}

	// Construct the server address structure
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
	server_addr.sin_port   = htons(server_port);

	// Send the subscription message to the server
	int msg_size = sendto(sock_fd,
			(subscription_message_t*) &new_subscript,
			(1024+sizeof(new_subscript)), 0,
			(struct sockaddr *) &server_addr, sizeof(server_addr));

	if (msg_size == -1 ) {
		fprintf(stderr, "Sent struct size: %d\n", msg_size);
		return DataClient::ExitCode_t::ERR_UNKNOWN;
	}

	// Closing subscription socket
	close(sock_fd);
	return DataClient::ExitCode_t::OK;
}


const char * DataClient::GetResourcePathString(res_bitset_t bitset) {
	std::string resource_path, unit_type_str;
	std::ostringstream outstr;
	std::bitset<BBQUE_DCI_LEN_RES> res_bitset(bitset);
	/* Retrieving the system ID */
	std::bitset<BBQUE_DCI_LEN_RES> sys_bitset =
		RangeBitset<BBQUE_DCI_OFFSET_SYS,
					BBQUE_DCI_OFFSET_SYS + BBQUE_DCI_LEN_SYS>(res_bitset);
	sys_bitset = (sys_bitset >> BBQUE_DCI_OFFSET_SYS);
	outstr << GetResourceTypeString(ResourceType::SYSTEM)
		<< std::to_string(sys_bitset.to_ulong());

	/* Retrieving unit TYPE */
	std::bitset<BBQUE_DCI_LEN_RES> unit_bitset =
		RangeBitset<BBQUE_DCI_OFFSET_UNIT_TYPE,
					BBQUE_DCI_OFFSET_UNIT_TYPE + BBQUE_DCI_LEN_UNIT_TYPE>(res_bitset);
	unit_bitset = (unit_bitset>>BBQUE_DCI_OFFSET_UNIT_TYPE);
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
			resource_path = outstr.str();
		return resource_path.c_str();
	}

	/* Retrieving unit ID */
	std::bitset<BBQUE_DCI_LEN_RES> unit_id_bitset =
		RangeBitset<BBQUE_DCI_OFFSET_UNIT_ID,
					BBQUE_DCI_OFFSET_UNIT_ID + BBQUE_DCI_LEN_UNIT_ID>(res_bitset);
	unit_id_bitset = (unit_id_bitset>>BBQUE_DCI_OFFSET_UNIT_ID);
	outstr << "." << unit_type_str << std::to_string(unit_id_bitset.to_ulong());

	/* Retrieving process element TYPE */
	std::bitset<BBQUE_DCI_LEN_RES> pe_bitset =
		RangeBitset<BBQUE_DCI_OFFSET_PE_TYPE,
					BBQUE_DCI_OFFSET_PE_TYPE + BBQUE_DCI_LEN_PE_TYPE>(res_bitset);
	pe_bitset = (pe_bitset>>BBQUE_DCI_OFFSET_PE_TYPE);

	/* Retrieving process element ID */
	if (pe_bitset.any()) {
		std::bitset<BBQUE_DCI_LEN_RES> pe_id_bitset =
			RangeBitset<BBQUE_DCI_OFFSET_PE_ID,
						BBQUE_DCI_OFFSET_PE_ID + BBQUE_DCI_LEN_PE_ID>(res_bitset);
		pe_id_bitset = (pe_id_bitset>>BBQUE_DCI_OFFSET_PE_ID);
		outstr << "." << GetResourceTypeString(ResourceType::PROC_ELEMENT)
			<< std::to_string(unit_id_bitset.to_ulong());
	}

	resource_path = outstr.str();
	return resource_path.c_str();
}

} // namespace bbque