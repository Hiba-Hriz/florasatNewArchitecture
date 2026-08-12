/*
 * Subject.h
 *
 *  Created on: Jul 25, 2017
 *      Author: FRaverta
 */

#ifndef __FLORA_ROUTING_SUBJECT_H_
#define __FLORA_ROUTING_SUBJECT_H_

#include <forward_list>
#include "Observer.h"

namespace flora {
namespace routing {

using namespace std;

class Subject
{
public:
	Subject();
	virtual ~Subject();

	void addObserver(Observer * o);
	void removeObserver(Observer * o);

protected:
	void notify();

private:
	forward_list<Observer *> observerList_;
};

}  // namespace routing
}  // namespace flora


#endif /* __FLORA_ROUTING_SUBJECT_H_ */
