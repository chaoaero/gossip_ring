/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     failure_detector.h
*   author:       Meng Weichao
*   created:      2016/01/25
*   description:  
*
================================================================*/
#ifndef __FAILURE_DETECTOR_H__
#define __FAILURE_DETECTOR_H__
#include<iostream>
#include<cmath>
#include<map>
#include<string>
#include "common/base/cxx11.h"
#include "arrival_window.h"
#include "common/base/stdext/shared_ptr.h"
#include "i_failure_detection_event_listener.h"

using namespace std;

class FailureDetector {
friend class ArrivalWindow; 

private:
    static const  int64_t INITIAL_VALUE_NANOS = 2000;
    //static const  double PHI_FACTOR = 1.0 / log(10.0); 
    static const  double PHI_FACTOR = 0.434294; 
    static const  int64_t SAMPLE_SIZE = 1000;
    static const  double  PHI_CONVICT_THRESHOLD= 8.0;
    map<string, stdext::shared_ptr<ArrivalWindow> > arrival_samples_;
    i_failure_detection_event_listener* fd_event_listener;

public:
    FailureDetector() {}
    ~FailureDetector() {}

    bool is_alive(string ep);
    void report(string ep);
    void interpret(string ep);
    void remove(string ep);
    void register_event_listener(i_failure_detection_event_listener* listener);
};

#endif //__FAILURE_DETECTOR_H__
