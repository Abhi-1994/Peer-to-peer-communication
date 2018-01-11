#define _WINSOCK_DERPECATED_NO_WARNINGS
#include "stdafx.h"
//#include <boost/foreach.hpp>
//#include <boost/cstdint.hpp>
//#ifndef foreach
//#define foreach         BOOST_FOREACH
//#endif

#include <iostream>
#include "time.h"
#include <list>
using namespace std;

#include "target.h"
#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN   
#define WIN32_LEAN_AND_MEAN   
#endif   
//#include <sal.h>
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include "process.h"
#ifndef socket_t 
#define socklen_t int
#endif   
#pragma comment(lib, "iphlpapi.lib")  
#pragma comment(lib, "ws2_32.lib")
// #pragma comment(lib, "ws2.lib")   // for windows mobile 6.5 and earlier 
#endif
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>   
#include <time.h>   
#include <sys/types.h>
#include <sys/timeb.h>
#include <math.h>


#include "helperClasses.h"
#include "someFunctions.h"
#define USE_ROUTING 0
#define TARGET_NUMBER_OF_NEIGHBORS 4
Packet pkt;
time_t currentTime;

void removeOldNeighbors(list<NeighborInfo> &bidirectionalNeighbors,
	list<NeighborInfo> &unidirectionalNeighbors,
	bool &searchingForNeighborFlag,
	NeighborInfo &tempNeighbor)
{
	//check for unidirectional neighbors which didn't send hello messages over 40 seconds and remove it 
	for (NeighborInfo &neighbor : unidirectionalNeighbors)
	{
		time(&currentTime);

		time_t timeDifference;
		timeDifference = difftime(currentTime, neighbor.timeWhenLastHelloArrived);
		if (timeDifference >= 40)
		{
			unidirectionalNeighbors.remove(neighbor);
			break;
		}
	}

	//check for bidirectional neighbors which didn't send hello messages over 40 seconds and remove it

	for (NeighborInfo &neighbor : bidirectionalNeighbors) {
		time(&currentTime);
		time_t timeDifference;
		timeDifference = difftime(currentTime, neighbor.timeWhenLastHelloArrived);
		if (timeDifference >= 40)
		{
			removeFromList(neighbor, bidirectionalNeighbors);
			break;
		}
	}

}

void sendHelloToAllNeighbors(bool searchingForNeighborFlag,
	list<NeighborInfo> &bidirectionalNeighbors,
	list<NeighborInfo> &unidirectionalNeighbors,
	NeighborInfo &tempNeighbor,
	HostId &thisHost,
	UdpSocket &udpSocket)
{
	pkt.getPacketReadyForWriting();
	HelloMessage helloMsg;
	helloMsg.type = HELLO_MESSAGE_TYPE;
	helloMsg.source = thisHost;

	//Adding the heard neighbors list to the hello packets for unidirectional neighbors
	for (NeighborInfo &neighbor : unidirectionalNeighbors)
	{
		helloMsg.addToNeighborsList(neighbor.hostId);
	}

	//Adding the heard neighbors list to the hello packets for bidirectional neighbors
	for (NeighborInfo &neighbor : bidirectionalNeighbors)
	{
		helloMsg.addToNeighborsList(neighbor.hostId);
	}

	helloMsg.addToPacket(pkt);

	//sending the hello packets to unidirectional neighbors
	for (NeighborInfo &neighbor : unidirectionalNeighbors)
	{
		udpSocket.sendTo(pkt, neighbor.hostId);
	}

	//sending the hello packets to bidirectional neighbors
	for (NeighborInfo &neighbor : bidirectionalNeighbors)
	{
		udpSocket.sendTo(pkt, neighbor.hostId);
	}

	if (searchingForNeighborFlag)
	{
		udpSocket.sendTo(pkt, tempNeighbor.hostId);
	}
}


void receiveHello(Packet &pkt,
	bool &searchingForNeighborFlag,
	list<NeighborInfo> &bidirectionalNeighbors,
	list<NeighborInfo> &unidirectionalNeighbors,
	NeighborInfo &tempNeighbor,
	HostId &thisHost)
{
	HelloMessage helloMessage;
	helloMessage.getFromPacket(pkt);
	bool uniDirectional, tempNbr, biDirectional;
	uniDirectional = tempNbr = biDirectional = false;
	//Update the time for bidirectional hosts latest msg timing
	//Update if the host is already in Unidirectional, then to bidirectional
	for (NeighborInfo &neighbor : unidirectionalNeighbors) {
		if (neighbor.hostId == helloMessage.source)
		{
			neighbor.updateTimeToCurrentTime();
			uniDirectional = true;
			for (HostId &host : helloMessage.neighbors)
			{
				if (host == thisHost) {
					addOrUpdateList(neighbor, bidirectionalNeighbors);
					unidirectionalNeighbors.remove(neighbor);
					break;
				}
			}
			break;
		}
	}

	//Update the time for bidirectional hosts latest msg timing
	//if not the temporary neighbor, then change it to unidirectional
	for (NeighborInfo &neighbor : bidirectionalNeighbors) {
		if (neighbor.hostId == helloMessage.source)
		{
			neighbor.updateTimeToCurrentTime();
			biDirectional = true;
			tempNbr = false;

			for (HostId &host : helloMessage.neighbors)
			{
				if (host == thisHost) {
					tempNbr = true;
					break;
				}
			}

			if (tempNbr == false)
			{
				neighbor.updateTimeToCurrentTime();
				addOrUpdateList(neighbor, unidirectionalNeighbors);
				bidirectionalNeighbors.remove(neighbor);
			}

			break;
		}
	}

	if (uniDirectional == false && biDirectional == false)
	{
		if (helloMessage.source == tempNeighbor.hostId) {
			searchingForNeighborFlag = false;
		}

		tempNbr = false;
		for (HostId &host : helloMessage.neighbors)
		{
			if (host == thisHost) {
				NeighborInfo neighbor;
				neighbor.hostId = helloMessage.source;
				neighbor.updateTimeToCurrentTime();
				addOrUpdateList(neighbor, bidirectionalNeighbors);
				tempNbr = true;
				break;
			}
		}
		if (tempNbr == false)
		{
			NeighborInfo neighbor;
			neighbor.hostId = helloMessage.source;
			neighbor.updateTimeToCurrentTime();
			addOrUpdateList(neighbor, unidirectionalNeighbors);
		}
	}
}
//void makeRoutingTable(RoutingTable &routingTable, list<NeighborInfo> &bidirectionalNeighbors, HostId &thisHost);

