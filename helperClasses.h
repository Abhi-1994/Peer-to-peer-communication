
#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef _HELPERS_
#define  _HELPERS_

//#include <boost/foreach.hpp>
//#include <boost/cstdint.hpp>
//#ifndef foreach
//#define foreach         BOOST_FOREACH
//#endif
#include <iostream>
#include "time.h"
#include <list>
#include <iostream>  // used for routing
#include <iomanip>	// used for routing
using namespace std;
#include "target.h"
#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN   
#define WIN32_LEAN_AND_MEAN   
#endif   
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <process.h>
#ifndef socket_t 
#define socklen_t int
#endif   
#pragma comment(lib, "iphlpapi.lib")  
#pragma comment(lib, "ws2_32.lib")
// #pragma comment(lib, "ws2.lib")   // for windows mobile 6.5 and earlier 
#else
#include <sys/time.h>
#include <errno.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <sys/select.h> /* this might not be needed*/
#ifndef SOCKET 
#define  SOCKET int
#include <unistd.h>
#endif   
#endif    
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>   
#include <time.h>   
#include <sys/types.h>
#include <sys/timeb.h>
#include <math.h>

#include <stdint.h>

#define HELLO_MESSAGE_TYPE (uint8_t)1

#define ADDRESS_LENGTH 16

class Packet {
public:
	static const int bufferLength = 65000;
	char ptr[bufferLength];
	int totalSize;
	int offset;

	void getPacketReadyForWriting() {
		totalSize = bufferLength;
		offset = 0;
	}

	uint8_t getPacktType() {
		return *(uint8_t *)ptr;
	}
	int remainingSize() {
		return totalSize - offset;
	}
	char * getPtr() {
		return ptr + offset;
	}
	Packet() {
		totalSize = 0;
		offset = 0;
	}
	void setTotalSize(int size) {
		totalSize = size;
	}
	int getTotalSize() {
		return totalSize;
	}
	void incOffset(int inc) {
		offset += inc;
		if (offset>totalSize) {
			printf("over ran packer buffer\n");
			throw;
		}
	}
};

class HostId {
public:
	char _address[ADDRESS_LENGTH];
	uint16_t _port;

	HostId() {};
	HostId(char *ipAddress, int port) {
#ifdef _WINDOWS
		strncpy_s(_address, ADDRESS_LENGTH, ipAddress, ADDRESS_LENGTH);
#else
		strncpy(_address, ipAddress, ADDRESS_LENGTH);
#endif
		_port = port;
	}
	HostId(HostId const & other) {
#ifdef _WINDOWS
		strncpy_s(_address, ADDRESS_LENGTH, other._address, ADDRESS_LENGTH);
#else
		strncpy(_address, other._address, ADDRESS_LENGTH);
#endif
		_port = other._port;
	}
	HostId & operator =(HostId const & other) {
		if (this != &other) {
#ifdef _WINDOWS
			strncpy_s(_address, ADDRESS_LENGTH, other._address, ADDRESS_LENGTH);
#else
			strncpy(_address, other._address, ADDRESS_LENGTH);
#endif
			_port = other._port;
		}
		return *this;
	}
	bool operator==(HostId const & other) const {
		if (strcmp(_address, other._address) == 0 && _port == other._port)
			return true;
		return false;
	}

	HostId(char address[ADDRESS_LENGTH], uint16_t port) {
#ifdef _WINDOWS
		strncpy_s(_address, ADDRESS_LENGTH, address, ADDRESS_LENGTH);
#else
		strncpy(_address, address, ADDRESS_LENGTH);
#endif
		_port = port;
	}

	void show() {
		cout << _address << ":" << _port;
	}

	void addToPacket(Packet &pkt) {
		// add HostId info to pkt at offset
#ifdef _WINDOWS
		strncpy_s(pkt.getPtr(), ADDRESS_LENGTH, _address, ADDRESS_LENGTH);
#else
		strncpy(pkt.getPtr(), _address, ADDRESS_LENGTH);
#endif
		pkt.incOffset(ADDRESS_LENGTH);


		uint16_t x = htons(_port);
		*(uint16_t *)pkt.getPtr() = x;//5htons(_port);
		pkt.incOffset(sizeof(_port));
	}
	bool getFromPacket(Packet &pkt) {
		if (pkt.remainingSize()<ADDRESS_LENGTH)
			return false;
#ifdef _WINDOWS
		strncpy_s(_address, ADDRESS_LENGTH, pkt.getPtr(), ADDRESS_LENGTH);
#else
		strncpy(_address, pkt.getPtr(), ADDRESS_LENGTH);
#endif
		pkt.incOffset(ADDRESS_LENGTH);

		if (pkt.remainingSize()<sizeof(_port))
			return false;
		_port = ntohs(*(uint16_t *)pkt.getPtr());
		pkt.incOffset(sizeof(_port));
		return true;
	}
};

class RoutingTableEntry {
public:
	HostId destination;
	HostId nextHop;
	uint16_t cost;
	RoutingTableEntry() {
		cost = 0;
	}
	RoutingTableEntry(HostId &_destination, HostId &_nextHop, int _cost) {
		destination = _destination;
		nextHop = _nextHop;
		cost = _cost;
	}

