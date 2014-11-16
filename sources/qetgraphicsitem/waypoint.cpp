/*
 * waypoint.cpp
 *
 *  Created on: 6 nov. 2014
 *      Author: rene
 */




#include "waypoint.h"




Waypoint::Waypoint( vec2d o, vec2d d)	{
	orig = o;
	dest = d;

	vDir = vec2d( orig, dest );
	vNor = vDir.vnormalize();

	nexttLeft = NULL;
	nexttRight = NULL;
}



