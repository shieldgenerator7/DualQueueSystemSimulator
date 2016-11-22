#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

//
// Global Variables
//
double lambda;//arrival rate (packets/hr), changes with each Simulation
double mu;//service rate (packets/hr), changes with each Simulation

//
// Packet
//
typedef struct{
	double timeEntered;
	double timeStarted;
	double timeFinished;
	int numberPacketsInQueueUponArrival;
	int accepted;//0=no, 1=yes
	int queueId;
}Packet;

//
// Queue
//
typedef struct{
	int currentServiceIndex;
	int numberPacketsInQueue;
	int queueId;
	Packet* packetList[10];
}Queue;
Queue q1 = (Queue){.queueId = 1};
Queue q2 = (Queue){.queueId = 2};

void resetQueue(Queue *q){
	//memset(q.packetList, 0, 10);
	q->numberPacketsInQueue = 0;
	q->currentServiceIndex = 0;
}

void servicePacket(Packet *p, double timeStarted){
	p->timeStarted = timeStarted;
	p->timeEntered = timeStarted + 60/mu;// 60/mu is minutes per packet
}

void serviceNextPacket(Queue *q, double timeStarted){
	servicePacket(&(q->packetList[q->currentServiceIndex]), timeStarted);
}

void addToQueue(Queue *q, Packet *p){
	int index = (q->currentServiceIndex+q->numberPacketsInQueue) % 10;
	q->packetList[index] = p;
	p->numberPacketsInQueueUponArrival = q->numberPacketsInQueue;
	p->accepted = 1;
	if (q->numberPacketsInQueue == 0){
		servicePacket(p, p->timeEntered);
	}
	q->numberPacketsInQueue++;
	//printf("ACCEPTED: p.timeEntered: %lf, %d\n",p->timeEntered,p->accepted);
}

void rejectPacket(Packet *p){
	p->accepted = 0;
	p->timeFinished = p->timeEntered;
	printf("REJECTED: p.timeEntered: %lf, %d\n",p->timeEntered,p->accepted);
}

void removeServicedPacket(Queue *q){
	q->currentServiceIndex = (q->currentServiceIndex + 1) %10;
	q->numberPacketsInQueue--;
}

//
// Iteration
//
typedef struct{
	double lambda;//the arrival rate (packets/hr)
	double mu;//the service rate (packets/hr)
	Packet packetList[100];
	char pas;//the first letter of the PAS it used: "r" = random, "s" = shortest queue
}Iteration;

void generatePacketList(int size, Packet list[]);

