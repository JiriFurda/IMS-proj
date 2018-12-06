#include "simlib.h"
#include <iostream>

using namespace std;

Queue ordersWaitingForDrone("Orders waiting for drone");
Facility drone;

class Order : public Process
{
	public:
		bool hasDrone;

		Order(void)
		{
		   this->hasDrone = false;
		}

		void Behavior()
		{
			this->getDrone();

			// Simulate some process
			Wait(Exponential(5));

			this->releaseDrone();
		}

		void getDrone()
		{
			while(!this->hasDrone)
			{
				// Get the loading platform or go into queue
				if(!drone.Busy())
				{
					Seize(drone);
					this->hasDrone = true;
				}
				else
				{
					ordersWaitingForDrone.Insert(this);	// Move to the queue for available drone
					Passivate(); // Sleep untill some activates this
				}
			}			
		}


		void releaseDrone()
		{
			if(this->hasDrone)
			{
				Release(drone); // Release drone

				// Pass drone to the first in the queue waiting for drone
				if(ordersWaitingForDrone.Length() > 0)
				{
					(ordersWaitingForDrone.GetFirst())->Activate();
				}
			}	
			else
				cerr << "Error: Tryied to release drone when doesn't have any\n";	
		}
};

class OrderGenerator : public Event
{
	void Behavior()
	{
		(new Order)->Activate();	// Generate new Order
		Activate(Time+Exponential(10));	// Wait untill next generating
	}
};

int	main()
{
	// Prepare enviroment
	Init(0,24*60); // 24 hours
	(new OrderGenerator)->Activate();

	// Execute simulation
	Run();

	// Print results
	ordersWaitingForDrone.Output();
}