#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "helperClasses.h"

HostId selectNeighbor(list<NeighborInfo> &bidirectionalNeighbors,
	list< NeighborInfo> &unidirectionalNeighbors,
	list<HostId> &allHosts,
	HostId &thisHost)
{
	// pick a host randomly from list of all hosts. If the selected host is already active or semi-active, then pick again

	int cnt = 0;
	while (1)
	{
		if (cnt++ > 10000)
			break;
		int hostToPick = (int)floor((double)rand() / (double)RAND_MAX*(double)allHosts.size()); // pick a random number from 0 to the size of allHosts
		if (hostToPick == allHosts.size())
			hostToPick--;	// just in case

		for (HostId hostId : allHosts)
		{
			if (hostToPick == 0)
			{
				for (NeighborInfo &neighbor : bidirectionalNeighbors) {
					if (neighbor.hostId == hostId)
						break;
				}
				for (NeighborInfo &neighbor : unidirectionalNeighbors) {
					if (neighbor.hostId == hostId)
						break;
				}
				if (hostId == thisHost)
					break;

				return hostId; // this one is ok

			}
			hostToPick--;
		}
	}
	printf("!!!!!Failed to select from the list of all hosts that is not either active or semi-active. \n");
	return allHosts.front(); // this should never happen
}

void fillThisHostIP(HostId &thisHost)
{
	// This determines the host IP address and saves if in thisHost.ip
	char MyIP[2000];
	struct hostent * hp;
	struct sockaddr_in to;

	gethostname(MyIP, 2000);
	hp = gethostbyname(MyIP);
	memcpy(&(to.sin_addr), hp->h_addr, hp->h_length);
#ifdef _WINDOWS
	strcpy_s(thisHost._address, ADDRESS_LENGTH, inet_ntoa(to.sin_addr));
#else
	strcpy(thisHost._address, inet_ntoa(to.sin_addr));
#endif

}



void readAllHostsList(char *fn, list<HostId> &allHosts, HostId &thisHost)
{
	// This reads the hosts IDs (ip and port) listed in file fn and puts the host IDs in allHosts
	// This function should be called during initialization
	FILE *fptr;
#ifdef _WINDOWS
	fopen_s(&fptr, fn, "rt");
#else
	fptr = fopen(fn, "rt");
#endif
	if (fptr == NULL)
	{
		printf("error: could not open file %s with list of all hosts\n", fn);
		exit(0);
	}

	char str[20];
	int port;
	printf("possible neighbors: \n");
	//while (fscanf_s(fptr,"%20s %d",str, &port)!=EOF )

	while (fscanf_s(fptr, "%20s ", str, sizeof(str)) != EOF && fscanf_s(fptr, "%d", &port) != EOF)
	{
		if (strcmp(str, "*") == 0)
#ifdef _WINDOWS
			strcpy_s(str, ADDRESS_LENGTH, thisHost._address);
#else
			strcpy(str, thisHost._address);
#endif
		printf("	%s %d\n", str, port);
		HostId hid(str, port);
		allHosts.push_back(hid);
	}
	printf("\n");
	fclose(fptr);
}

void removeFromList(NeighborInfo &neighborToRemove, list<NeighborInfo> &listToCheck) {
	listToCheck.remove(neighborToRemove);
}

void addOrUpdateList(NeighborInfo &neighborToUpdate, list<NeighborInfo> &listToCheck) {
	for (NeighborInfo &ni : listToCheck) {
		if (ni == neighborToUpdate) {
			ni.timeWhenLastHelloArrived = neighborToUpdate.timeWhenLastHelloArrived;
			return;
		}
	}
	listToCheck.push_back(neighborToUpdate);

}


void showStatus(bool &searchingForNeighborFlag,
	list<NeighborInfo> &bidirectionalNeighbors,
	list<NeighborInfo> &unidirectionalNeighbors,
	NeighborInfo &tempNeighbor,
	HostId &thisHost,
	int targetNumberOfNeighbors)
{
	cout << "----------------------------------------------------------------------\n";
	cout << "This host: "; thisHost.show(); cout << "\n";
	cout << "has " << bidirectionalNeighbors.size() << " bidirectional neighbors ";
	cout << "and has " << unidirectionalNeighbors.size() << " unidirectional neighbors. ";
	cout << "The target number of neighbors is: " << targetNumberOfNeighbors << "\n";
	if (searchingForNeighborFlag)
		cout << "current is searching for a new neighbor\n";
	else
		cout << "current is not searching for a new neighbor\n";
	if (bidirectionalNeighbors.size()>0) {
		cout << "Bidirectional neighbors: \n";
		for (NeighborInfo &neighbor : bidirectionalNeighbors) {
			neighbor.show();
			cout << "\n";
		}
	}
	if (unidirectionalNeighbors.size() > 0) {
		cout << "Unidirectional neighbors: \n";
		for (NeighborInfo &neighbor : unidirectionalNeighbors) {
			neighbor.show();
			cout << "\n";
		}
	}
	if (searchingForNeighborFlag) {
		cout << "trying to connect to: ";
		tempNeighbor.show();
		cout << "\n";
	}
	cout << "----------------------------------------------------------------------\n";
}