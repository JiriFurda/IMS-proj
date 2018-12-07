#define DEBUG_BUILD

#include "main.hpp"

const double SETTINGS_maxDestinationDistance = 10000;
const int SETTINGS_droneCount = 3;
const double SETTINGS_systemDuration = 24*60;
const double SETTINGS_droneSpeed = 30;  // km/h
const double SETTINGS_droneChargeTime = 30; // How many minutes it takes to charge dron from empty to full battery

int STAT_packagesCount = 0;
int STAT_packagesDelivered = 0;

Stat STAT_chargingWithPackage("Charging with assigned package");
Stat STAT_chargingWithoutPackage("Charging without assigned package");
Stat STAT_idlingCharged("Idling fully charged");
Stat STAT_traveling("Drone in flight");

PackagesQueue packagesWaitingForDrone("Packages waiting for drone");
Drone drones[SETTINGS_droneCount];




Drone::Drone(void)
{
    this->batteryMax = SETTINGS_maxDestinationDistance*2;	// Drone can travel to the most distant destination and back
    this->battery = this->batteryMax;	// Drone has full battery when created
    this->speed = SETTINGS_droneSpeed / 3.6 * 60; // meters per minute
    this->chargingRate = this->batteryMax / SETTINGS_droneChargeTime; // meters per minute
    this->beginOfIdle = Time;
}

Drone* Drone::findFree()
{
    for(int i=0; i < SETTINGS_droneCount; i++) // Loop through drones in the system
    {
        if(!drones[i].Busy()) // If found available one
        {
            return &drones[i];
        }
    }
    return NULL;
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

        this->charge(distance); // Charge battery
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


Package::Package(void)
{
    this->drone = NULL; // Package has no drone when created
    this->destinationDistance = Exponential(SETTINGS_maxDestinationDistance);	// How many meters must be traveled to deliver the package

    DEBUG("Package created (distance=" << this->destinationDistance << ")\n");
    STAT_packagesCount++;
}

void Package::Behavior()
{
    this->getDrone();

    // Charge if needed
    double chargingTime = this->drone->chargeForFlight(this->destinationDistance * 2);
    Wait(chargingTime);
    STAT_chargingWithPackage(chargingTime);

    Wait(5); // Grab the package

    DEBUG("Package on the way\n");
    Wait(this->drone->travel(this->destinationDistance));   // Flying to package destination
    STAT_packagesDelivered++;
    DEBUG("Package delivered\n");

    this->sendDroneHome();

    DEBUG("Package leaving system\n");
}

void Package::getDrone()
{
    while(!this->drone)
    {
        // Get the drone or go into queue
        if(this->drone = Drone::findFree())
        {
            DEBUG("Package has assigned drone\n");

            Seize(*(this->drone));

            double batteryBeforeIdleCharge = this->drone->battery;
            int idleDuration = Time - this->drone->beginOfIdle;

            this->drone->charge(idleDuration); // Add charged energy while idling
            if(batteryBeforeIdleCharge != this->drone->battery)
            {
                DEBUG("Idle charged " << this->drone->battery - batteryBeforeIdleCharge << " units\n");
            }
            STAT_chargingWithoutPackage((this->drone->battery - batteryBeforeIdleCharge) / this->drone->chargingRate);
            STAT_idlingCharged(idleDuration - (this->drone->battery - batteryBeforeIdleCharge) / this->drone->chargingRate);


            this->drone->beginOfIdle = -1;  // Not idling anymore
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
    STAT_traveling(this->headquartersDistance*2 / this->drone->speed);
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
    Activate(Time+Uniform(1,15));	// Wait untill next generating
}


int	main()
{
    // Prepare environment
    Init(0, SETTINGS_systemDuration);
    (new PackageGenerator)->Activate();

    // Execute simulation
    Run();

    // Print results
    packagesWaitingForDrone.Output();
    STAT_chargingWithPackage.Output();
    STAT_chargingWithoutPackage.Output();
    STAT_idlingCharged.Output();
    STAT_traveling.Output();

    cout << "+----------------------------------------------------------+\n";
    cout << "| STATISTIC                                                |\n";
    cout << "+----------------------------------------------------------+\n";
    cout << "|  Packages created = " << STAT_packagesCount << "\n";
    cout << "|  Packages delivered = " << STAT_packagesDelivered << "\n";
    cout << "+----------------------------------------------------------+\n";

}