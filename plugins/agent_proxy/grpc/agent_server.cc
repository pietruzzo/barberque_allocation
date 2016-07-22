#include <iostream>

#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "agent_server.h"

namespace bbque
{
namespace plugins
{

AgentServer::AgentServer(std::string const & _server_addr_port):
	server_address_port(_server_addr_port),
	server_thread(&AgentServer::RunServer, this)
{
}

AgentServer::~AgentServer()
{
	server_thread.join();
}

void AgentServer::RunServer()
{
	grpc::ServerBuilder builder;
	builder.AddListeningPort(
	        server_address_port, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	server = builder.BuildAndStart();
	std::cout << "[INF] Server listening on " <<
	          server_address_port << std::endl;
	server->Wait();
}

void AgentServer::WaitToStop()
{
	server_thread.join();
	std::cout << "[INF] Sever is down" << std::endl;
}

void AgentServer::StopServer()
{
	std::cout << "[INF] Sever shutting down..." << std::endl;
//   server->Shutdown();
}

} // namespace plugins

} // namespace bbque
