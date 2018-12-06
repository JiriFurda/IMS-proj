#include "simlib.h"
#include <stdio.h>

Queue ordersWaitingForDrones("Orders waiting for drones");

class Order : public Process
{
	public:
		void Behavior()
		{
			ordersWaitingForDrones.Insert(this);	// Move to the queue for available drone
			Passivate();	// Sleep untill some activates this
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
	Init(0,24*60); // 24 hours

	(new OrderGenerator)->Activate();

	Run();

	ordersWaitingForDrones.Output();
}