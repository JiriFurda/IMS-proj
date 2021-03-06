//#define DEBUG_BUILD

#include "main.hpp"

#define CZ_SUMMER_DAYLIGHT 787
#define CZ_WINTER_DAYLIGHT 479

const double SETTINGS_maxBattery = 40000;
const double SETTINGS_maxDestinationDistance = 16000;
const int SETTINGS_droneCount = 144;
const double SETTINGS_systemDuration = CZ_SUMMER_DAYLIGHT;
const double SETTINGS_droneSpeed = 80;  // km/h
const double SETTINGS_droneChargeTime = 120; // How many minutes it takes to charge dron from empty to full battery
const double SETTINGS_orderAcceptance = SETTINGS_systemDuration - 180; // make sure all packages are delivered

int STAT_packagesCount = 0;
int STAT_packagesDelivered = 0;
int STAT_packagesDeliveredUnderLimit = 0;

Stat STAT_chargingWithPackage("Charging with assigned package");
Stat STAT_chargingWithoutPackage("Charging without assigned package");
Stat STAT_idlingCharged("Idling fully charged");
Stat STAT_traveling("Drone in flight");
Stat STAT_deliveryTime("Delivery time");
Stat STAT_distance("Destination distance");

PackagesQueue packagesWaitingForDrone("Packages waiting for drone");
Drone drones[SETTINGS_droneCount];



Drone::Drone(void)
{
    this->batteryMax = SETTINGS_maxBattery;	// Drone can travel up to 40 km with full battery
    this->battery = this->batteryMax;	// Drone has full battery when created
    this->speed = SETTINGS_droneSpeed / 3.6 * 60; // meters per minute
    this->chargingRate = this->batteryMax / SETTINGS_droneChargeTime; // meters per minute
    this->beginOfIdle = Time;
    this->returning = false;
}

Drone* Drone::findOptimal(double batteryRequired)
{
    double minBattery = numeric_limits<double>::max();
    int minIndex = -1;
    double maxBattery = numeric_limits<double>::min();
    int maxIndex = -1;

    // Search for ideal drone
    for(int i=0; i < SETTINGS_droneCount; i++) // Loop through drones in the system
    {
        if(!drones[i].Busy() && !drones[i].returning) // If found available one
        {
            if(drones[i].battery >= batteryRequired && drones[i].battery < minBattery)
            {
                minIndex = i;
                minBattery = drones[i].battery;
            }

            if(drones[i].battery > maxBattery)
            {
                maxIndex = i;
                maxBattery = drones[i].battery;
            }
        }
    }

    if(maxIndex == -1)
        return NULL;    // Not found

    if(minIndex != -1)
        return &drones[minIndex];   // Found drone with enough battery

    return &drones[maxIndex];   // Found available drone but not with enough battery
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
    this->destinationDistance = Random()*SETTINGS_maxDestinationDistance;	// How many meters must be traveled to deliver the package
    //this->destinationDistance = 8000;
    this->createdAt = Time;

    STAT_distance(this->destinationDistance);

    DEBUG("Package created (distance=" << this->destinationDistance << ")\n");
    STAT_packagesCount++;

    if (this->destinationDistance > SETTINGS_maxDestinationDistance)
        exit(-1);
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
    DEBUG("Package delivered\n");


    // Update statistics
    STAT_packagesDelivered++;
    double deliveryDuration = Time - this->createdAt;
    if(deliveryDuration <= 30)
        STAT_packagesDeliveredUnderLimit++;
    STAT_deliveryTime(deliveryDuration);


    this->sendDroneHome();

    DEBUG("Package leaving system\n");
}

void Package::getDrone()
{
    while(!this->drone)
    {
        // Get the drone or go into queue
        if(this->drone = Drone::findOptimal(this->destinationDistance*2))
        {
            DEBUG("Package has assigned drone\n");

            Seize(*(this->drone));

            double batteryBeforeIdleCharge = this->drone->battery;
            int idleDuration = Time - this->drone->beginOfIdle;

            this->drone->charge(idleDuration*(this->drone->chargingRate)); // Add charged energy while idling
            if(batteryBeforeIdleCharge != this->drone->battery)
            {
                DEBUG("Idle charged " << this->drone->battery - batteryBeforeIdleCharge << " units\n");
            }
            STAT_chargingWithoutPackage((this->drone->battery - batteryBeforeIdleCharge) / this->drone->chargingRate);
            STAT_idlingCharged(idleDuration - (this->drone->battery - batteryBeforeIdleCharge) / this->drone->chargingRate);


            this->drone->beginOfIdle = -1; // Not idling anymore
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
        this->drone->returning = true;
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
    {
        Seize(*(this->drone));
    }
    else
        cerr << "ERROR: Drone returning home is already seized\n";


    // Travel back to headquarters
    DEBUG("Drone is returning to HQ\n");
    Wait(drone->travel(this->headquartersDistance));
    DEBUG("Drone is at HQ\n");


    // Drone arrived to HQ
    STAT_traveling(this->headquartersDistance*2 / this->drone->speed);
    Release(*(this->drone));
    this->drone->returning = false;
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
    if (Time <= SETTINGS_orderAcceptance)
    {
        (new Package)->Activate();	// Generate new Order
        Activate(Time+ Exponential(1));	// Wait untill next generating
    }
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
    STAT_deliveryTime.Output();
    STAT_distance.Output();

    cout << "+----------------------------------------------------------+\n";
    cout << "| STATISTIC                                                |\n";
    cout << "+----------------------------------------------------------+\n";
    cout << "|  Number of drones = " << sizeof(drones)/sizeof(Drone) << "\n";
    cout << "|  Packages created = " << STAT_packagesCount << "\n";
    cout << "|  Packages delivered = " << STAT_packagesDelivered << "\n";
    cout << "|  Packages delivered under 30 minutes = " << STAT_packagesDeliveredUnderLimit << " (" << (float)STAT_packagesDeliveredUnderLimit/(float)STAT_packagesCount*100 << "%)\n";
    cout << "+----------------------------------------------------------+\n";

}