void dumpNeighbors(list<NeighborInfo> &bidirectionalNeighbors, HostId &thisHost)
{
	FILE *fptr;
	char fn[200];
#pragma warning (disable : 4996)
	sprintf(fn, "d:\\neighborInfo%d.csv", thisHost._port);
	fptr = fopen(fn, "wt");
	if (fptr == 0)
		printf("failed to open file %s!!!!!!!!!!!!!!!!!!!!!\n", fn);
	for (NeighborInfo neighbor : bidirectionalNeighbors) {
		fprintf(fptr, "%d %d\n", thisHost._port, neighbor.hostId._port);
	}
	fclose(fptr);
}

int main(int argc, char* argv[])
{

#ifdef _WINDOWS
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 1), &wsaData) != 0)
	{
		printf("WSAStartup failed: %d\n", GetLastError());
		return(1);
	}
#endif
	RoutingTable routingTable; // new for routing

	bool searchingForNeighborFlag = false;
	list<NeighborInfo> bidirectionalNeighbors;
	list<NeighborInfo> unidirectionalNeighbors;
	list<HostId> allHosts;
	NeighborInfo tempNeighbor;
	HostId thisHost;

	if (argc != 3)
	{
		printf("usage: MaintainNeighbors PortNumberOfThisHost FullPathAndFileNameOfListOfAllHosts\n");
		return 0;
	}

#ifdef _WINDOWS
	srand(_getpid());
#else
	srand(getpid());
#endif

	fillThisHostIP(thisHost);
	thisHost._port = atoi(argv[1]);
	readAllHostsList(argv[2], allHosts, thisHost);

	printf("Running on ip %s and port %d\n", thisHost._address, thisHost._port);
	printf("press any  key to begin\n");
	//char c = getchar();

	time_t lastTimeHellosWereSent;
	time(&lastTimeHellosWereSent);
	time_t currentTime, lastTimeStatusWasShown;
	time(&lastTimeStatusWasShown);



	UdpSocket udpSocket;
	if (udpSocket.bindToPort(thisHost._port) != 0) {
		cout << "bind failed\n";
		exit(-1);
	}

	Packet pkt;

	while (1)
	{


		if (udpSocket.checkForNewPacket(pkt, 2)>0)
		{
			receiveHello(pkt, searchingForNeighborFlag, bidirectionalNeighbors, unidirectionalNeighbors, tempNeighbor, thisHost);
		}


		if (bidirectionalNeighbors.size() + unidirectionalNeighbors.size() < TARGET_NUMBER_OF_NEIGHBORS && searchingForNeighborFlag == false)
		{
			searchingForNeighborFlag = true;
			tempNeighbor = selectNeighbor(bidirectionalNeighbors, unidirectionalNeighbors, allHosts, thisHost);
			tempNeighbor.updateTimeToCurrentTime();
		}
		time(&currentTime);
		time_t timeDifferenceLastMsg;
		timeDifferenceLastMsg = difftime(currentTime, tempNeighbor.timeWhenLastHelloArrived);
		if ((timeDifferenceLastMsg >= 40) && searchingForNeighborFlag == true)
		{
			searchingForNeighborFlag = false;
		}
		time(&currentTime);
		time_t timeDifferenceSent;
		time_t timeDifferenceShown;
		timeDifferenceSent = difftime(currentTime, lastTimeHellosWereSent);
		timeDifferenceShown = difftime(currentTime, lastTimeStatusWasShown);
		if (timeDifferenceSent >= 10)
		{
			lastTimeHellosWereSent = currentTime;
			sendHelloToAllNeighbors(searchingForNeighborFlag, bidirectionalNeighbors, unidirectionalNeighbors, tempNeighbor, thisHost, udpSocket);
		}

		removeOldNeighbors(bidirectionalNeighbors, unidirectionalNeighbors, searchingForNeighborFlag, tempNeighbor);

		if (timeDifferenceShown >= 10)
		{
			time(&lastTimeStatusWasShown);
			showStatus(searchingForNeighborFlag,
				bidirectionalNeighbors,
				unidirectionalNeighbors,
				tempNeighbor,
				thisHost,
				TARGET_NUMBER_OF_NEIGHBORS);
		}

	}
}
