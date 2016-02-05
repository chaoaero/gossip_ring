/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     i_failure_detection_event_listener.h
*   author:       Meng Weichao
*   created:      2016/02/04
*   description:  
*
================================================================*/
#ifndef __I_FAILURE_DETECTION_EVENT_LISTENER_H__
#define __I_FAILURE_DETECTION_EVENT_LISTENER_H__

#include<string>
#include<iostream>
using namespace std;

class i_failure_detection_event_listener {
public:
    virtual ~i_failure_detection_event_listener() {}

    virtual void convict(string ep, double phi) = 0;
    virtual bool is_alive(string ep) = 0;
    virtual void mark_alive(string& ep, double phi) = 0;

};

#endif //__I_FAILURE_DETECTION_EVENT_LISTENER_H__
