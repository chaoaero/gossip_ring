/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     client.cc
*   author:       Meng Weichao
*   created:      2016/01/23
*   description:  
*
================================================================*/
#include "common/base/string/algorithm.h"
#include "common/system/concurrency/this_thread.h"

// rpc client related headers.
#include "gossip_ring/gossip_ring.pb.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"

// include concurrency related headers.
#include "common/system/concurrency/atomic/atomic.h"

// includes from thirdparty
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
#include "common/base/closure.h"

// using namespace common;

DEFINE_string(server_address, "10.136.127.187:10000", "the address server listen on.");

Atomic<int> g_request_count;

void EchoCallback(poppy::RpcController* controller, GossipDigestSync* request, GossipDigestAck* response)
{
    LOG(INFO)<<" begin to execute callback";
    if (controller->Failed()) {
        LOG(INFO) << "failed: " << controller->ErrorText();
    } else {
        LOG(INFO) << "response: " << response->digests(0).endpoint();
        LOG(INFO) << "succeed";
    }

    delete controller;
    delete request;
    delete response;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);
    //google::InitGoogleLogging(argv[0]);

    // Define an rpc client. It's shared by all rpc channels.
    poppy::RpcClient rpc_client;

    // Define an rpc channel. It connects to a specified group of servers.
    poppy::RpcChannelOptions options;
    options.set_tos(148);
    poppy::RpcChannel rpc_channel(&rpc_client, FLAGS_server_address, NULL, options);

    // Define an rpc stub. It use a specifed channel to send requests.
    GossipRingServer::Stub gossip_client(&rpc_channel);
    poppy::RpcController* rpc_controller = new poppy::RpcController();
    GossipDigestSync* request = new GossipDigestSync();
    GossipDigestAck* response = new GossipDigestAck();
    google::protobuf::Closure* done =
        NewClosure(&EchoCallback, rpc_controller, request, response);

    //request->set_cluster_id("haomatong");
    //request->set_partitioner("Murmur3Partitioner");
    request->set_from("client");
    gossip_client.GossipDigestSynHandle(rpc_controller, request, response, done);
    LOG(INFO) << "receiving request: ";

    return EXIT_SUCCESS;

}
