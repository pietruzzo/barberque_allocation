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

#include "bbque/utils/utility.h"

#include "bbque/data_manager.h"

#define DATA_MANAGER_NAMESPACE "bq.dm"
#define MODULE_NAMESPACE DATA_MANAGER_NAMESPACE

#define DEFAULT_SLEEP_TIME 1000

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
		while(any_subscriber){

		}
	}
}

void DataManager::SubscriptionHandler() {
	subscription_server_tid = gettid();
	logger->Info("Starting the subscription server... tid = %d", subscription_server_tid);

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

void DataManager::Subscribe(SubscriberPtr_t & subscr, bool event){
	std::unique_lock<std::mutex> subs_lock(subscribers_mtx, std::defer_lock);

	subs_lock.lock();
	any_subscriber = true;

	if(event) {
		subscribers_on_event.push_back(subscr);
	}
	else {
		subscribers_on_rate.push_back(subscr);
		logger->Debug("Sorting on rate...");
		subscribers_on_rate.sort();
	}


	for (auto s : subscribers_on_rate){
		logger->Debug("Subisccribers on rate: %s %d %s %d", 
			s->ip_address.c_str(), 
			s->rate_deadline_ms,
			s->subscription.filter.to_string().c_str(),
			s->subscription.rate_ms);	
	}


	subs_lock.unlock();
}

} // namespace bbque