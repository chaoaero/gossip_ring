/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     gossip_server_impl.cc
*   author:       Meng Weichao
*   created:      2016/01/25
*   description:  
*
================================================================*/
#include "common/system/concurrency/this_thread.h"

#include "gossip_server_impl.h"
#include "common/system/net/ip_address.h"
#include <iostream>
#include <memory>
#include <vector>
#include<cstdlib>
#include <cmath>
#include<ctime>
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
#include "thirdparty/jsoncpp/json.h"

// rpc client related headers.
#include "gossip_ring/gossip_ring.pb.h"
#include "poppy/rpc_channel.h"
#include "poppy/rpc_client.h"
#include "common/base/cxx11.h"
#include "common/base/closure.h"

using namespace common;
using namespace std;
using google::protobuf::RepeatedPtrField;

GossipRingServerImpl::GossipRingServerImpl(string& server_address, string& seeds_list, string& tokens): local_address_(server_address) {

    SplitStringToSet(seeds_list, ",", &seeds_);
    SplitStringToSet(tokens, ",", &tokens_);

    //construct endpointstate for local server
    stdext::shared_ptr<EndpointState> local_ep(new EndpointState());
    local_ep->set_heart_beat_state(getCurrentTimestamp());
    local_ep->set_is_alive(true);
    // add TOKENS application state
    ApplicationStateVersionEntry* entry = local_ep->add_a_state_version_value();
    entry->set_application_state("TOKEN");
    VersionValue* vv = entry->mutable_version_value();
    vv->set_value(tokens);
    vv->set_version(local_ep->heart_beat_state());
    
    // add MOCK STATUS application state
    entry = local_ep->add_a_state_version_value();
    entry->set_application_state("STATUS");
    vv = entry->mutable_version_value();
    vv->set_value("alive");
    vv->set_version(local_ep->heart_beat_state());
    // debug use, check local application state
    display_endpoint(local_ep.get(), local_address_);

    endpoint_state_map_[local_address_] = local_ep;

    LOG(INFO)<<"constructor func";

    timer_manager_.AddPeriodTimer(1000, Bind(&GossipRingServerImpl::Run, this));
    timer_manager_.AddPeriodTimer(1000, Bind(&GossipRingServerImpl::update_heartbeat, this));

    // register event listener
    fd_.register_event_listener(this);
}

void GossipRingServerImpl::display_endpoint(EndpointState* ep, string& addr) {
    LOG(INFO)<<"display the endpoint state for :"<<addr<<"-------";
    for(int i = 0; i < ep->a_state_version_value_size(); i++) {
        LOG(INFO)<<"the "<<i + 1<<"th application state: ";
        ApplicationStateVersionEntry entry = ep->a_state_version_value(i);
        LOG(INFO)<<entry.application_state()<<"  "<<entry.version_value().value()<<"  "<<entry.version_value().version();
    }

}

GossipRingServerImpl::~GossipRingServerImpl() {
    timer_manager_.Stop();
    timer_manager_.Stop();

}

void GossipRingServerImpl::Run( uint64_t timer_id) {
    LOG(INFO)<<"------print known nodes----------";
    for(map<string, stdext::shared_ptr<EndpointState> >::const_iterator iter = endpoint_state_map_.begin(); iter != endpoint_state_map_.end(); iter++) {
        LOG(INFO)<<iter->first;
        string add(iter->first);
        display_endpoint(iter->second.get(), add);
    }
    //LOG(INFO)<<"------end print -----------------";
    //if(seeds_.count(local_address_) > 0) {
    //    LOG(INFO) << "this is seed node should not call itself";
    //    return;
    //}
    GossipDigestSync* request = new GossipDigestSync();
    make_random_gossip_digest(request); 
    bool gossip_to_seed = do_gossip_to_live_number(request);
    do_gossip_to_unreachable_number(request);
    if(!gossip_to_seed || live_endpoints_.size() < seeds_.size())
        do_gossip_to_seeds(request);
    do_status_check();
    LOG(INFO)<<"=============================================================================================";
}

void GossipRingServerImpl::do_status_check() {
    for(map<string, stdext::shared_ptr<EndpointState> >::const_iterator iter = endpoint_state_map_.begin(); iter != endpoint_state_map_.end(); iter++) {
        string endpoint = iter->first;
        stdext::shared_ptr<EndpointState> ep = iter->second;
        LOG(INFO)<<"-----now we do status check -----------for "<<endpoint<<"-------";
        if(endpoint == local_address_)
            continue;
        fd_.interpret(endpoint);
    }     
}

