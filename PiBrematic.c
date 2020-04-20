// PiBrematic.cpp
// v1.0
// Author: derrow@yahoo.com
// (c) 1997-2018 Derrow Decision Developments
//
// Program to clone a Raspberry Pi as a Brennenstuhl Brematic Gateway
// Raspberry Pi will act as such a Brematic Gateway to switch radio controlled power sockets.

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wiringPi.h>
#include <ifaddrs.h>

#define SIZE 1024
#define PORT 49880

int iTransmitPin = 0; 
int iState = 0;
char cIpAddress[25];

void customDelay(unsigned long time) 
{
    unsigned long end_time = micros() + time;
    while(micros() < end_time);
}

void sendSignal(int delayTime)
{
	iState = !iState;
	digitalWrite(iTransmitPin, iState);
	customDelay(delayTime);
}

void SendTXPCode(char *cCode)
{
	printf("Received TXP: packet.\n");
	
	char cSignals[1024];
	char delimiter[] = ",;:";
	char *ptr,*ptrLoop;
	int i;
	
	//TXP: Syntax Format:
	//TXP:0,0,Retries,Pause,Signal-Length,Signal-Count, Signals , 0

	strcpy(cSignals, cCode);

	ptr = strtok(cSignals, delimiter);	//TXP:
	ptr = strtok(NULL, delimiter);	//0
	ptr = strtok(NULL, delimiter);	//0
	ptr = strtok(NULL, delimiter);	//Retries
	int iRetries = atoi(ptr);
	ptr = strtok(NULL, delimiter);	//Pause
	int iPause = atoi(ptr);
	ptr = strtok(NULL, delimiter);	//Signal-Length
	int iSLength = atoi(ptr);
	ptr = strtok(NULL, delimiter);	//Signal-Count
	int iSCount = atoi(ptr);
	printf("Generating Signal with %d retries, %d Signal Length, %d Pause\n", iRetries, iSLength, iPause);

	printf("Sending Signal mit Länge: ");
	for(i = 0; i < iRetries * 3; i++)
	{
		ptr = strtok(NULL, delimiter);	//Signals
		while(ptr != NULL) 
		{
			int iSignal = atoi(ptr) * iSLength;
			printf("%d, ", iSignal);
			sendSignal(iSignal);
	
		 	ptr = strtok(NULL, delimiter);
		}
		printf("%d, ", iPause);
//		digitalWrite(iTransmitPin, 0);

		if(!iState)
			iState = !iState;
		sendSignal(iPause);
		
		printf("\n");

		strcpy(cSignals, cCode);
		ptr = strtok(cSignals, delimiter);	//TXP:
		ptr = strtok(NULL, delimiter);	//0
		ptr = strtok(NULL, delimiter);	//0
		ptr = strtok(NULL, delimiter);	//Retries
		ptr = strtok(NULL, delimiter);	//Pause
		ptr = strtok(NULL, delimiter);	//Signal-Length
		ptr = strtok(NULL, delimiter);	//Signal-Count
	}
	digitalWrite(iTransmitPin, 0);
}

int RunUdpServer(void)
{
        struct sockaddr_in si_me, si_other;
        int s, i, slen=sizeof(si_other);
        char buf[SIZE];

        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
        {
                printf("Failed to create socket\n\n" );
                return -1;
        }

        memset((char *) &si_me, 0, sizeof(si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(PORT);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);
        if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1)
        {
                printf("bind failed.\n\n" );
                return -1;
        }

        while(recvfrom(s, buf, SIZE, 0, &si_other, &slen) > -1)
        {
                printf("Received packet from %s:%d\nData: %s\n\n", 
                	inet_ntoa(si_other.sin_addr), 
                	ntohs(si_other.sin_port), 
                	buf);
    						
    						if( strlen(buf) > 5 && strncmp(buf,"TXP:", 4) == 0)
    							SendTXPCode(buf);
    						else if( strlen(buf) > 5 && strncmp(buf,"SEARCH HCGW", 11) == 0)
    						{
                	sprintf(buf, "HCGW:VC:Brennenstuhl;MC:0290217;FW:V016;IP:%s;;", cIpAddress);
                	sendto ( s, buf, strlen(buf), 0, (struct sockaddr*) &si_other, slen);
                }
                
                memset(buf, 0, sizeof(buf));
      }

        close(s);
        return 0;
}

int main(int argc, char **argv)
{
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) 
        { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
            
            if(strncmp(addressBuffer, "192", 3) == 0)
            	strcpy(cIpAddress, addressBuffer);
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        } 
    }
    if (ifAddrStruct!=NULL)
    	freeifaddrs(ifAddrStruct);
		printf("My IP Address %s\n", cIpAddress); 	
		
				
	if (wiringPiSetup() == -1)
		return 1;

	pinMode(iTransmitPin, OUTPUT);
	digitalWrite(iTransmitPin, 0);

	return RunUdpServer();
}
