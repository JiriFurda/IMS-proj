#define DEBUG_BUILD

#include "main.hpp"

int maxDestinationDistance = 10000;

Queue packagesWaitingForDrone("Packages waiting for drone");
Drone globalDrone;


Drone::Drone(void)
{
    this->batteryMax = maxDestinationDistance*2;	// Drone can travel to the farest destination and back
    this->battery = this->batteryMax;	// Drone has full battery when created
    this->speed = 30 / 3.6 * 60; // meters per minute
}

double Drone::travel(double distance)
{
    if(this->battery >= distance)
    {
        this->battery -= distance; // Drain battery
        return (distance / this->speed);    // Return time needed to fly to destination
    }
    else
        cerr << "Not enough battery to travel\n";
}





Package::Package(void)
{
    this->drone = NULL;
    this->destinationDistance = Exponential(maxDestinationDistance);	// How many meters must be traveled to deliver the package

    DEBUG("Package created\n");
}

void Package::Behavior()
{
    this->getDrone();

    Wait(5); // Grab the package

    DEBUG("Package on the way\n");
    Wait(this->drone->travel(this->destinationDistance));   // Flying to package destination
    DEBUG("Package delivered\n");

    this->sendDroneHome();

    DEBUG("Package leaving system\n");
}

void Package::getDrone()
{
    while(!this->drone)
    {
        // Get the loading platform or go into queue
        if(!globalDrone.Busy())
        {
            Seize(globalDrone);
            this->drone = &globalDrone;
        }
        else
        {
            packagesWaitingForDrone.Insert(this);	// Move to the queue for available drone
            Passivate(); // Sleep untill some activates this
        }
    }
}

void Package::sendDroneHome()
{
    if(this->drone)
    {
        Release(*(this->drone));    // Transfer drone to DroneReturning process
        (new DroneReturning(this->drone, destinationDistance))->Activate();
    }
    else
        cerr << "ERROR: Cannot send drone home when don't have any";
}


DroneReturning::DroneReturning(Drone* drone, double distance)
{
    this->drone = drone;
    this->headquartersDistance = distance;
}

void DroneReturning::Behavior()
{
    // Seize drone released by delivered package
    if(!globalDrone.Busy())
        Seize(*(this->drone));
    else
        cerr << "ERROR: Drone returning home is already seized";

    // Travel back to headquarters
    DEBUG("Drone is returing to HQ\n");
    Wait(drone->travel(this->headquartersDistance));
    DEBUG("Drone is at HQ\n");

    // @todo
}

void Package::releaseDrone()
{
    if(this->drone)
    {
         // Release drone

        this->drone = NULL;

        // Pass drone to the first in the queue waiting for drone
        if(packagesWaitingForDrone.Length() > 0)
        {
            (packagesWaitingForDrone.GetFirst())->Activate();
        }
    }
    else
        cerr << "Error: Tryied to release drone when doesn't have any\n";
}


void PackageGenerator::Behavior()
{
    (new Package)->Activate();	// Generate new Order
    Activate(Time+Uniform(1,10));	// Wait untill next generating
}


int	main()
{
    // Prepare enviroment
    Init(0,24*60); // 24 hours
    (new PackageGenerator)->Activate();

    // Execute simulation
    Run();

    // Print results
    packagesWaitingForDrone.Output();
}