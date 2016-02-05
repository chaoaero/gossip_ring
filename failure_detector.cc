/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     failure_detector.cc
*   author:       Meng Weichao
*   created:      2016/02/01
*   description:  
*
================================================================*/
#include "failure_detector.h"
#include "common/system/time/timestamp.h"
#include "thirdparty/glog/logging.h"
#include<iostream>

void FailureDetector::remove(string addr) {
    arrival_samples_.erase(addr);
}
void FailureDetector::report(string addr) {
    long now = GetTimeStampInUs();
    if( arrival_samples_.count(addr)) {
        stdext::shared_ptr<ArrivalWindow> lp = arrival_samples_[addr];
        lp->add(now);
    } else {
        stdext::shared_ptr<ArrivalWindow> heartbeat_window(new ArrivalWindow(SAMPLE_SIZE));
        heartbeat_window->add(now);
        arrival_samples_[addr] = heartbeat_window;
    }
}

void FailureDetector::interpret(string addr) {
    if(arrival_samples_.count(addr) == 0)
        return;
    stdext::shared_ptr<ArrivalWindow> hb_wind = arrival_samples_[addr];
    long now = GetTimeStampInUs();
    double phi = hb_wind->phi(now);
    LOG(INFO)<<" the phi times PHI_FACTOR is "<<PHI_FACTOR * phi<<" and threshold is "<<PHI_CONVICT_THRESHOLD;
    if(PHI_FACTOR * phi > PHI_CONVICT_THRESHOLD  ) {
        fd_event_listener->convict(addr, phi);
    } else {
        fd_event_listener->mark_alive(addr, phi);
    } 
}

bool FailureDetector::is_alive(string ep) {
    fd_event_listener->is_alive(ep); 
}

void FailureDetector::register_event_listener(i_failure_detection_event_listener* listener) {
    fd_event_listener = listener;
}
