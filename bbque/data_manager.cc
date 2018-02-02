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
#include <algorithm>

#include "bbque/utils/utility.h"

#include "bbque/data_manager.h"

#define DATA_MANAGER_NAMESPACE "bq.dm"
#define MODULE_NAMESPACE DATA_MANAGER_NAMESPACE

#define DEFAULT_SLEEP_TIME 1000
#define SERVER_PORT 30200

// The prefix for configuration file attributes
#define MODULE_CONFIG "DataManager"

namespace bbque {

using namespace bbque::stat;
using namespace boost::asio::ip;
using namespace boost::archive;
namespace bd = bbque::data;
DataManager & DataManager::GetInstance() {
	static DataManager instance;
	return instance;
}

DataManager::DataManager() : Worker() {
	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	logger->Debug("Setupping the publisher...");
	sleep_time = DEFAULT_SLEEP_TIME;
	Setup("DataManagerPublisher", MODULE_NAMESPACE".pub");
	logger->Info("Starting the publisher...");
	Start();

	// Setting the subscription server thread
	server_port = SERVER_PORT;
	//any_subscriber = 0;
	logger->Debug("Spawing thread for the subscription server...");
	subscription_server = std::thread(&DataManager::SubscriptionHandler, this);

	subscription_server.detach();

	// Setting the event handler thread
	logger->Debug("Spawing thread for the event handler...");
	event_handler = std::thread(&DataManager::EventHandler, this);

	event_handler.detach();
}

DataManager::~DataManager() {
	// Send signal to server
	assert(subscription_server_tid != 0);
	::kill(subscription_server_tid, SIGUSR1);

	// Send signal to event handler
	assert(event_handler_tid != 0);
	::kill(event_handler_tid, SIGUSR1);

	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);
	std::unique_lock<std::mutex> events_lock(events_mtx, std::defer_lock);

	subs_lock.lock();
	//any_subscriber = 0;
	subscribers_on_rate.clear();
	subscribers_on_event.clear();
	subs_lock.unlock();

	events_lock.lock();
	event_queue.clear();
	events_lock.unlock();
	
	logger->Info("Terminating the publisher...");
	Terminate();
}

void DataManager::PrintSubscribers(){

	// Subscribers on rate
	for (auto s : subscribers_on_rate){
		logger->Debug("Subscribers on rate: ip: %s port: %d deadline: %d filter: %s rate: %d", 
			s->ip_address.c_str(), 
			s->port_num,
			s->rate_deadline_ms,
			s->subscription.filter.to_string().c_str(),
			s->subscription.rate_ms);	
	}

	// Subscribers on event
	for (auto s : subscribers_on_event){
		logger->Debug("Subscribers on event: ip: %s port: %d event: %s", 
			s->ip_address.c_str(), 
			s->port_num,
			s->subscription.event.to_string().c_str());	
	}
}

SubscriberPtr_t DataManager::findSubscriber(SubscriberPtr_t & subscr,
	SubscriberPtrList_t & list){

	for(auto sub : list){
		if(*sub.get() == *subscr.get())
			return sub;
	}
	return nullptr;
} 

/*******************************************************************/
/*                      Subscription handling                      */
/*******************************************************************/

