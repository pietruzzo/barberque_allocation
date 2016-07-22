#ifndef BBQUE_AGENT_PROXY_GRPC_SERVER_H_
#define BBQUE_AGENT_PROXY_GRPC_SERVER_H_

#include <memory>
#include <string>
#include <thread>

#include <grpc/grpc.h>

#include "bbque/plugins/agent_proxy_types.h"
#include "agent_impl.h"
#include "agent_com.grpc.pb.h"

namespace bbque
{
namespace plugins
{

class AgentServer
{

public:
	AgentServer(std::string const & _server_addr_port);

	virtual ~AgentServer();

	void WaitToStop();

	void StopServer();

private:

	void RunServer();

	std::string server_address_port;
	std::thread server_thread;
	std::unique_ptr<grpc::Server> server;
	AgentImpl service;
};

} // namespace plugins

} // namespace bbque

#endif // BBQUE_AGENT_PROXY_GRPC_SERVER_H_
