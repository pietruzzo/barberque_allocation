
if (NOT CONFIG_BBQUE_AGENT_PROXY_GRPC)
  return (grpc)
endif (NOT CONFIG_BBQUE_AGENT_PROXY_GRPC)

if (CONFIG_BBQUE_AGENT_PROXY_DEFAULT_GRPC)
  set (BBQUE_AGENT_PROXY_DEFAULT "grpc" CACHE STRING "Setting agent proxy plugin name" FORCE)
endif (CONFIG_BBQUE_AGENT_PROXY_DEFAULT_GRPC)

set (BBQUE_AGENT_PROXY_PLUGIN bbque_agent_proxy_grpc)
set (BBQUE_AGENT_PROXY_PLUGIN_SOURCE_DIR ${PROJECT_SOURCE_DIR}/plugins/agent_proxy/grpc)
set (BBQUE_AGENT_PROXY_PLUGIN_SOURCE
  agent_impl agent_client agent_server agent_proxy)

add_library(${BBQUE_AGENT_PROXY_PLUGIN} ${BBQUE_AGENT_PROXY_PLUGIN_SOURCE})

# Build the message protocol buffer file and generate the gRPC code
set(PROTO_DIR proto)
set(PROTO_LIB proto_gen_agent)
add_subdirectory(${PROTO_DIR})

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROTO_DIR}
    ${GOOGLE_DIR}/include
)

target_link_libraries(${BBQUE_AGENT_PROXY_PLUGIN}
    ${GRPC_LIB}
    ${GRPCXX_LIB}
    ${PROTOBUF_LIB}
    ${PROTO_LIB}
    pthread
    dl
)
