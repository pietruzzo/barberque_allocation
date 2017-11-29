/*
 * Copyright (C) 2017  Politecnico di Milano
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
#include <csignal>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>

#include "bbque/utils/utility.h"

#include "bbque/data_manager.h"

#define DATA_MANAGER_NAMESPACE "bq.dm"
#define MODULE_NAMESPACE DATA_MANAGER_NAMESPACE

#define DEFAULT_SLEEP_TIME 1000
#define SERVER_PORT 30200

// The prefix for configuration file attributes
#define MODULE_CONFIG "DataManager"

namespace bbque {

DataManager & DataManager::GetInstance() {
	static DataManager instance;
	return instance;
}

DataManager::DataManager() : Worker() {
	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	logger->Debug("Setupping the publisher...");
	sleep_time = DEFAULT_SLEEP_TIME;
	Setup("DataManagePublisher", MODULE_NAMESPACE".pub");
	logger->Info("Starting the publisher...");
	Start();

	// Setting the subscription server port
	server_port = SERVER_PORT;
	logger->Debug("Spawing thread for the subscription server...");
	subscription_server = std::thread(&DataManager::SubscriptionHandler, this);

	subscription_server.detach();
}

DataManager::~DataManager() {
	// Send signal to server
	assert(subscription_server_tid != 0);
	::kill(subscription_server_tid, SIGUSR1);

	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	subs_lock.lock();
	any_subscriber = false;
	subscribers_on_rate.clear();
	subscribers_on_event.clear();
	subs_lock.unlock();
	
	logger->Info("Terminating the publisher...");
	Terminate();
}

void DataManager::Task() {

	logger->Debug("Starting worker...");
	while(1){

		while(!any_subscriber){};
		//while(any_subscriber){

			Publish();
			
			logger->Debug("Going to sleep for %d...",sleep_time);
			
			// Sleep
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
		//}
	}
}

void DataManager::SubscriptionHandler() {

	/* ---------------- Subscription server setup ---------------- */

	// Getting the id of the subscription server thread
	subscription_server_tid = gettid();

	logger->Info("Starting the subscription server... tid = %d", subscription_server_tid);

	// Allocating memory for the incoming subscriptions
	bbque::stat::subscription_t * temp_sub = 
		(bbque::stat::subscription_t*) malloc(sizeof(bbque::stat::subscription_t));

	logger->Debug("Creating the socket...");

	// Creating the socket
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(sock<0)
		logger->Error("Error during socket creation");

	logger->Debug("Allocating the local address...");

	// Allocating memory for the local address and port
	memset(&local_address, 0, sizeof(local_address));	/* Zero out structure */
	local_address.sin_family = AF_INET;					/* Internet address family */
	local_address.sin_addr.s_addr = htonl(INADDR_ANY);	/* Any incoming interface */
	local_address.sin_port = htons(server_port);		/* Local port */

	logger->Debug("Binding the socket...");

	// Binding the socket
	if(bind(sock, (struct sockaddr *)&local_address, sizeof(local_address))<0)
		logger->Error("Error during socket binding");

	logger->Debug("Receiving UDP packets...");

	/* ----------------------------------------------------------- */

	/* ------------------ Subscription handling ------------------ */
	// Listening cycle
	for(;;) {

		client_addr_size = sizeof(client_addr);

		// Subscription receiving waiting
		if((recv_msg_size = recvfrom(sock, temp_sub, sizeof(*temp_sub),	
			0, (struct sockaddr*)&client_addr, &client_addr_size))<0)
				logger->Error("Error during socket receiving");
		
		// Creating the temp Subscription
		Subscription temp_subscription(data::sub_bitset_t(temp_sub->filter), 
			data::sub_bitset_t(temp_sub->event),
			temp_sub->rate_ms);

		// Creating the temp Subscriber
		SubscriberPtr_t temp_subscriber = std::make_shared<Subscriber>
			(std::string(inet_ntoa(client_addr.sin_addr)),
				(uint32_t) temp_sub->port_num,
				temp_subscription);

		// Logging messages
		logger->Debug("Handling client %s", inet_ntoa(client_addr.sin_addr));		
		logger->Debug("Incoming length: %u", sizeof(*temp_sub));
		logger->Notice("Client subscriber:");
		logger->Notice("\tPort: %d",temp_subscriber->port_num);
		logger->Notice("\tFilter: %s",
			data::sub_bitset_t(temp_subscriber->subscription.filter).to_string().c_str()); 
		logger->Notice("\tEvent: %s", 
			data::sub_bitset_t(temp_subscriber->subscription.event).to_string().c_str());
		logger->Notice("\tRate: %d ms",temp_subscriber->subscription.rate_ms);
		logger->Notice("\tMode: %d",temp_sub->mode);

		if(temp_sub->mode == 0)
			Subscribe(temp_subscriber,temp_sub->event != 0);
		else
			Unsubscribe(temp_subscriber);
	}

	/* ----------------------------------------------------------- */
	

	/* SAMPLE CODE */
