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

}

} // namespace bbque