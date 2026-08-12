/*
 * Observer.h
 *
 *  Created on: Jul 25, 2017
 *      Author: FRaverta
 */

#ifndef __FLORA_ROUTING_OBSERVER_H_
#define __FLORA_ROUTING_OBSERVER_H_

namespace flora {
namespace routing {

class Observer
{
public:

	virtual void update(void) = 0;
};

}  // namespace routing
}  // namespace flora

#endif /* SRC_UTILS_OBSERVER_H_ */
