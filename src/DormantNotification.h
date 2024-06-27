/*
 * DormantNotification.h
 *
 * Interface for Dormant Notification
 *
 *  Created on: 26 Jun 2024
 *      Author: jondurrant
 */

#ifndef SRC_DORMANTNOTIFICATION_H_
#define SRC_DORMANTNOTIFICATION_H_

#include "pico/stdlib.h"


class DormantNotification {
public:
	DormantNotification();
	virtual ~DormantNotification();

	virtual void notifyDormant(uint minutes);

	virtual void notifyWake(uint minutes);
};

#endif /* SRC_DORMANTNOTIFICATION_H_ */
