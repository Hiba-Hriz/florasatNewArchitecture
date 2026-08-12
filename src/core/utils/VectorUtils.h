/*
 * VecUtils.h
 *
 *  Created on: Mai 12, 2023
 *      Author: Robin Ohs
 */

#ifndef __FLORA_CORE_UTILS_VECTORUTILS_H_
#define __FLORA_CORE_UTILS_VECTORUTILS_H_

#include <string>
#include <sstream>

namespace flora {
namespace core {
namespace utils {
namespace vector {

template<typename T>
std::string toString(T begin, T end)
{
    std::stringstream ss;
    bool first = true;
    for (; begin != end; begin++)
    {
        if (!first)
            ss << ", ";
        ss << *begin;
        first = false;
    }
    return ss.str();
}

}  // namespace vector
}  // namespace utils
}  // namespace core
}  // namespace flora

#endif  // __FLORA_CORE_UTILS_VECTORUTILS_H_