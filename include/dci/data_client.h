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

#ifndef BBQUE_DATA_CLIENT_H_
#define BBQUE_DATA_CLIENT_H_

#include "bbque/stat/types.h"

#include <functional>
#include <thread>

namespace bbque {

using namespace bbque::stat;

/*
 * @brief This class is the base class for third-party client to interface
 * with the BarbequeRTRM DataManager module.
 * It hides from the developers the connection and network management.
 * It provides a method to subscribe to specific event and information filter.
 * It allows the client to set a callback function to update the client when new status information
 * are published by BarbequeRTRM instance.
 * This class should be instantiated by the third-party client.
 */
class DataClient {
public: 

	/*
	 * @brief Subscription modes
	 *
	 */
	enum SubMode_t {
		SUBSCRIBE = 0,
		UNSUBSCRIBE = 1
	};

	/*enum ExitCode_t
	 *
	 * @brief Class specific return codes
	 */
	enum ExitCode_t {
		OK = 0,           /** Successful call */
		ERR_SERVER_COMM,  /** The server is unreachable */
		ERR_UNKNOWN       /** A not specified error code   */
	};

	DataClient() = delete;

	DataClient(std::string serverIP, uint32_t serverPort, uint32_t clientPort);

	virtual ~DataClient(){};

	inline void setClientPort(const uint32_t clientPort) noexcept {
		this->clientPort = clientPort;
	}

	/*
	 * @brief This method performs the connection with the BarbequeRTRM DataManager 
	 * server instance starting the receiver thread for incoming published updates.
	 * 
	 * @return A DataClient exit code. 0 if OK, 1 if ERR_SERVER_COMM.
	 */
	ExitCode_t Connect();

	/*
	 * @brief This method stops the receiver thread and closes connection to the
	 * BarbequeRTRM DataManager server instance.
	 * 
	 * @return A DataClient exit code. 0 if OK, 1 if ERR_SERVER_COMM.
	 */
	ExitCode_t Disconnect();

	/*
	 * @brief It performs the subscription for the Client to the BarbequeRTRM DataManager
	 * server instance basing on the given preferences.
	 * 
	 * @param filter The subscription target type filter
	 * @param event The event filter
	 * @param period The rate for the update
	 * @param mode The subscription mode according to SubMode_t
	 *
	 * @return A DataClient exit code. 0 if OK, 1 if ERR_SERVER_COMM.
	 */
	ExitCode_t Subscribe(status_filter_t filter, 
		status_event_t event, 
		uint16_t period, 
		SubMode_t mode);

	/*
	 * @brief With this method the client can set its callback function.
	 * The callback function is then invoked when new information message
	 * are published by the BarbequeRTRM server instance.
	 * The callback function has to be implemented by the third-party client
	 * and has to have a status message parameter.
	 * .
	 * @param client_callback The client's callback function.
	 */
	inline void SetCallback(std::function<void(status_message_t)> callback) noexcept {
		client_callback = callback;
	}

private:

	/*
	 * @brief This is the receiver loop-function to manage incoming information
	 * published by the DataManager server instance.
	 */
	void ClientReceiver();

	/*
	 * @brief This is the callback function of the third-party client
	 * It is called when new information are published by the 
	 * DataManager server instance.
	 */
	std::function<void(status_message_t)> client_callback;

	/* @brief IP address of the server */
	std::string serverIP;             

	/* @brief The TCP port of the server*/
	uint32_t serverPort;        

	/* @brief The TCP port of the client*/
	uint32_t clientPort;        

	/* @brief The client receiver thread */
	std::thread client_thread;

	/* @brief Flag to control the execution of the 
	 * receiver thread is started
	 */
	bool receiver_started;

};

} // namespace bbque

#endif // BBQUE_DATA_CLIENT_H_