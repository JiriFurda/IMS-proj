#ifndef __MAIN_H_INCLUDED__
#define __MAIN_H_INCLUDED__

#include "simlib.h"
#include <iostream>


using namespace std;

#ifdef DEBUG_BUILD
	#define DEBUG(x) do { std::cerr << "DEBUG: " << Time << " " << x; } while (0)
#else
	#define DEBUG(x)
#endif



class Drone : public Facility
{
	public:
		int maxFlyingRange;	// Capacity of the battery (in meters)
		int currentFlyingRange;	// Current state of the battery (in meters)
		int speed; 

		Drone(void);
};


class Package : public Process
{
	public:
		Drone* drone;
		int destinationDistance;

		Package(void);
		void Behavior();
		void getDrone();
		void releaseDrone();
};

class PackageGenerator : public Event
{
	void Behavior();
};

int	main();

#endif // __MAIN_H_INCLUDED__