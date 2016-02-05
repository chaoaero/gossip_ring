/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     arrival_window.h
*   author:       Meng Weichao
*   created:      2016/01/25
*   description:  
*
================================================================*/
#ifndef __ARRIVAL_WINDOW_H__
#define __ARRIVAL_WINDOW_H__
#include "bounded_stats_deque.h"
class ArrivalWindow {
private:
    long tLast ;
    static const int64_t max_interval_nano_seconds = 1000000000;
    bounded_stats_deque arrival_intervals_;
    //static  double PHI_FACTOR{1.0 / std::log(10.0)};
    static const  double PHI_FACTOR = 0.434294; 

public:
    ArrivalWindow(int64_t size, long tLast = 0) : arrival_intervals_(size), tLast(tLast) {}
    void add(long value);
    double mean();
    double phi(long tnow);
};

#endif //__ARRIVAL_WINDOW_H__