	RoutingTableEntry(const RoutingTableEntry &other) {
		cost = other.cost;
		destination = other.destination;
		nextHop = other.nextHop;
	}
	RoutingTableEntry & operator=(const RoutingTableEntry &other) {
		cost = other.cost;
		destination = other.destination;
		nextHop = other.nextHop;
		return *this;
	}

	void addToPacket(Packet &pkt) {
		destination.addToPacket(pkt);
		nextHop.addToPacket(pkt);
		uint16_t x = htons(cost);
		*(uint16_t *)pkt.getPtr() = x;
		pkt.incOffset(sizeof(cost));
	}
	bool getFromPacket(Packet &pkt) {
		if (destination.getFromPacket(pkt) == false)
			return false;
		if (nextHop.getFromPacket(pkt) == false)
			return false;
		if (pkt.remainingSize()<sizeof(cost))
			return false;
		cost = ntohs(*(uint16_t *)pkt.getPtr());
		pkt.incOffset(sizeof(uint16_t));
		return true;
	}

};

class RoutingTable {
public:
	std::list<RoutingTableEntry> table;
	RoutingTable() {
	}
	RoutingTable(const RoutingTable &other) {
		table = other.table;
	}
	void addEntry(RoutingTableEntry entry) {
		table.push_back(entry);
	}

	void addEntry(HostId &destination, HostId &nextHop, int cost) {
		RoutingTableEntry entry(destination, nextHop, cost);
		table.push_back(entry);
	}
	void clear() {
		table.clear();
	}

	void show() {
		cout << "Routing Table\n";
		cout << "Destination  " << "Next Hop  " << "Cost\n";
		for (auto &entry : table) {
			entry.destination.show();
			cout << "   ";
			entry.nextHop.show();
			cout << "   " << entry.cost << "\n";
		}
	}
	void addToPacket(Packet &pkt) {
		uint16_t x = htons(table.size());
		*(uint16_t *)pkt.getPtr() = x;
		pkt.incOffset(sizeof(uint16_t));

		for (RoutingTableEntry &entry : table) {
			entry.destination.addToPacket(pkt);
			entry.nextHop.addToPacket(pkt);
			uint16_t x = htons(entry.cost);
			*(uint16_t *)pkt.getPtr() = x;
			pkt.incOffset(sizeof(uint16_t));
		}
	}
	bool getFromPacket(Packet &pkt) {
		table.clear();
		if (pkt.remainingSize()<sizeof(uint16_t))
			return false;
		int numberOfEntries = ntohs(*(uint16_t *)pkt.getPtr());
		pkt.incOffset(sizeof(uint16_t));

		for (int i = 0; i<numberOfEntries; i++) {
			RoutingTableEntry entry;
			if (entry.destination.getFromPacket(pkt) == false)
				return false;
			if (entry.nextHop.getFromPacket(pkt) == false)
				return false;
			if (pkt.remainingSize()<sizeof(uint16_t))
				return false;
			entry.cost = ntohs(*(uint16_t *)pkt.getPtr());
			pkt.incOffset(sizeof(uint16_t));
			table.push_back(entry);
		}
		return true;
	}
};


class NeighborInfo {
public:
	HostId hostId;
	time_t timeWhenLastHelloArrived;
#ifdef _ROUTING_
	RoutingTable routingTable; // used for routing
#endif

	NeighborInfo() {};
	NeighborInfo(HostId const  & host) {
		hostId = host;
	}
	NeighborInfo(NeighborInfo const &other) {
		hostId = other.hostId;
		timeWhenLastHelloArrived = other.timeWhenLastHelloArrived;
#ifdef _ROUTING_
		routingTable = other.routingTable; // used for routing
#endif
	}
	void updateTimeToCurrentTime() {
		time(&timeWhenLastHelloArrived);
	}

	NeighborInfo & operator=(NeighborInfo const &other) {
		hostId = other.hostId;
		timeWhenLastHelloArrived = other.timeWhenLastHelloArrived;
#ifdef _ROUTING_
		routingTable = other.routingTable;
#endif
		return *this;
	}
	bool operator==(NeighborInfo const & other) const {
		return hostId == other.hostId;
	}


	void show() {
		hostId.show();
		time_t currentTime;
		time(&currentTime);
		cout << " last hello arrived " << difftime(currentTime, timeWhenLastHelloArrived) << " seconds ago";
#ifdef _ROUTING_
		routingTable.show();
#endif
	}
	void addToPacket(Packet &pkt) {
		hostId.addToPacket(pkt);
	}

	// gets message from packet. 
	// Returns false is failed
	void getFromPacket(Packet &pkt) {
		hostId.getFromPacket(pkt);
		time(&timeWhenLastHelloArrived);
	}
};


class HelloMessage {
public:
	uint8_t type;
	HostId source;
	uint16_t numberOfNeighbors;
	list<HostId> neighbors;
#ifdef _ROUTING_
	RoutingTable routingTable;  // used for routing
#endif

