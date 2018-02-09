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
#include "bbque/utils/timer.h"

#include "bbque/data_manager.h"

#define DATA_MANAGER_NAMESPACE "bq.dm"
#define MODULE_NAMESPACE DATA_MANAGER_NAMESPACE

#define DEFAULT_SLEEP_TIME 1000
#define DEFAULT_SERVER_PORT 30200


// The prefix for configuration file attributes
#define MODULE_CONFIG "DataManager"

namespace bbque {

using namespace bbque::stat;
using namespace boost::asio::ip;
using namespace boost::archive;
namespace bd = bbque::data;
namespace br = bbque::res;
namespace po = boost::program_options;


#define LOAD_CONFIG_OPTION(name, type, var, default) \
	opts_desc.add_options() \
		(MODULE_CONFIG "." name, po::value<type>(&var)->default_value(default), "");


DataManager & DataManager::GetInstance() {
	static DataManager instance;
	return instance;
}

DataManager::DataManager() : Worker(),
	cfm(ConfigurationManager::GetInstance()),
	ra(ResourceAccounter::GetInstance()){

	logger = bu::Logger::GetLogger(MODULE_NAMESPACE);
	logger->Debug("Publisher setup...");
	sleep_time = DEFAULT_SLEEP_TIME;
	Setup("DataManagerPublisher", MODULE_NAMESPACE".pub");

	try {
		po::options_description opts_desc("Data Manager options");
		LOAD_CONFIG_OPTION("server_port", uint32_t,  server_port, DEFAULT_SERVER_PORT);

		po::variables_map opts_vm;
		cfm.ParseConfigurationFile(opts_desc, opts_vm);
	}
	catch(boost::program_options::invalid_option_value ex) {
		logger->Error("Errors in configuration file [%s]", ex.what());
	}

	logger->Info("Publisher start...");
	Start();

	// Setting the subscription server thread
	logger->Debug("Subscription server thread start...");
	subscription_server = std::thread(&DataManager::SubscriptionHandler, this);
	subscription_server.detach();

	// Setting the event handler thread
	logger->Debug("Event handler thread start...");
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
	

	logger->Info("DataManager terminating...");
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

SubscriberPtr_t DataManager::FindSubscriber(
		SubscriberPtr_t & sub_to_find,
		SubscriberPtrList_t & subscribers){
	for (auto sub: subscribers){
		if (*sub.get() == *sub_to_find.get())
			return sub;
	}
	return nullptr;
}

/*******************************************************************/
/*                      Subscription handling                      */
/*******************************************************************/

void DataManager::SubscriptionHandler() {
	/* ---------------- Server setup ---------------- */

	// Getting the id of the subscription server thread
	subscription_server_tid = gettid();
	logger->Info("SubscriptionHandler: starting server [tid=%d]... ",
		subscription_server_tid);

	// Creating the socket
	logger->Debug("SubscriptionHandler: opening socket @<%s:%d>...",
		ip_address.c_str(), server_port);
	sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sock_fd<0)
		logger->Error("SubscriptionHandler: error during socket creation");

	// Local address settings
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	local_addr.sin_port = htons(server_port);

	// Binding the socket
	logger->Debug("SubscriptionHandler: binding socket...");
	if (bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
		logger->Error("SubscriptionHandler: error during socket binding");

	/* ----------------------------------------------------------- */
	/* ------------------ Subscription handling ------------------ */

	// Incoming subscription message
	bbque::stat::subscription_message_t sub_msg;
	logger->Debug("SubscriptionHandler: ready");
	for(;;) {
		client_addr_size = sizeof(client_addr);

		// Subscription receiving waiting
		if((recv_msg_size = recvfrom(sock_fd, &sub_msg, sizeof(sub_msg),
			0, (struct sockaddr*) &client_addr, &client_addr_size)) < 0)
				logger->Error("SubscriptionHandler: error during socket receiving");

		// Subscription information
		Subscription subscription_info(
			bd::sub_bitset_t(sub_msg.filter),
			bd::sub_bitset_t(sub_msg.event),
			static_cast<double>(sub_msg.rate_ms));

		// Subscriber
		SubscriberPtr_t subscriber = std::make_shared<Subscriber>
			(std::string(inet_ntoa(client_addr.sin_addr)),
				(uint32_t) sub_msg.port_num, subscription_info);

		// Logging messages
		logger->Debug("SubscriptionHandler: client <%s:%d>",
			inet_ntoa(client_addr.sin_addr), sub_msg.port_num);
		logger->Debug("SubscriptionHandler: message size: %u", sizeof(sub_msg));
		logger->Notice("Client subscriber:");
		logger->Notice("\tPort: %d",subscriber->port_num);
		logger->Notice("\tFilter: %s",
			data::sub_bitset_t(subscriber->subscription.filter).to_string().c_str());
		logger->Notice("\tEvent: %s",
			data::sub_bitset_t(subscriber->subscription.event).to_string().c_str());
		logger->Notice("\tRate: %d ms",subscriber->subscription.rate_ms);
		logger->Notice("\tMode: %d",sub_msg.mode);

		if (sub_msg.mode == 0)
			Subscribe(subscriber, sub_msg.event != 0);
		else
			Unsubscribe(subscriber, sub_msg.event != 0);
	}
	/* ----------------------------------------------------------- */
}

	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

void DataManager::Subscribe(SubscriberPtr_t & subscr, bool event_based) {
	logger->Info("Subscribe: client <%s:%d>",
			subscr->ip_address.c_str(),subscr->port_num);

	subs_lock.lock();

	if (event_based) { // If event-based subscription
		auto sub = FindSubscriber(subscr, subscribers_on_event);
		// If the client is already a subscriber just update its event filter
		if (sub != nullptr) {
			logger->Debug("Subscribe: <%s> current events: %s",
				subscr->ip_address.c_str(),
				sub->subscription.event.to_string().c_str());

			sub->subscription.event |= subscr->subscription.event;
			logger->Debug("Subscribe: <%s> updated events: %s",
				subscr->ip_address.c_str(),
				sub->subscription.event.to_string().c_str());
		}
		else {
			// If new just add it to the list
			subscribers_on_event.push_back(subscr);
		}
	}
	else { // If rate-based subscription
		auto sub = FindSubscriber(subscr, subscribers_on_rate);
		// If the client is already a subscriber just update its filter and rate
		if (sub != nullptr) {
			logger->Debug("Subscribe: <%s> current filter: %s rate: %d ms",
				subscr->ip_address.c_str(),
				sub->subscription.filter.to_string().c_str(),
				sub->subscription.rate_ms);

			sub->subscription.filter |= subscr->subscription.filter;
			sub->subscription.rate_ms = subscr->subscription.rate_ms;
			sub->rate_deadline_ms     = sub->subscription.rate_ms;
			logger->Debug("Subscribe: <%s> updated filter: %s rate: %d m",
				subscr->ip_address.c_str(),
				sub->subscription.filter.to_string().c_str(),
				sub->subscription.rate_ms);
		}
		else {
			subscribers_on_rate.push_back(subscr);
			subs_cv.notify_one();
		}
		subscribers_on_rate.sort();
	}

	PrintSubscribers();
	subs_lock.unlock();
}

	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

void DataManager::Unsubscribe(SubscriberPtr_t & subscr, bool event_based) {
	logger->Info("Unsubscribe: client <%s:%d>",
	subs_lock.lock();
		subscr->ip_address.c_str(), subscr->port_num);
	if (event_based) {
		auto sub = FindSubscriber(subscr, subscribers_on_event);
		if (sub != nullptr) {
			sub->subscription.event &= ~(subscr->subscription.event);
			if (sub->subscription.event.none())
				subscribers_on_event.remove(sub);
		}
		else
			logger->Error("Unsubscribe: client not found!");
	}
	else {
		auto sub = FindSubscriber(subscr, subscribers_on_rate);
		if (sub != nullptr){
			sub->subscription.filter &= ~(subscr->subscription.filter);
			if (sub->subscription.filter.none())
				subscribers_on_rate.remove(sub);
			subscribers_on_rate.sort();
		}
		else
			logger->Error("Unsubscribe: client not found!");
	}

	PrintSubscribers();

	subs_lock.unlock();
}


/*******************************************************************/
/*                       Publishing handling                       */
/*******************************************************************/

void DataManager::Task() {
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);
		subs_lock.lock();
	logger->Debug("Task: starting worker...");
	while (true) {
		while (subscribers_on_rate.empty())
			subs_cv.wait(subs_lock);
		subs_lock.unlock();
		PublishOnRate();
		logger->Debug("Task: sleeping for %d...", sleep_time);
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
	}
}

void DataManager::NotifyUpdate(status_event_t event) {
	logger->Notice("NotifyUpdate: update notified: %d",event);
	std::unique_lock<std::mutex> events_lock(events_mtx, std::defer_lock);
	events_lock.lock();
	event_queue.push_back(event);
	events_lock.unlock();
	event_cv.notify_one();
}

void DataManager::EventHandler() {
	event_handler_tid = gettid();
	logger->Info("EventHandler: starting handler..,.[tid=%d]", event_handler_tid);
	std::unique_lock<std::mutex> events_lock(events_mtx, std::defer_lock);

	while (true) {
		std::vector<status_event_t> events_vec;
		events_lock.lock();
		while (event_queue.empty())
			event_cv.wait(events_lock);
		for (auto & event: event_queue)
			events_vec.push_back(event);
		event_queue.clear();
		events_lock.unlock();
		// Avoiding blocking function due to long execution time
		// of PublishOnEvent
		for (const auto & event: events_vec) {
			logger->Debug("EventHandler: psblishing for event %d", event);
			PublishOnEvent(event);
		}
	}
}

void DataManager::PublishOnEvent(status_event_t event){
	ExitCode_t result;
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);
	UpdateData(); // Update resources and applications data
	subs_lock.lock();
	for (const auto s : subscribers_on_event) {
		// If event matched
		if((s->subscription.event & bd::sub_bitset_t(event)) == bd::sub_bitset_t(event)){
			result = Push(s);
			if (result != OK) {
				logger->Error("PublishOnEvent: error in publish status to %s:%d",
					s->ip_address.c_str(),s->port_num);
			}
			else
				logger->Notice("PublishOnEvent: publish status to <%s:%d>",
					s->ip_address.c_str(),s->port_num);
		}
	}
	subs_lock.unlock();
}

