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

#ifndef BBQUE_DATA_MANAGER_H_
#define BBQUE_DATA_MANAGER_H_

#include <list>
#include <bitset>
#include <iostream>
#include <thread>
#include <chrono>

#include "bbque/stat/types.h"
#include "bbque/cpp11/mutex.h"
#include "bbque/utils/worker.h"

namespace bu = bbque::utils;

namespace bbque { 

namespace data {

	typedef std::bitset<8> sub_bitset_t;

	typedef enum status_event {
		scheduling = 0,
		application,
		resource
	} status_event_t;

	class Subscription {
	public:
		Subscription(){};

		Subscription(std::string filter_bit, 
			std::string event_bit, 
			uint16_t rate_ms = -1){
				filter = sub_bitset_t(filter_bit);
				event = sub_bitset_t(event_bit);
				this->rate_ms = rate_ms;
		}

		sub_bitset_t filter;
		sub_bitset_t event;
		uint16_t rate_ms;
	};

	class Subscriber {
	public:
		Subscriber(){};

		Subscriber(std::string ip_address,
			uint32_t port_num,
			Subscription & sub){
				this->ip_address = ip_address;
				this->port_num = port_num;
				this->subscription = sub;
				this->rate_deadline_ms = sub.rate_ms;
		};

		std::string ip_address;
		uint32_t port_num;
		Subscription subscription;
		uint16_t rate_deadline_ms;

		bool cmp(Subscriber &s1, Subscriber &s2) {
			if(s1.rate_deadline_ms < s2.rate_deadline_ms)
				return true;
			return false;
		}
	};

} // namespace data

using bbque::data::Subscription;
using bbque::data::Subscriber;
using bbque::data::status_event_t;

typedef std::shared_ptr<Subscriber> SubscriberPtr_t;
typedef std::list<SubscriberPtr_t> SubscriberPtrList_t;

/**
 * @class DataManager
 *
 */
class DataManager: public utils::Worker {

public:
	DataManager();

	virtual ~DataManager();

	/**
	 * @brief Return a reference to the DataManager module
	 *
	 * @return a reference to the DataManger module
	 */
	static DataManager & GetInstance();


	/*
	 * @brief Notify an update event to the DataManager
	 *
	 * @param event: the notified event
	 */
	void NotifyUpdate(status_event_t event);

private:

	/** The logger used by the application manager */
	std::unique_ptr<bu::Logger> logger;

	std::string ip_address = "0.0.0.0";

	uint32_t port_num;

	std::thread subscription_server;

	/**
	 * @brief Condition variable to indicate the presence of any subscriber
	 */
	bool any_subscriber;

	/**
	 * @brief Time interval between different data publishing
	 */
	std::chrono::duration<int> sleep_time;

	/*
	 * @brief List of all rate-based subscribers
	 */
	SubscriberPtrList_t subscribers_on_rate;

	/*
	 * @brief List of all event-based subscribers
	 */
	SubscriberPtrList_t subscribers_on_event;

	/**
	 * Mutex to protect concurrent access to the subscribers lists.
	 */
	std::mutex subscribers_mtx;

	/**
	 * @brief The thread ID of the subscription server thread
	 */
	pid_t subscription_server_tid;

	/*
	 * @brief Publish the data to the @ref{subscribers_on_rate}
	 */
	void Publish();

	/*
	 * @brief Add subscription
	 *
	 * @param subscr: the subscriber to add
	 * @param event: true if the subscription refers to an event, false otherwise
	 */
	void Subscribe(SubscriberPtr_t & subscr, bool event);

	/*
	 * @brief Publish the data to the @ref{subscribers_on_event}
	 */
	void PublishOnEvent();

	/*
	 * @brief Server to handle the subscription/unsubscription requests
	 */	
	void SubscriptionHandler();


	void Task();

};

} // namespace bbque

#endif // BBQUE_DATA_MANAGER_H_