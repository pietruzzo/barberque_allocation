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

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "bbque/stat/types.h"
#include "bbque/cpp11/mutex.h"
#include "bbque/utils/worker.h"
// Boost libraries
#include <boost/serialization/access.hpp>
#include <boost/serialization/list.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/asio.hpp>

namespace bu = bbque::utils;

using namespace bbque::stat;

namespace bbque { 

namespace data {

	using sub_bitset_t = std::bitset<8>;

	/**
	 * @brief Status event description
	 */
	/*enum status_event_t {
		SCHEDULING = 1,
		APPLICATION = 2,
		RESOURCE = 4
	};*/

	class Subscription {
	public:
		Subscription(){};

		Subscription(sub_bitset_t filter_bit, 
			sub_bitset_t event_bit, 
			uint16_t rate_ms = -1){
				filter = filter_bit;
				event = event_bit;
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

		bool operator== (const Subscriber &other) const{
			if(this == &other)
				return true;
			if(ip_address == other.ip_address && port_num == other.port_num)
				return true;
			return false;
		} 
	};

} // namespace data

using namespace bbque::data;
using namespace bbque::stat;

/* Type definition */

using SubscriberPtr_t = std::shared_ptr<Subscriber>;
using SubscriberPtrList_t = std::list<SubscriberPtr_t>;
using sockaddr_in_t = struct sockaddr_in;
using SubscriberListIt_t = std::list<SubscriberPtr_t>::iterator;

/**
 * @class DataManager
 *
 */
class DataManager: public utils::Worker {

public:
	/**
	 * @enum ExitCode_t
	 *
	 * @brief Class specific return codes
	 */
	enum ExitCode_t {
		OK = 0,           /** Successful call */
		ERR_CLIENT_COMM,  /** The client is unreachable */
		ERR_UNKNOWN       /** A not specified error code   */
	};

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

	

	uint32_t port_num;

	std::thread subscription_server;

	/**
	 * @brief Condition variable to indicate the presence of any subscriber
	 */
	uint32_t any_subscriber;

	/**
	 * @brief Time interval between different data publishing
	 */
	//std::chrono::milliseconds sleep_time;
	uint16_t sleep_time;

	/*
	 * @brief List of all rate-based subscribers
	 */
	SubscriberPtrList_t subscribers_on_rate;

	/*
	 * @brief List of all event-based subscribers
	 */
	SubscriberPtrList_t subscribers_on_event;
	//SubscriperPtrEvtMap_t subscribers_on_event;

	/**
	 * @brief Mutex to protect concurrent access to the subscribers lists.
	 */
	std::mutex subscribers_mtx;

	/**
	 * @brief The thread ID of the subscription server thread
	 */
	pid_t subscription_server_tid;

	/**
	 * @brief Utility template function to drop bits outside the range [R, L) == [R, L - 1]
	 * @brief R The rightmost bits of range
	 * @brief L The leftmost bits of range
	 * @brief N The size of the bitset
	 * @param b The original bitset
	 * @return A new bitset between the requested range
	 */
	template<std::size_t R, std::size_t L, std::size_t N>
	std::bitset<N> BitsetRange(std::bitset<N> b)
	{
	    static_assert(R <= L && L <= N, "invalid bitrange");
	    b >>= R;            // drop R rightmost bits
	    b <<= (N - L + R);  // drop L-1 leftmost bits
	    b >>= (N - L);      // shift back into place
	    return b;
	}

	/*
  	 * @brief Utility method to print all the current subscribers
  	 */
  	void PrintSubscribers();

  	/*
  	 * @brief Utility method to find a subscriber into a specific list.
  	 * The search is performed on the ip address.
	 *
	 * @param subscr The subscriber to search
	 * @param list The list in which to find the specified subscriber
	 *
	 * @return An iterator object pointing the found subscriber
	 *
  	 */
  	SubscriberPtr_t findSubscriber(SubscriberPtr_t & subscr,
  		SubscriberPtrList_t & list);

  	/*
  	 * @brief A pointer to the list of all application updated status structs
  	 */
  	std::list<app_status_t> app_stats;

  	/*
  	 * @brief A pointer to the list of all resources updated status structs
  	 */
  	std::list<resource_status_t> res_stats;

  	/*
  	 * @brief The number of current managed applications
  	 */
  	uint32_t num_current_res;

  	/*
  	 * @brief The number of current managed resources
  	 */
  	uint32_t num_current_apps;

	/*******************************************************************/
	/*                      Communication fields                       */
	/*******************************************************************/

	/* 
	 * @brief Socket descriptor
	*/
	int32_t sock;      
  	
  	/* 
	 * @brief Local address
	*/
  	sockaddr_in_t local_address;
  	
  	/* 
	 * @brief Client address
	*/
  	sockaddr_in_t client_addr;

  	/* 
	 * @brief Server port 
	*/
  	uint32_t server_port;
  	  	
  	/* 
	 * @brief Length of incoming message
	*/
  	uint32_t client_addr_size;
  	  	
  	/* 
	 * @brief IP address of server
	*/
  	std::string ip_address = "0.0.0.0";
  	  	
  	/* 
	 * @brief Size of received response
	*/
  	int32_t recv_msg_size;


	/*******************************************************************/
	/*                    Publish/Subscribe methods                    */
	/*******************************************************************/

	/*
	 * @brief Publish the data to the @ref{subscribers_on_rate}
	 */
	void PublishOnRate();

	/*
	 * @brief Add a subscription
	 *
	 * @param subscr: the subscriber to add
	 * @param event: true if the subscription refers to an event, false otherwise
	 */
	void Subscribe(SubscriberPtr_t & subscr, bool event);

	/*
	 * @brief Remove a subscription
	 */
	void Unsubscribe(SubscriberPtr_t & subscr, bool event);

	/*
	 * @brief Publish the data to the @ref{subscribers_on_event}
	 *
	 * @param event The event triggering the publication
	 */
	void PublishOnEvent(data::sub_bitset_t event);

	/**
	 * @brief Push the update to the specified subscriber based on its information filter
	 *
	 * @param sub The target subscriber
	 */
	ExitCode_t Push(SubscriberPtr_t sub);

	/**
	 * @brief Update the content of resource and application data information
	 */
	void UpdateData();

	/*
	 * @brief Server to handle the subscription/unsubscription requests
	 */	
	void SubscriptionHandler();


	void Task();

};

} // namespace bbque

#endif // BBQUE_DATA_MANAGER_H_