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
        double batteryMax;	// Capacity of the battery (in meters)
        double battery;	// Current state of the battery (in meters)
		double speed;

		Drone(void);
		double travel(double distance);
};


class Package : public Process
{
	public:
		Drone* drone;
		double destinationDistance;

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