void GossipRingServerImpl::convict(string ep, double phi){
    if(endpoint_state_map_.count(ep) < 1)
        return;
    LOG(INFO)<<"---------------now make node "<<ep<<" dead with phi value "<<phi<<" ---------------------";
    mark_dead(ep); 
}

bool GossipRingServerImpl::is_alive(string ep) {
    if(endpoint_state_map_.count(ep) < 1) {
        LOG(INFO)<<"-------- node "<<ep<<" does not in ep map, we don't know the status of it";
        return false;
    }
    stdext::shared_ptr<EndpointState> p = endpoint_state_map_[ep];
    return p->is_alive();
}

void GossipRingServerImpl::mark_dead(string& ep) {
    live_endpoints_.erase(ep);
    unreachable_endpoints_.insert(ep);
    stdext::shared_ptr<EndpointState> ep_ptr = endpoint_state_map_[ep]; 
    ep_ptr->set_is_alive(false);
}

void GossipRingServerImpl::mark_alive(string& ep, double phi) {
    LOG(INFO)<<"----------- now the node "<<ep<<"recovers with phi value  "<<phi<<"------";
    unreachable_endpoints_.erase(ep);
    live_endpoints_.insert(ep);
    stdext::shared_ptr<EndpointState> ep_ptr = endpoint_state_map_[ep]; 
    ep_ptr->set_is_alive(true);
}

void GossipRingServerImpl::do_gossip_to_unreachable_number(GossipDigestSync* request) {
    int live_size = live_endpoints_.size();
    int unreachable_size = unreachable_endpoints_.size();
    double prob = (double)live_size / (double)(unreachable_size + 1);
    srand((unsigned)time(NULL));
    double rd = (double)rand() / (double)RAND_MAX;
    if(rd < prob) {
        send_gossip(request, unreachable_endpoints_);
    }
     
}

bool GossipRingServerImpl::do_gossip_to_live_number(GossipDigestSync* request) {
    int size = live_endpoints_.size();
    if(size < 1) {
        LOG(INFO)<<"------there are no nodes in live_endpoints_ ---------";
        return false;
    }
    return send_gossip(request, live_endpoints_);
}

void GossipRingServerImpl::make_random_gossip_digest(GossipDigestSync* request) {
    // construct syn message
    request->set_from(local_address_);
    for(map<string, stdext::shared_ptr<EndpointState> >::const_iterator iter = endpoint_state_map_.begin(); iter != endpoint_state_map_.end(); iter++) {
        GossipDigest* digest = request->add_digests();
        digest->set_endpoint(iter->first);
        int64_t max_version = 0;
        for(int i = 0; i < iter->second->a_state_version_value_size(); i++ ) {
            int64_t temp_version = iter->second->a_state_version_value(i).version_value().version();
            if(temp_version > max_version )
                max_version = temp_version;
        }
        digest->set_max_version(max_version);
    }
}


void GossipRingServerImpl::do_gossip_to_seeds(GossipDigestSync* gossip_digests_sync) {
    int size = seeds_.size();
    if(size > 0) {
        if(size == 1 && seeds_.count(local_address_) > 0)
            return;
        if(live_endpoints_.size() == 0)
            send_gossip(gossip_digests_sync, seeds_);
        else {
            double prob = seeds_.size() / (double)(live_endpoints_.size() + unreachable_endpoints_.size());
            // generate rand between 0 ~ 1
            srand((unsigned)time(NULL));
            double rd = (double)rand() / (double)RAND_MAX;
            if(rd <= prob) {
                LOG(INFO)<<" do_gossip_to_seeds: live_endpoints_ "<<live_endpoints_.size()<<" unreachable_endpoints_ : "<<unreachable_endpoints_.size();
                send_gossip(gossip_digests_sync, seeds_);
            }
        }
    }

}

bool GossipRingServerImpl::send_gossip(GossipDigestSync* gossip_digests_sync, set<string>& candidates) {
    vector<string> ep_list(candidates.begin(), candidates.end());
    int size = ep_list.size();
    if(size < 1) {
        LOG(INFO)<<"no data in candidate set";
        return false;
    }
    srand((unsigned)time(NULL));
    int index = rand() % size;
    string target_ep = ep_list[index];
    LOG(INFO)<<"-------------begin gossiping with node: "<<target_ep<<"------------------------------------------";
    // Initialize the rpc client
    // Define an rpc client. It's shared by all rpc channels.
    poppy::RpcClient rpc_client;
    // Define an rpc channel. It connects to a specified group of servers.
    poppy::RpcChannelOptions options;
    options.set_tos(148);
    poppy::RpcChannel rpc_channel(&rpc_client, target_ep , NULL, options);
    GossipRingServer::Stub gossip_client(&rpc_channel);
    // Define an rpc stub. It use a specifed channel to send requests.
    //gossip_client_.reset(&gossip_client);
    // now send sync request
    poppy::RpcController* rpc_controller = new poppy::RpcController();
    GossipDigestAck* response = new GossipDigestAck();
    google::protobuf::Closure* done =
        NewClosure(this, &GossipRingServerImpl::GossipSynCallback, rpc_controller, gossip_digests_sync, response);
    gossip_digests_sync->set_from(local_address_);
    gossip_client.GossipDigestSynHandle(rpc_controller, gossip_digests_sync, response, done);   
    return seeds_.count(target_ep) > 0;
}