void DataManager::PublishOnRate(){
	uint16_t tmp_sleep_time; // = sleep_time;
	ExitCode_t result;
	utils::Timer tmr;
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);
	tmp_sleep_time = subscribers_on_rate.back()->subscription.rate_ms;
	UpdateData(); // Update resources and applications data
	subs_lock.lock();
	tmr.start();
	for (auto s: subscribers_on_rate) {
		// Update deadline after sleep
		s->rate_deadline_ms = s->rate_deadline_ms - actual_sleep_time;
		logger->Debug("PublishOnRate: subscriber: <%s:%d> next_deadline: %d",
			s->ip_address.c_str(), s->port_num, s->rate_deadline_ms);

		// Deadline passed => push the updated information
		if (s->rate_deadline_ms <= 0) {
			result = Push(s);
			if (result != OK)
				logger->Error("PublishOnRate: error in publishing to <%s:%d>",
					s->ip_address.c_str(),s->port_num);
			else
				logger->Info("PublishOnRate: published information to <%s:%d>",
					s->ip_address.c_str(),s->port_num);

			// Reset the deadline
			s->rate_deadline_ms = s->subscription.rate_ms;
			logger->Debug("PublishOnRate: subscriber: <%s:%d> updated next_deadline: %d",
				s->ip_address.c_str(), s->port_num, s->rate_deadline_ms);
		}
		// Calculating the earlier sleep time the sleep time with the earlier
		if(s->rate_deadline_ms < tmp_sleep_time)
			tmp_sleep_time = s->rate_deadline_ms;
	}
	tmr.stop();

	// Updating the sleep time
	sleep_time = tmp_sleep_time - static_cast<uint16_t>(tmr.getElapsedTimeMs());
	actual_sleep_time = tmp_sleep_time;

	// Sort subscribers by deadline in ascending order
	subscribers_on_rate.sort();
	subs_lock.unlock();
}