void DataManager::SubscriptionHandler() {

	/* ---------------- Subscription server setup ---------------- */

	// Getting the id of the subscription server thread
	subscription_server_tid = gettid();

	logger->Info("Starting the subscription server... tid = %d", subscription_server_tid);

	// Allocating memory for the incoming subscriptions
	bbque::stat::subscription_message_t * temp_sub = 
		(bbque::stat::subscription_message_t*) malloc(sizeof(bbque::stat::subscription_message_t));

	logger->Debug("Creating the socket at address %s and port %d...", 
		ip_address.c_str(), 
		server_port);

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
	/* Listening cycle */
	for(;;) {

		client_addr_size = sizeof(client_addr);

		// Subscription receiving waiting
		if((recv_msg_size = recvfrom(sock, temp_sub, sizeof(*temp_sub),	
			0, (struct sockaddr*)&client_addr, &client_addr_size))<0)
				logger->Error("Error during socket receiving");
		
		// Creating the temp Subscription
		Subscription temp_subscription(bd::sub_bitset_t(temp_sub->filter), 
			bd::sub_bitset_t(temp_sub->event),
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
			Unsubscribe(temp_subscriber,temp_sub->event != 0);
	}
	/* ----------------------------------------------------------- */
}

void DataManager::Subscribe(SubscriberPtr_t & subscr, bool event){
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	logger->Info("Subscribing client: %s:%d",subscr->ip_address.c_str(),subscr->port_num);

	subs_lock.lock();

	if(event) { // If event-based subscription
		auto sub = findSubscriber(subscr, subscribers_on_event);

		// If the client is already a subscriber just update its event filter
		if(sub != nullptr){
			logger->Debug("Before update %s - event: %s",
				subscr->ip_address.c_str(),
				sub->subscription.event.to_string().c_str());

			sub->subscription.event|=subscr->subscription.event;

			logger->Debug("After update %s - event: %s",
				subscr->ip_address.c_str(),
				sub->subscription.event.to_string().c_str());
		}else { 
		// If is new, just add it to the list
			subscribers_on_event.push_back(subscr);
		}
	}
	else { // If rate-based subscription
		auto sub = findSubscriber(subscr, subscribers_on_rate);

		// If the client is already a subscriber just update its filter and rate
		if(sub != nullptr){
			logger->Debug("Before update %s - filter: %s - rate: %d",
				subscr->ip_address.c_str(),
				sub->subscription.filter.to_string().c_str(),
				sub->subscription.rate_ms);

			sub->subscription.filter|=subscr->subscription.filter;
			sub->subscription.rate_ms = subscr->subscription.rate_ms;

			logger->Debug("After update %s - filter: %s - rate: %d",
				subscr->ip_address.c_str(),
				sub->subscription.filter.to_string().c_str(),
				sub->subscription.rate_ms);
		}else { 
		// If is new, just add it to the list
			subscribers_on_rate.push_back(subscr);
			logger->Debug("Sorting on rate...");
			subscribers_on_rate.sort();
			subs_cv.notify_one();
		}
		subscribers_on_rate.sort();
	}

	// Printing all the subscribers
	PrintSubscribers();

	subs_lock.unlock();

	
}

void DataManager::Unsubscribe(SubscriberPtr_t & subscr, bool event){
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	logger->Info("Unsubscribing client: %s:%d",
		subscr->ip_address.c_str(),
		subscr->port_num);

	subs_lock.lock();

	if(event){ // If event-based subscription
		auto sub = findSubscriber(subscr, subscribers_on_event);
		
		if(sub != nullptr){
			sub->subscription.event&=~(subscr->subscription.event);
			if(sub->subscription.event == 0){
				subscribers_on_event.remove(sub);
		}
		}else{
			logger->Error("Client not found!");
		}
	}else{ // If rate-based subscription
		auto sub = findSubscriber(subscr, subscribers_on_rate);	
		
		if(sub != nullptr){
			sub->subscription.filter&=~(subscr->subscription.filter);
			if(sub->subscription.filter == 0){
				subscribers_on_rate.remove(sub);
			}
			subscribers_on_rate.sort();
		}else{
			logger->Error("Client not found!");
		}
		
	}

	// Printing all the subscribers
	PrintSubscribers();

	subs_lock.unlock();
}


/*******************************************************************/
/*                       Publishing handling                       */
/*******************************************************************/

void DataManager::Task() {
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	logger->Debug("Starting worker...");
	while(true){
		
		subs_lock.lock();
		while(subscribers_on_rate.empty()){
			subs_cv.wait(subs_lock);
		};
		subs_lock.unlock();
			
		PublishOnRate();
			
		logger->Debug("Going to sleep for %d...",sleep_time);
			
		// Sleep
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
	}
}

void DataManager::NotifyUpdate(status_event_t event){

	logger->Notice("Update notified: %d",event);

	std::unique_lock<std::mutex> events_lock(events_mtx, std::defer_lock);

	events_lock.lock();
	event_queue.push_back(event);
	events_lock.unlock();
	evt_cv.notify_one();
}

void DataManager::EventHandler(){
	// Getting the id of the subscription server thread
	event_handler_tid = gettid();

	logger->Info("Starting the event handler... tid = %d", event_handler_tid);

	std::unique_lock<std::mutex> events_lock(events_mtx, std::defer_lock);

	while(true){
		std::vector<status_event_t> events_vec;

		events_lock.lock();
		while(event_queue.size()==0){
			evt_cv.wait(events_lock) ;
		}

		for(auto & event: event_queue){
			events_vec.push_back(event);
		}
		event_queue.clear();
		events_lock.unlock();

		// Avoiding blocking function
		for(const auto & event : events_vec){
			logger->Debug("Event handling: %d",event);
			PublishOnEvent(event);
		}
	}
}

void DataManager::PublishOnEvent(status_event_t event){
	ExitCode_t result;

	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	// Update resources and applications data
	UpdateData();

	subs_lock.lock();

	for(const auto & s : subscribers_on_event){
		// If event matched
		if((s->subscription.event & bd::sub_bitset_t(event)) == bd::sub_bitset_t(event)){

			result = Push(s);

			if(result != OK){
				logger->Error("Error in publish status to %s:%d", 
					s->ip_address.c_str(),s->port_num);
			}else{
				logger->Notice("Publish status to %s:%d", 
					s->ip_address.c_str(),s->port_num);
			}
		}
	}

	subs_lock.unlock();
}

void DataManager::PublishOnRate(){
	uint16_t tmp_sleep_time;// = sleep_time;
	ExitCode_t result;

	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	// Update resources and applications data
	UpdateData();

	subs_lock.lock();

	tmp_sleep_time = subscribers_on_rate.back()->subscription.rate_ms;

	for(auto s : subscribers_on_rate){
		// Updating the deadline after the sleep
		s->rate_deadline_ms = s->rate_deadline_ms - sleep_time;

		logger->Debug("Subscriber: %s -- next_deadline: %u",
			s->ip_address.c_str(),
			s->rate_deadline_ms);

		// If the deadline is missed or is now push the updated info
		if(s->rate_deadline_ms <= 0) {

			result = Push(s);

			if(result != OK){
				logger->Error("Error in publish status to %s:%d", 
					s->ip_address.c_str(),s->port_num);
			}else{
				logger->Notice("Publish status to %s:%d", 
					s->ip_address.c_str(),s->port_num);
			}

			// Reset the deadline
			s->rate_deadline_ms = s->subscription.rate_ms;

			logger->Debug("Subscriber: %s -- updated next_deadline: %u",
				s->ip_address.c_str(),
				s->rate_deadline_ms);

		}
		// Calculating the earlier sleep time the sleep time with the earlier
		if(s->rate_deadline_ms < tmp_sleep_time)
			tmp_sleep_time = s->rate_deadline_ms;
	}

	// Updating the sleep time
	sleep_time = tmp_sleep_time;

	subscribers_on_rate.sort();

	subs_lock.unlock();
}

DataManager::ExitCode_t DataManager::Push(SubscriberPtr_t sub){
	
	// Status message initialization
	status_message_t newStat;
	newStat.ts = 1; //static_cast<uint32_t>(timer.getTimestamp());
	newStat.n_app_status_msgs = 0;
	newStat.n_res_status_msgs = 0;

	logger->Debug("Status timestamp is %d", newStat.ts);

	// Checking the subscriber filter

	// If it has resource subscription
	if((sub->subscription.filter & bd::sub_bitset_t(FILTER_RESOURCE)) == 
		bd::sub_bitset_t(FILTER_RESOURCE)){
		
		logger->Debug("Adding resources info to the subscriber %s's message...", 
			sub->ip_address.c_str());
		newStat.n_res_status_msgs = num_current_res;
		for(auto res_stat : res_stats){
			newStat.res_status_msgs.push_back(res_stat);
		}
		// Debug logging
		for(auto res_stat : newStat.res_status_msgs){
			logger->Debug("ResId: %d, Occupancy: %d, Load: %d, Power: %d, Temp: %d",
				res_stat.id, 
				res_stat.occupancy, 
				res_stat.load, 
				res_stat.power, 
				res_stat.temp);
		}	
	}

	// If it has application subscription
	if((sub->subscription.filter & bd::sub_bitset_t(FILTER_APPLICATION)) == 
		bd::sub_bitset_t(FILTER_APPLICATION)){
		
		logger->Debug("Adding applications info to the subscriber %s's message...", 
			sub->ip_address.c_str());

		newStat.n_app_status_msgs = num_current_apps;

		for(auto app_stat : app_stats){
			newStat.app_status_msgs.push_back(app_stat);
		}

		// Debug logging
		for(auto app_stat : newStat.app_status_msgs){
			logger->Debug("AppId: %d, AppName: %s, NunTasks: %d, Mapping: %d",
				app_stat.id, 
				app_stat.name.c_str(), 
				app_stat.n_task, 
				app_stat.mapping);
			for(auto task_stat : app_stat.tasks){
				logger->Debug("TaskId: %d, Perf: %d, Mapping: %d",
				task_stat.id, 
				task_stat.perf, 
				task_stat.mapping);
			}
		}
	}

	// Create a TCP socket
	tcp::iostream client_sock(sub->ip_address, std::to_string(sub->port_num));
	
	if(!client_sock){
		logger->Error("Cannot connect to %s:%d",
			sub->ip_address.c_str(),
			sub->port_num);
		return ERR_CLIENT_COMM;
	}

	// Saving data to archive
	text_oarchive archive(client_sock);

	// Sending data to the client
	try{ 
		archive << newStat;
	}catch(boost::exception const& ex){
		logger->Error("Boost archive exception");
		return ERR_CLIENT_COMM;
	}

	return OK;
}

res_bitset_t DataManager::BuildResourceBitset(br::ResourcePathPtr_t resource_path){
	res_bitset_t res_bitset = 0;
	for(auto resource_identifier : resource_path->GetIdentifiers()){
	
		switch(resource_identifier->Type()){
			case res::ResourceType::SYSTEM:
				res_bitset |= static_cast<uint64_t>(resource_identifier->ID()) << BITSET_OFFSET_SYS;
				break;
			case res::ResourceType::CPU: 
			case res::ResourceType::GPU:
			case res::ResourceType::ACCELERATOR:
				res_bitset |= 
					static_cast<uint64_t>(resource_identifier->Type()) << BITSET_OFFSET_UNIT_TYPE;
				res_bitset |= static_cast<uint64_t>(resource_identifier->ID()) << BITSET_OFFSET_UNIT_ID;
				break;
			case res::ResourceType::PROC_ELEMENT:
				res_bitset |= static_cast<uint64_t>(resource_identifier->ID()) << BITSET_OFFSET_PE;
				break;
			default:
				break;
		}
	}
	return res_bitset;
}

void DataManager::UpdateData(){
	logger->Debug("Updating applications and resources data...");

	
	// Taking data from Resource accounter and Power Manager
	std::set<br::ResourcePtr_t> resource_set = ra.GetResourceSet();

	num_current_res = resource_set.size();
	// Updating resource status list
	res_stats.clear();

	for(auto & resource_ptr : resource_set){
		br::ResourcePathPtr_t resource_path = ra.GetPath(resource_ptr->Path());
		logger->Debug("%ld: Used: %d Unreserved: %d Total: %d Temperature: %d Frequency: %d Power: %2.2f",
			BuildResourceBitset(resource_path),
			resource_ptr->Used(),
			resource_ptr->Unreserved(),
			resource_ptr->Total(),
			static_cast<uint32_t>(resource_ptr->GetPowerInfo(PowerManager::InfoType::TEMPERATURE, 
				br::Resource::ValueType::INSTANT)),
			static_cast<uint32_t>(resource_ptr->GetPowerInfo(PowerManager::InfoType::FREQUENCY, 
				br::Resource::ValueType::INSTANT)),
			static_cast<uint32_t>(resource_ptr->GetPowerInfo(PowerManager::InfoType::POWER, 
				br::Resource::ValueType::INSTANT)));

		resource_status_t temp_res;
		temp_res.id = BuildResourceBitset(resource_path);
		temp_res.occupancy = static_cast<uint8_t>(resource_ptr->Used());
		temp_res.load = static_cast<uint8_t>(resource_ptr->GetPowerInfo(PowerManager::InfoType::LOAD, 
			br::Resource::ValueType::INSTANT));
		temp_res.power = static_cast<uint32_t>(resource_ptr->GetPowerInfo(PowerManager::InfoType::POWER, 
			br::Resource::ValueType::INSTANT));
		temp_res.temp = static_cast<uint32_t>(resource_ptr->GetPowerInfo(PowerManager::InfoType::TEMPERATURE, 
			br::Resource::ValueType::INSTANT));
		res_stats.push_back(temp_res);
	}
}

} // namespace bbque