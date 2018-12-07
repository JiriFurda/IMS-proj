#define DEBUG_BUILD
#define DRONE_COUNT 1

#include "main.hpp"

int maxDestinationDistance = 10000;

PackagesQueue packagesWaitingForDrone("Packages waiting for drone");
Drone drones[DRONE_COUNT];




Drone::Drone(void)
{
    this->batteryMax = maxDestinationDistance*2;	// Drone can travel to the farest destination and back
    this->battery = this->batteryMax;	// Drone has full battery when created
    this->speed = 30 / 3.6 * 60; // meters per minute (from 30 km/h)
    this->chargingRate = 500; // meters per minute
    this->beginOfIdle = Time;
}

Drone* Drone::findFree()
{
    for(int i=0; i < DRONE_COUNT; i++) // Loop through drones in the system
    {
        //if(drones[i].isIdle()) // If found available one
        if(!drones[i].Busy()) // If found available one
        {
            return &drones[i];
        }
    }
}

double Drone::travel(double distance)
{
    if(this->battery >= distance)
    {
        this->battery -= distance; // Drain battery
        return (distance / this->speed);    // Return time needed to fly to destination
    }
    else
        cerr << "ERROR: Not enough battery to travel\n";
}

double Drone::chargeForFlight(double distance)
{
    if(this->battery < distance)
    {
        double time = (distance - this->battery) / this->chargingRate;

        DEBUG("Drone has to be charged (" << time << " minutes)\n");

        this->battery += distance; // Charge battery
        return time;   // Return time needed to charge
    }
    else
    {
        DEBUG("Drone has enough battery\n");
        return 0;
    }
}

void Drone::charge(double value)
{
    this->battery = min(this->batteryMax, this->battery + value);   // Add charged energy but do not exceed 100%
}
/*
bool Drone::isIdle()
{
    return (this->beginOfIdle != -1);
}
*/



Package::Package(void)
{
    this->drone = NULL; // Package has no drone when created
    this->destinationDistance = Exponential(maxDestinationDistance);	// How many meters must be traveled to deliver the package

    DEBUG("Package created\n");
}

void Package::Behavior()
{
    this->getDrone();

    Wait(this->drone->chargeForFlight(this->destinationDistance * 2));  // Charge if needed

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
        // Get the drone or go into queue
        if(!drones[0].Busy())
        {
            DEBUG("Package has assigned drone\n");

            this->drone = Drone::findFree(); // Asign free done
            Seize(*(this->drone));

            int debug = this->drone->battery;
            this->drone->charge(Time - this->drone->beginOfIdle); // Add charged energy while idling
            if(debug != this->drone->battery)
            {
                DEBUG("Idle charged " << this->drone->battery - debug << " units\n");
            }
            this->drone->beginOfIdle = -1;
        }
        else
        {
            packagesWaitingForDrone.Insert(this);	// Move to the queue for available drone
            Passivate(); // Sleep untill someone activates this
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
        cerr << "ERROR: Cannot send drone home when don't have any\n";
}


DroneReturning::DroneReturning(Drone* drone, double distance)
{
    this->drone = drone;
    this->headquartersDistance = distance;
}

void DroneReturning::Behavior()
{
    // Seize drone released by delivered package
    if(!this->drone->Busy())
        Seize(*(this->drone));
    else
        cerr << "ERROR: Drone returning home is already seized\n";


    // Travel back to headquarters
    DEBUG("Drone is returning to HQ\n");
    Wait(drone->travel(this->headquartersDistance));
    DEBUG("Drone is at HQ\n");


    // Drone arrived to HQ
    Release(*(this->drone));
    this->drone->beginOfIdle = Time;
    this->drone = NULL; // Unnecessary
    packagesWaitingForDrone.sendNextPackage();
}



void PackagesQueue::sendNextPackage()
{
    if(this->Length() > 0)  // If any package is waiting for drone
    {
        this->GetFirst()->Activate(); // Activate Package process
    }
}





void PackageGenerator::Behavior()
{
    (new Package)->Activate();	// Generate new Order
    Activate(Time+Uniform(1000,1001));	// Wait untill next generating
}


int	main()
{
    // Prepare environment
    Init(0,24*60); // 24 hours
    (new PackageGenerator)->Activate();

    // Execute simulation
    Run();

    // Print results
    packagesWaitingForDrone.Output();
}