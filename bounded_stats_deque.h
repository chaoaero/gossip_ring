/*==================================================================
*   Copyright (C) 2016 All rights reserved.
*   
*   filename:     bounded_stats_deque.h
*   author:       Meng Weichao
*   created:      2016/02/01
*   description:  
*
================================================================*/
#ifndef __BOUNDED_STATS_DEQUE_H__
#define __BOUNDED_STATS_DEQUE_H__
#include<deque>

class bounded_stats_deque {
private:
    std::deque<long> deque_;
    long _sum ;
    int64_t max_size_;
public:
    bounded_stats_deque(int64_t size, long sum = 0): max_size_(size), _sum(sum) {}

    int64_t size() {
        return deque_.size();
    }

    void add(long i) {
        if(size() >= max_size_) {
            long removed = deque_.front();
            deque_.pop_front();
            _sum -= removed;
        }
        deque_.push_back(i);
        _sum += i;
    }

    long sum() {
        return _sum;
    }

    double mean() {
        return size() > 0 ? ((double)sum()) / size() : 0 ;
    }

    const std::deque<long>& deque() const {
        return deque_;
    }

};

#endif //__BOUNDED_STATS_DEQUE_H__