void GossipRingServerImpl::process_gossip_digest_ack2(const GossipDigestAck2* request, Ack2Status* response) {
    LOG(INFO) << "------------ack2-------------- receiving request form "<<request->from();
    LOG(INFO) <<" we receive "<<request->map_size()<<" nums endpoint_state_map";
    for(int i = 0; i< request->map_size(); i++) {
        EndpointStateEntry tmp = request->map(i);
        string addr(tmp.endpoint());
        LOG(INFO)<<"---- receiving the "<< i + 1<<" endpointstate";
        LOG(INFO)<<"address: "<<addr<<" status"<<tmp.endpoint_state().is_alive();

        notify_failure_detector(addr, tmp.endpoint_state());

        LOG(INFO)<<"---- apply those state locally ---------";
        endpoint_state_map_[addr] = stdext::shared_ptr<EndpointState> (new EndpointState(tmp.endpoint_state()) );
    }
    response->set_succeed(true);
    response->set_from(local_address_);
    LOG(INFO)<<"----------------end ack2 ------------------";
}
/*
Sort gossip digest list according to the difference in max version number between sender's digest and our own information in descending order. That is, handle those digests first that differ mostly in version number. Number of endpoint information that fits in one gossip message is limited. This step is to guarantee that we favor sending information about nodes where information difference is biggest (sending node has very old information compared to us).
*/
void GossipRingServerImpl::process_gossip_digest_sync(const GossipDigestSync* request, GossipDigestAck* response) {
    LOG(INFO)<<"--------------syn-------------- receiving request from "<<request->from();

    //live_endpoints_.insert(request->from());
    map<string, GossipDigest> raw_digests;
    for(int i = 0; i < request->digests_size(); i++) {
        raw_digests[request->digests(i).endpoint()] = request->digests(i);
    }

    map<int64_t, string, std::greater<int64_t> > diff_digests;
    for(int i = 0; i < request->digests_size(); i++) {
        string addr = request->digests(i).endpoint();
        int64_t v = get_max_endpoint_state_version(addr); 
        diff_digests[abs(v - request->digests(i).max_version())] = addr;
    }

    vector<GossipDigest> sorted_digests;
    
    for(map<int64_t, string>::const_iterator iter = diff_digests.begin(); iter != diff_digests.end(); iter++) {
        sorted_digests.push_back(raw_digests[iter->second]); 
    }

    for(vector<GossipDigest>::const_iterator iter = sorted_digests.begin(); iter != sorted_digests.end(); iter++) {
        int64_t remote_max_version = iter->max_version(); 
        string remote_addr = iter->endpoint();
        if(endpoint_state_map_.count(remote_addr)) {
            LOG(INFO)<<"-----contain it------"<<remote_addr;
            int64_t local_max_version = get_max_endpoint_state_version(remote_addr);
            if(remote_max_version > local_max_version) {
                GossipDigest* digest = response->add_digests();
                digest->set_endpoint(remote_addr);
                digest->set_max_version(local_max_version);
            } else {
                EndpointStateEntry *entry = response->add_map(); 
                entry->set_endpoint(remote_addr);
                EndpointState* es = entry->mutable_endpoint_state();
                stdext::shared_ptr<EndpointState> lp = endpoint_state_map_[iter->endpoint()];
                es->CopyFrom(*lp.get());
                //display_endpoint(es, remote_addr);
            }
        } else {
            LOG(INFO)<<"-----not contain it-----"<<remote_addr;
            GossipDigest *digest = response->add_digests();
            digest->set_endpoint(iter->endpoint());
            digest->set_max_version(0);
        }
        
    }  
    response->set_from(local_address_);
}

