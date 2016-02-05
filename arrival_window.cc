/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     arrival_window.cc
*   author:       Meng Weichao
*   created:      2016/02/01
*   description:  
*
================================================================*/
#include "arrival_window.h"
#include "common/base/stdext/shared_ptr.h"
#include "thirdparty/glog/logging.h"

void ArrivalWindow:: add(long value) {
    if(tLast > 0) {
        long inter_arrival_time = value - tLast;
        if(inter_arrival_time < ArrivalWindow::max_interval_nano_seconds) 
            arrival_intervals_.add(value);
    } else
        arrival_intervals_.add(ArrivalWindow::max_interval_nano_seconds); //
    tLast = value;
}

double ArrivalWindow::mean() {
   return arrival_intervals_.mean(); 
}

double ArrivalWindow::phi(long tnow) {
    if(tLast == 0)
        return 0;
    long t = tnow - tLast;
    if(mean() == 0) {
        LOG(INFO)<<"mean is zero---------";
        return 0;
    }
    return t / mean();
}