DataManager::ExitCode_t DataManager::Push(SubscriberPtr_t sub) {
	status_message_t updated_status;
	updated_status.ts = static_cast<uint32_t>(utils::Timer::getTimestamp());
	updated_status.n_app_status_msgs = 0;
	updated_status.n_res_status_msgs = 0;
	logger->Debug("Push: timestamp=%d", updated_status.ts);

	// Subscriber has "resource" subscription?
	if ((sub->subscription.filter & bd::sub_bitset_t(FILTER_RESOURCE)) ==
		bd::sub_bitset_t(FILTER_RESOURCE)) {
		logger->Debug("Push: adding resources info for subscriber <%s>...",
			sub->ip_address.c_str());

		updated_status.n_res_status_msgs = num_resources;
		for (auto res_stat: res_stats)
			updated_status.res_status_msgs.push_back(res_stat);

		// Debug logging
		for (auto res_stat : updated_status.res_status_msgs) {
			logger->Debug("Resource: id=%ld, occupancy=%d, load=%d, power=%d, temp=%d",
				res_stat.id,
				res_stat.occupancy,
				res_stat.load,
				res_stat.power,
				res_stat.temp);
		}
	}

	// Subscriber filter has "application" subscription?
	if ((sub->subscription.filter & bd::sub_bitset_t(FILTER_APPLICATION)) ==
		bd::sub_bitset_t(FILTER_APPLICATION)) {
		logger->Debug("Push: adding applications info for subscriber <%s>...",
			sub->ip_address.c_str());

		updated_status.n_app_status_msgs = num_applications;
		for (auto app_stat : app_stats)
			updated_status.app_status_msgs.push_back(app_stat);

		// Debug logging
		for (auto app_stat : updated_status.app_status_msgs) {
			logger->Debug("Application: id=%d, name=%s, nr_tasks=%d, mapping=%d",
				app_stat.id,
				app_stat.name.c_str(),
				app_stat.n_task,
				app_stat.mapping);
			for(auto task_stat : app_stat.tasks){
				logger->Debug("Task: id=%d, perf:%d, mapping=%d",
				task_stat.id,
				task_stat.perf,
				task_stat.mapping);
			}
		}
	}

	// TCP socket for data messages
	tcp::iostream client_sock(sub->ip_address, std::to_string(sub->port_num));
	if (!client_sock) {
		logger->Error("Push: cannot connect to <%s:%>d",
			sub->ip_address.c_str(), sub->port_num);
		return ERR_CLIENT_COMM;
	}

	// Message serialization and transmission
	text_oarchive archive(client_sock);
	try {
		archive << updated_status;  // Send
	} catch(boost::exception const& ex){
		logger->Error("Boost archive exception");
		return ERR_CLIENT_COMM;
	}

	return OK;
}