int64_t GossipRingServerImpl::get_max_endpoint_state_version(string& addr) {
    if(endpoint_state_map_.count(addr) < 1)
        return 0;
    stdext::shared_ptr<EndpointState> p = endpoint_state_map_[addr];
    int max = p->heart_beat_state(); 
    for(int i = 0; i < p->a_state_version_value_size(); i++) {
        int64_t tmp_v = p->a_state_version_value(i).version_value().version();
        max = tmp_v > max ? tmp_v : max;
    }
    return max;
}

void GossipRingServerImpl::notify_failure_detector(string& addr, const EndpointState& es) {
    LOG(INFO)<<"notify_failure_detector------"<<addr;
    fd_.report(addr);
    //if(endpoint_state_map_.count(addr)) {
    //    int64_t remote_max_version = es.heart_beat_state();
    //    if(remote_max_version > get_max_endpoint_state_version(addr)) {
    //        fd_.report(addr);
    //    }
    //} else {
    //    fd_.report(addr);
    //}

}

void GossipRingServerImpl::GossipSynCallback(poppy::RpcController* controller, GossipDigestSync* request, GossipDigestAck* response) {
    LOG(INFO) <<" begin to execute sync callback";
    if (controller->Failed()) {
        LOG(INFO) << "failed: " << controller->ErrorText();
        return;
    } else {
        LOG(INFO) << "----------- response from: " << response->from() <<"---------";
        LOG(INFO) << "succeed";
    }

    for(int i = 0; i< response->map_size(); i++) {
        EndpointStateEntry tmp = response->map(i);
        string addr(tmp.endpoint());
        LOG(INFO)<<"---- receiving the "<< i + 1<<" endpoint state";
        LOG(INFO)<<"address: "<<addr<<" status"<<tmp.endpoint_state().is_alive();
        LOG(INFO)<<"---- apply those state locally ---------";
        endpoint_state_map_[addr] = stdext::shared_ptr<EndpointState> (new EndpointState(tmp.endpoint_state()) );
        notify_failure_detector(addr, tmp.endpoint_state());
    }
    
    // Define an rpc client. It's shared by all rpc channels.
    poppy::RpcClient rpc_client;
    // Define an rpc channel. It connects to a specified group of servers.
    poppy::RpcChannelOptions options;
    options.set_tos(148);
    poppy::RpcChannel rpc_channel(&rpc_client, response->from(), NULL, options);
    GossipRingServer::Stub gossip_client(&rpc_channel);
    poppy::RpcController* rpc_controller = new poppy::RpcController();
    GossipDigestAck2 *req = new GossipDigestAck2();
    LOG(INFO)<<"-----digest size -------------"<<response->digests_size();
    for(int i = 0; i < response->digests_size(); i++) {
        GossipDigest digest = response->digests(i);
        string remote_addr = digest.endpoint();
        EndpointStateEntry* entry = req->add_map();
        entry->set_endpoint(remote_addr);
        EndpointState* st = entry->mutable_endpoint_state();
        stdext::shared_ptr<EndpointState> lp = endpoint_state_map_[remote_addr];
        st->CopyFrom(*lp.get());
        display_endpoint(st, remote_addr);
    }
    
    req->set_from(local_address_);
    Ack2Status *rsp = new Ack2Status();
    google::protobuf::Closure* done =
        NewClosure(this, &GossipRingServerImpl::GossipAck2Callback, rpc_controller, req,rsp);

    gossip_client.GossipDigestAckHandle(rpc_controller, req, rsp, done);
    
    delete controller;
    delete request;
    delete response;
}


void GossipRingServerImpl::GossipAck2Callback(poppy::RpcController* controller, GossipDigestAck2* request, Ack2Status* response) {
    LOG(INFO) <<" begin to execute ack2 callback";
    if (controller->Failed()) {
        LOG(INFO) << "failed: " << controller->ErrorText();
    } else {
        LOG(INFO) << "response: " << response->from() <<" status "<<response->succeed();
        LOG(INFO) << "succeed";
    }
    delete controller;
    delete request;
    delete response;
}


void GossipRingServerImpl::update_heartbeat(uint64_t timer_id) {
    stdext::shared_ptr<EndpointState> ep = endpoint_state_map_[local_address_];
   // LOG(INFO)<<"the local heartbeat is " <<ep->heart_beat_state();
    ep->set_heart_beat_state(ep->heart_beat_state() + 1);
    //LOG(INFO)<<"update the local  heartbeat";
    LOG(INFO)<<"now the heartbeat is " <<ep->heart_beat_state();
}

int64_t GossipRingServerImpl::getCurrentTimestamp() {
    return (GetTimeStampInMs() / 1000);
}

bool GossipRingServerImpl::Init() {
    LOG(INFO)<<"init";
    return true;
}

