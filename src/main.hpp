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

		Drone();
		double travel(double distance);
};


class Package : public Process
{
	public:
		Drone* drone;
		double destinationDistance;

		Package();
		void Behavior();
		void getDrone();
        void sendDroneHome();

		void releaseDrone();
};

class DroneReturning : public Process
{
    public:
        Drone* drone;
        double headquartersDistance;

        DroneReturning(Drone* drone, double distance);
        void Behavior();
};

class PackageGenerator : public Event
{
	void Behavior();
};

int	main();

#endif // __MAIN_H_INCLUDED__