void simulate(Iteration *iteration){
	//Reset the queues
	resetQueue(&q1);
	resetQueue(&q2);
	//Generate packets
	generatePacketList(100, iteration->packetList);
	double time = 0.0;//how much time has elapsed
	int packetsSent = 0;//how many of the planned packets have been sent
	while (packetsSent < 100 || q1.numberPacketsInQueue > 0 || q2.numberPacketsInQueue > 0){
		//Determine which queue has the next event to process
		double nextPacket = iteration->packetList[packetsSent].timeEntered;
		double packet1 = 10000;
		if (q1.numberPacketsInQueue > 0){
			packet1 = q1.packetList[q1.currentServiceIndex]->timeFinished;
		}
		double packet2 = 10000;
		if (q2.numberPacketsInQueue > 0){
			packet2 = q2.packetList[q2.currentServiceIndex]->timeFinished;
		}
		//Process that event
		if (nextPacket < packet1 && nextPacket < packet2){//nextPacket is least
			time = nextPacket;//update time to when the packet arrives
			Packet* packetToSend = &iteration->packetList[packetsSent];
			//Add a packet to the queues
			if (iteration->pas == 'r'){//Random PAS
				int q = rand() % 2;
				if (q == 0){
					if (q1.numberPacketsInQueue < 10){
						addToQueue(&q1, packetToSend);
						//printf(">>>yes, accepted: %d\n",packetToSend.accepted);
					}
					else{
						rejectPacket(packetToSend);
						//printf(">>>no, not accepted: %d\n",packetToSend.accepted);
					}
				}
				else if (q == 1){
					if (q2.numberPacketsInQueue < 10){
						addToQueue(&q2, packetToSend);
						//printf(">>>yes, accepted: %d\n",packetToSend.accepted);
					}
					else{
						rejectPacket(packetToSend);
						//printf(">>>no, not accepted: %d\n",packetToSend.accepted);
					}
				}
				else{
					printf("/!\\ Something's wrong! random q: %d\n",q);
				}
			}
			else if (iteration->pas == 's'){//Shortest Queue PAS
				if (q1.numberPacketsInQueue <= q2.numberPacketsInQueue && q1.numberPacketsInQueue < 10){
					addToQueue(&q1, packetToSend);
					//printf(">>>yes, accepted: %d\n",packetToSend.accepted);
				}
				else if (q2.numberPacketsInQueue < 10){
					addToQueue(&q2, packetToSend);
					//printf(">>>yes, accepted: %d\n",packetToSend.accepted);
				}
				else{
					rejectPacket(packetToSend);
					//printf(">>>no, not accepted: %d\n",packetToSend.accepted);
				}
			}
			else{
				printf("/!\\ Something's wrong! iteration.pas: %c\n",iteration->pas);
			}
			packetsSent++;//regardless, increment the packetsSent counter

		}
		else{
			if (packet1 < packet2){//packet1 is least (or tied with nextPacket)
				time = packet1;
				removeServicedPacket(&q1);
				serviceNextPacket(&q1, time);
			}
			else{//packet2 is least (or tied)
				time = packet2;
				removeServicedPacket(&q2);
				serviceNextPacket(&q2, time);
			}
		}
	}
}

//
// Simulation
//
typedef struct {
	Iteration iterationList[100];
	char pas;//the first letter of the PAS it used: "r" = random, "s" = shortest queue
}Simulation;

void startSimulation(Simulation s, double lambda, double mu){
	//for each iteration
	for (int i = 0; i<10; i++){
		Iteration iteration = (Iteration){.lambda=lambda, .mu=mu, .pas = s.pas};
		printf("\n==Iteration #%d==\n",(i+1));
		//run the simulation
		simulate(&iteration);
		int acceptedPackets = 0;
		for (int i = 0; i < 100; i++){
			if (iteration.packetList[i].accepted == 1){
				acceptedPackets++;
			}
		}
		printf("  # of Accepted Packets: %d\n",acceptedPackets);
	}
}

//
// Methods
//

int main (){
	Simulation simulR = (Simulation){.pas='r'};
	Simulation simulS = (Simulation){.pas='s'};
	//For each value of lambda
	lambda = 100;
	mu = 100;
	//for (lambda = 1; lambda <= 100; lambda*=10){
	//Random PAS Simulation
	startSimulation(simulR, lambda, mu);
	//Shortest Queue PAS Simulation
	startSimulation(simulS, lambda, mu);
	//}
	//For each value of mu
	lambda = 100;
	//for (mu = 1; mu <= 10000; mu*=10){
	//Create a simulation
	//Run it
	//}
	//Iteration iteration = (Iteration){.lambda=100, .mu=100};
	//generatePacketList(100, iteration.packetList);
	//for (int i = 0; i<100; i++){
	//	printf("index: %d, packet: %lf\n",i,iteration.packetList[i].timeEntered);
	//}
	//Queue q1 = (Queue){.queueId=1};
	//printf("Hello World");
}

void generatePacketList(int size, Packet list[]){
	//for each number from 0 to size
	for (int i = 0; i<size; i++){
		//construct a packet
		double timeEnteredIn = fmod((double)rand()/7, 60.0);
		Packet p = (Packet){.timeEntered=timeEnteredIn};
		//and randomly set its timeEntered variable
		list[i] = p;
		//printf("index: %d, packet: %lf\n",i,list[i].timeEntered);
	}
	//sort array
	for (int i = 0; i<size; i++){
		for (int j = 0; j<size-1; j++){
			if (list[j].timeEntered > list[j+1].timeEntered){
				Packet temp = list[j];
				list[j] = list[j+1];
				list[j+1] = temp;
			}
		}
	}
}