/*
	Subscription subscr1("010","000",5000);
	Subscription subscr2("001","000",4000);

	logger->Notice("Subscription 1 rate = %d", subscr1.rate_ms);

	SubscriberPtr_t Sub1(new Subscriber);
	SubscriberPtr_t Sub2(new Subscriber);

	Sub1->ip_address = "0.0.0.1";
	Sub1->subscription = subscr1;
	Sub1->rate_deadline_ms = subscr1.rate_ms;
	
	logger->Notice("Subscriber 1 rate = %d rate_deadline = %d", 
		Sub1->subscription.rate_ms, 
		Sub1->rate_deadline_ms);
	
	Sub2->ip_address = "0.0.0.2";
	Sub2->subscription = subscr2;
	Sub2->rate_deadline_ms = subscr2.rate_ms;

	Subscribe(Sub2, false);
	Subscribe(Sub1, false);
*/
	/* END SAMPLE CODE */

}

void DataManager::Subscribe(SubscriberPtr_t & subscr, bool event){
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	logger->Info("Subscribing client: %s",subscr->ip_address.c_str());

	subs_lock.lock();
/*
	if(event) {
		auto it = find(subscribers_on_event.begin(), subscribers_on_event.end(), subscr->ip_address);
		if(it != subscribers_on_event.end()){
			it->subscription.event|=subscr->subscription.event;
		}else {
			subscribers_on_event.push_back(subscr);
			any_subscriber++;
		}
	}
	else {
		subscribers_on_rate.push_back(subscr);
		logger->Debug("Sorting on rate...");
		subscribers_on_rate.sort();
	}*/


	for (auto s : subscribers_on_rate){
		logger->Debug("Subiscribers on rate: %s %d %s %d", 
			s->ip_address.c_str(), 
			s->rate_deadline_ms,
			s->subscription.filter.to_string().c_str(),
			s->subscription.rate_ms);	
	}


	subs_lock.unlock();
}

void DataManager::Unsubscribe(SubscriberPtr_t & subscr){
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	logger->Info("Unsubscribing client: %s",subscr->ip_address.c_str());
}

void DataManager::Publish(){
	uint16_t tmp_sleep_time, max_sleep_time = 0;// = sleep_time;

	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	subs_lock.lock();

	for(auto s : subscribers_on_rate){
		s->rate_deadline_ms = s->rate_deadline_ms - sleep_time;

		logger->Debug("Subscriber: %s -- next_deadline: %u",
			s->ip_address.c_str(),
			s->rate_deadline_ms);

		if(s->rate_deadline_ms <= 0) {
			//Push();

			logger->Notice("Publish status to %s", s->ip_address.c_str());

			s->rate_deadline_ms = s->subscription.rate_ms;

			logger->Debug("Subscriber: %s -- updated next_deadline: %u",
				s->ip_address.c_str(),
				s->rate_deadline_ms);

		}
		tmp_sleep_time = subscribers_on_rate.front()->rate_deadline_ms;
		if(s->rate_deadline_ms < tmp_sleep_time)
			tmp_sleep_time = s->rate_deadline_ms;
	}

	sleep_time = tmp_sleep_time;

	subscribers_on_rate.sort();

	subs_lock.unlock();
}

} // namespace bbque