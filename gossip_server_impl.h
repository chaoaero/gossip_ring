/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     gossip_server_impl.h
*   author:       Meng Weichao
*   created:      2016/01/25
*   description:  
*
================================================================*/
#ifndef __GOSSIP_SERVER_IMPL_H__
#define __GOSSIP_SERVER_IMPL_H__
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "gossip_ring/gossip_ring.pb.h"
#include "poppy/rpc_server.h"
#include "common/system/time/timestamp.h"
#include "common/system/timer/timer_manager.h"
#include "common/system/concurrency/mutex.h"
#include "common/base/string/algorithm.h"

// includes from thirdparty
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "gossip_server_impl.h"
#include "failure_detector.h"
#include "i_failure_detection_event_listener.h"
#include "common/base/stdext/shared_ptr.h"

using namespace common;
using namespace std;

class GossipRingServerImpl : public GossipRingServer, public i_failure_detection_event_listener {
friend class FailureDetector;
public:
    GossipRingServerImpl(string& server_address, string& seeds_list, string& tokens); 
    ~GossipRingServerImpl();

    virtual void GossipDigestSynHandle(google::protobuf::RpcController* controller, const GossipDigestSync* request, GossipDigestAck* response, google::protobuf::Closure* done) {
        process_gossip_digest_sync(request, response);
        done->Run();
    } 

    virtual void GossipDigestAckHandle(google::protobuf::RpcController* controller, const GossipDigestAck2* request, Ack2Status* response, google::protobuf::Closure* done) {
        process_gossip_digest_ack2(request, response);
        done->Run();
    }
    void process_gossip_digest_sync(const GossipDigestSync* request, GossipDigestAck* response);
    void process_gossip_digest_ack2(const GossipDigestAck2* request, Ack2Status* response);

    void do_gossip_to_seeds(GossipDigestSync* gossip_digests_sync); 
    void do_gossip_to_unreachable_number(GossipDigestSync* gossip_digests_sync);
    bool do_gossip_to_live_number(GossipDigestSync* gossip_digests_sync);
    bool send_gossip(GossipDigestSync* gossip_digests_sync, set<string>& candidates);
    void make_random_gossip_digest(GossipDigestSync* request);
    void do_status_check();

    bool Init();
    uint32_t HashFunction(const char *key, uint32_t len);

    void Run( uint64_t timer_id); 
    int64_t getCurrentTimestamp(); 
    void update_heartbeat(uint64_t timer_id);
    void GossipSynCallback(poppy::RpcController* controller, GossipDigestSync* request, GossipDigestAck* response); 
    void GossipAck2Callback(poppy::RpcController* controller, GossipDigestAck2* request, Ack2Status* response);
    void display_endpoint(EndpointState* ep, string& addr);
    virtual void convict(string ep, double phi);
    virtual bool is_alive(string ep);
    void mark_dead(string& ep);
    virtual void mark_alive(string& ep, double phi);
    int64_t get_max_endpoint_state_version(string& addr);
    void notify_failure_detector(string& addr, const EndpointState& es);
    

private:
    TimerManager timer_manager_;
    std::map<string, stdext::shared_ptr<EndpointState> > endpoint_state_map_; //used for recording status info of each known nodes;
    set<string> seeds_;
    string local_address_; // local IP address
    set<string> tokens_;
    set<string> live_endpoints_; // known live endpoints
    set<string> unreachable_endpoints_; //unreachable endpoints 
    set<string> expire_time_endpoint_map_; //  
    Mutex endpoint_map_mutex_;
    FailureDetector fd_;
};


#endif //__GOSSIP_SERVER_IMPL_H__