res_bitset_t
DataManager::BuildResourceBitset(br::ResourcePathPtr_t resource_path) {
	res_bitset_t res_bitset = 0;
	for(auto resource_identifier: resource_path->GetIdentifiers()){
		switch(resource_identifier->Type()){
			case res::ResourceType::SYSTEM:
				res_bitset |= static_cast<uint64_t>(resource_identifier->ID()) << BITSET_OFFSET_SYS;
				break;
			case res::ResourceType::CPU: 
			case res::ResourceType::GPU:
			case res::ResourceType::ACCELERATOR:
			case res::ResourceType::MEMORY:
			case res::ResourceType::NETWORK_IF:
				res_bitset |= 
					static_cast<uint64_t>(resource_identifier->Type()) << BITSET_OFFSET_UNIT_TYPE;
				res_bitset |= static_cast<uint64_t>(resource_identifier->ID()) << BITSET_OFFSET_UNIT_ID;
				break;
			case res::ResourceType::PROC_ELEMENT:
				res_bitset |= static_cast<uint64_t>(1) << BITSET_OFFSET_PE_TYPE;
				res_bitset |= static_cast<uint64_t>(resource_identifier->ID()) << BITSET_OFFSET_PE_ID;
				break;
			default:
				break;
		}
	}
	return res_bitset;
}

void DataManager::UpdateData(){
	logger->Debug("UpdateData: Updating applications and resources data...");

	// Taking data from Resource accounter and Power Manager
	std::set<br::ResourcePtr_t> resource_set = ra.GetResourceSet();
	num_resources = resource_set.size();

	// Updating resource status list
	res_stats.clear();
	for (auto & resource_ptr : resource_set) {
		br::ResourcePathPtr_t resource_path = ra.GetPath(resource_ptr->Path());
		logger->Debug("UpdateData: <%ld>: used=%d  unreserved=%d total=%d",
			BuildResourceBitset(resource_path),
			resource_ptr->Used(),
			resource_ptr->Unreserved(),
			resource_ptr->Total());

#ifdef CONFIG_BBQUE_PM
		logger->Debug("UpdateData: <%ld>: temp=%d freq=%d power=%2.2f",
			BuildResourceBitset(resource_path),
			static_cast<uint32_t>(
				resource_ptr->GetPowerInfo(PowerManager::InfoType::TEMPERATURE, br::Resource::ValueType::INSTANT)),
			static_cast<uint32_t>(
				resource_ptr->GetPowerInfo(PowerManager::InfoType::FREQUENCY, br::Resource::ValueType::INSTANT)),
			static_cast<uint32_t>(
				resource_ptr->GetPowerInfo(PowerManager::InfoType::POWER, br::Resource::ValueType::INSTANT)));
#endif // CONFIG_BBQUE_PM

		resource_status_t temp_res;
		temp_res.id = BuildResourceBitset(resource_path);
		temp_res.occupancy = static_cast<uint8_t>(resource_ptr->Used());
#ifdef CONFIG_BBQUE_PM
		temp_res.load = static_cast<uint8_t>(
			resource_ptr->GetPowerInfo(PowerManager::InfoType::LOAD, br::Resource::ValueType::INSTANT));
		temp_res.power = static_cast<uint32_t>(
			resource_ptr->GetPowerInfo(PowerManager::InfoType::POWER, br::Resource::ValueType::INSTANT));
		temp_res.temp = static_cast<uint32_t>(
			resource_ptr->GetPowerInfo(PowerManager::InfoType::TEMPERATURE, br::Resource::ValueType::INSTANT));
#endif // CONFIG_BBQUE_PM
		res_stats.push_back(temp_res);
	}
}

} // namespace bbque