/*
 * Timer.h
 *
 *  Created on: Apr 06, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_CORE_TIMER_H_
#define __FLORA_CORE_TIMER_H_

#include <chrono>

using namespace std::chrono;

namespace flora {
namespace core {

class Timer {
   public:
    Timer() {
        reset();
    }

    float getTime() {
        time_point<high_resolution_clock> now = high_resolution_clock::now();
        return duration_cast<std::chrono::nanoseconds>(now - start).count();
    }

    void reset() {
        start = high_resolution_clock::now();
    }

   private:
    time_point<high_resolution_clock> start;
};

}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_THREADPOOL_H_