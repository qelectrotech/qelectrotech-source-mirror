/*
 * waypoint.h
 *
 *  Created on: 6 nov. 2014
 *      Author: rene
 */

#ifndef WAYPOINT_H_
#define WAYPOINT_H_

#include "Mathlib.h"

class Waypoint;


class Waypoint 	{
	public:
						 Waypoint(vec2d, vec2d);
						~Waypoint();

	private:
		vec2d			next;

		vec2d			orig;
		vec2d			dest;
		vec2d			vDir;
		vec2d			vNor;

		Waypoint*		nexttLeft;
		Waypoint*		nexttRight;
};

#endif /* WAYPOINT_H_ */