	HelloMessage() {
		numberOfNeighbors = 0;
	}

	void show() {
		cout << "Hello message from: ";
		source.show();
		cout << " advertising neighbors: ";
		for (HostId &host : neighbors) {
			host.show();
			cout << "  ";
		}
#ifdef _ROUTING_
		cout << "Routing Table\n"; // used for routing
		routingTable.show(); // used for routing
#endif

		cout << "\n";

	}

	void addToNeighborsList(HostId &hostId) {
		neighbors.push_back(hostId);
		numberOfNeighbors++;
	}

	// gets message from packet. 
	// Returns false is failed
	bool getFromPacket(Packet &pkt) {
		if (pkt.getPacktType() != HELLO_MESSAGE_TYPE)
			return false;

		if (pkt.remainingSize()<sizeof(type))
			return false; // failed to get pkt

		type = *(uint8_t *)pkt.getPtr();
		pkt.incOffset(sizeof(type));

		if (source.getFromPacket(pkt) == false)
			return false;

		if (pkt.remainingSize()<sizeof(numberOfNeighbors))
			return false;
		numberOfNeighbors = ntohs(*(uint16_t *)pkt.getPtr());
		pkt.incOffset(sizeof(numberOfNeighbors));
		neighbors.clear();

		for (int i = 0; i<numberOfNeighbors; i++) {
			HostId hostId;
			if (hostId.getFromPacket(pkt) == false)
				return false;
			neighbors.push_back(hostId);
		}
#ifdef _ROUTING_
		if (routingTable.getFromPacket(pkt) == false)  // used for routing
			return false;								// used for routing
#endif

		return true;
	}
	void addToPacket(Packet &pkt) {

		*(uint8_t *)pkt.getPtr() = HELLO_MESSAGE_TYPE;
		pkt.incOffset(sizeof(type));

		source.addToPacket(pkt);
		*(uint16_t *)pkt.getPtr() = htons(numberOfNeighbors);
		pkt.incOffset(sizeof(numberOfNeighbors));
		for (HostId hostId : neighbors) {
			hostId.addToPacket(pkt);
		}
#ifdef _ROUTING_
		routingTable.addToPacket(pkt); // used for routing
#endif
	}
};


class UdpSocket {
public:
	SOCKET _socket;

	UdpSocket() {
		_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);  // IPPROTO_UDP - so this will be a UDP Socket

	}
	int bindToPort(int port) {
		// set up socket 
		struct sockaddr_in My;
		memset(&My, 0, sizeof(My));                  // clear memory
		My.sin_family = AF_INET;		           // address family. must be this for ipv4, or AF_INET6 for ipv6
		My.sin_addr.s_addr = htonl(INADDR_ANY);    // allows socket to work send and receive on any of the machines interfaces (which machine is used to send?)
		My.sin_port = htons(port);		       // the port on this host

											   // BIND THE SOCKET TO Port  
		int ret = bind(_socket, (struct sockaddr *)&My, sizeof(struct sockaddr));  // ask OS to let us use the socket (why might it say we cannot?)
		if (ret != 0)
			printf("bind failed\n");
		return ret;
	}

	int checkForNewPacket(Packet &pkt, int TimeOut)
	{
		int len = sizeof(struct sockaddr);
		struct sockaddr_in from;
		fd_set readFd;
		timeval SelectTime;
		SelectTime.tv_sec = TimeOut;
		SelectTime.tv_usec = 0;
		FD_ZERO(&readFd);
		FD_SET(_socket, &readFd);
		int ret = select(255, &readFd, NULL, NULL, &SelectTime);
		if (FD_ISSET(_socket, &readFd) == 1)
		{
			int len = sizeof(struct sockaddr);
			ret = recvfrom(_socket, pkt.ptr, pkt.bufferLength, NULL, (struct sockaddr *)&from, &len);
			if (ret >= 0)
			{
				unsigned int fp = ntohs(from.sin_port);
				printf("Received pkt with message from %s and port %d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));
				pkt.totalSize = ret;
				pkt.offset = 0;
				return ret;
			}
			else
			{
				// // for windows only
				if (WSAECONNRESET == WSAGetLastError())
					printf("possibly sent a packet to an unopened port\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
				else
					printf("revfrom error %d %d\n", ret, WSAGetLastError());

			}
		}
		return -1;

	}
	int sendTo(Packet &pkt, HostId destinationHost) {
		struct sockaddr_in to;
		memset(&to, 0, sizeof(to));
		to.sin_addr.s_addr = inet_addr(destinationHost._address);     // we specifed the address as a string, inet_addr translates to a number in the correct byte order
		to.sin_family = AF_INET;                         // ipv4
		to.sin_port = htons(destinationHost._port);                // set port address (is this the sender's port or the receiver's port
		int ret = sendto(_socket, (char *)pkt.ptr, pkt.offset, 0, (struct sockaddr *)&to, sizeof(struct sockaddr));
		//printf("sendto sent %d bytes\n",ret);
		return ret;
	}


};



#endif


#pragma once
#pragma once
