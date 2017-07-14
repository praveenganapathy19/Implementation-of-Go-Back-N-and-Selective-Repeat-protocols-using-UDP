/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */


/*Header Files*/
#include <cstdlib>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include<iostream>
#include <algorithm>
#include<sstream>
#include <bitset>
#include<fstream>
#include<csignal>
#include<vector>

using namespace std;
int sockfd;
void ServerReceiving_Error();
void ServerSending_Error();
void GoBackNServer(int gbnsocket,int gbntotPacket,string gbnsegSize,string gbnwinSize);
void SelectiveRepeat(int srsocket,string srwinSize);
int computeChecksum(string data, string chksum);


socklen_t clientLength;
struct sockaddr_in serverAddr, clientAddr;

/*Function for displaying corresponding error message*/
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

/**
 * Function responsible for handling key-press Interrupts (Ctrl+C)
 * Closes all the active sockets and exits control
 */
void signalHandler( int signum) {
   cout << "Interrupt signal (" << signum << ") received.\n";
   cout<<"Closing Server and Client socket connections\n";
   //close(newsockfd);
   close(sockfd);
   exit(signum);  
}

/**
 * @param argc-- takes integer value; gives the number of arguments
 * @param argv-- takes char pointer; server takes one argument: PORT Number
 * @return integer
 */
int main(int argc, char *argv[])
{
	/*Variable Declaration*/
    int portNumber,totalPkts;
    char recvSegsize[4],recvWinsize[5],recvProtocol[5];
    string segmentSize,windowSize,protocolName; 
    int *recvintBuf;
    signal(SIGINT, signalHandler);
	
    /*checks for the PORT number argument*/
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
	
    /*
     * Create Internet domain socket
     *  and checks for error in creating the socket
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
       error("ERROR opening socket");
   
   //Fills the entire socket structure with Zeroes
    memset((char *) &serverAddr, 0,  sizeof(serverAddr));
    portNumber = atoi(argv[1]);
	
    /*
     * sets the values of socket structure members
     * and binds them with a defined socket
     */
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(portNumber);
    if (bind(sockfd, (struct sockaddr *) &serverAddr,
             sizeof(serverAddr)) < 0) 
             error("ERROR in binding");
    cout<<"\nListening on port "<<portNumber<<" \n";
    
    clientLength = sizeof(clientAddr);
    memset(recvintBuf,0,sizeof(recvintBuf));
    if((recvfrom(sockfd,recvintBuf, sizeof(recvintBuf), 0, (struct sockaddr *) &clientAddr, &clientLength))<0)
        ServerReceiving_Error();
    
    memset(recvProtocol,0,sizeof(recvProtocol));
    if((recvfrom(sockfd,recvProtocol, sizeof(recvProtocol), 0, (struct sockaddr *) &clientAddr, &clientLength))<0)
        ServerReceiving_Error();
    
    memset(recvSegsize,0,sizeof(recvSegsize));
    if((recvfrom(sockfd,recvSegsize, sizeof(recvSegsize), 0, (struct sockaddr *) &clientAddr, &clientLength))<0)
        ServerReceiving_Error();
    
    memset(recvWinsize,0,sizeof(recvWinsize));
    if((recvfrom(sockfd,recvWinsize, sizeof(recvWinsize), 0, (struct sockaddr *) &clientAddr, &clientLength))<0)
        ServerReceiving_Error();
    
    protocolName=recvProtocol;
    segmentSize=recvSegsize;
    windowSize=recvWinsize;
    totalPkts=*recvintBuf;
        
    cout<<"Protocol Name as received from Client is:"<<protocolName<<endl;
    cout<<"Total Packets as received from Client is:"<<totalPkts<<endl;
    cout<<"Segment Size as received from Client is:"<<segmentSize<<endl;
    cout<<"Window Size as received from Client is:"<<windowSize<<endl;
    
    if(protocolName == "GBN")
    {
         cout<<"This is GBN"<<endl;
         GoBackNServer(sockfd,totalPkts,segmentSize,windowSize);
    }
    else if(protocolName == "SR")
    {
         cout<<"This is SR"<<endl;
         SelectiveRepeat(sockfd,windowSize);
    }
  else
    cout<<"Enter one of the following protocols: GBN or SR"<<endl;
    
    
    //closes all the open sockets
    cout<<"\nConnection Ended";
    close(sockfd);
    return 0; 
}


void GoBackNServer(int gbnsocket,int gbntotPacket,string gbnsegSize,string gbnwinSize)
{
    int recvdataLen,gbnWinSize,cmpChksum,seqnumServer=0,AcklostRand;
    char charBuffer[13];
    string recvcheckSum,receivedData,recvSeqnum,ackPacket;
    gbnWinSize=atoi(gbnwinSize.c_str());
    srand(time(0));
    AcklostRand = 26;
    while(1)
    {   
        memset(charBuffer, 0, sizeof(charBuffer));
        recvdataLen=recvfrom(gbnsocket,charBuffer, sizeof(charBuffer), 0, (struct sockaddr *) &clientAddr, &clientLength);
        if( recvdataLen<0)
            ServerReceiving_Error();
        recvSeqnum=((string)charBuffer).substr(9,(strlen(charBuffer))-9);
        recvcheckSum=((string)charBuffer).substr(1,8);
        receivedData=charBuffer[0];

        cout<<"Received Checksum is "<<recvcheckSum<<endl;
        cout<<"Received Data is "<<receivedData<<endl;
        cout<<"Received Sequence Number is "<<recvSeqnum<<endl;

        cmpChksum = computeChecksum(receivedData,recvcheckSum);
        if((cmpChksum==1) && (seqnumServer%(gbnWinSize+1))==(atoi(recvSeqnum.c_str())))
            {
                if(seqnumServer>gbnWinSize)
                {
                    seqnumServer=0;
                }
                //if(AcklostRand > 10)
                    ackPacket="ACK:" + std::to_string(seqnumServer);
                if(seqnumServer<=gbnWinSize)
                {
                    seqnumServer++;
                }
                cout<<"Sending ACK for the packet as "<<ackPacket<<endl;
                int a=(sendto(gbnsocket,ackPacket.c_str(),strlen(ackPacket.c_str()),0,(struct sockaddr *) &clientAddr, sizeof(clientAddr)));
                if ( a < 0)
                    ServerSending_Error();
                cout<<"----------------------------"<<endl;
            }
            else
            {
                int duplAck=seqnumServer-1;
                ackPacket="ACK:" + std::to_string(duplAck);
                cout<<"Sending duplicate ACK for the packet as "<<duplAck<<endl;
                int b=(sendto(gbnsocket,ackPacket.c_str(),strlen(ackPacket.c_str()),0,(struct sockaddr *) &clientAddr, sizeof(clientAddr)));
                if ( b < 0)
                    ServerSending_Error();
                cout<<"----------------------------"<<endl;
            }
    }
}

struct myClass 
{
      int   intValue;
      bool  boolValue;
      myClass( const int& iVal, const bool& bVal ):
               intValue( iVal ),
               boolValue( bVal) {}
};

void SelectiveRepeat(int srsocket,string winSize)
{
   std::vector<myClass> receiverVector;
   int recvDataLen, srWinSize, cmpCheckSum, seqNumServer =0, Rf=0, Rn=0 , recvdataInt, AcklostRand;
   char charBuffer[13];
   int ValtoDel, ValtoAdd;
   string recvcheckSum,receivedData,recvSeqnum,ackPacket;
   srWinSize = atoi(winSize.c_str());
   srand(time(0)); 
   
   Rn = srWinSize;
   for (int i=Rf; i<Rn;i++)
   {
       receiverVector.push_back(myClass(i,false));
   }
   
   while(1)
   {
        AcklostRand=rand() % (srWinSize/2) + ((srWinSize*3)/2);
        memset(charBuffer, 0, sizeof(charBuffer));
        recvDataLen=recvfrom(srsocket,charBuffer, sizeof(charBuffer), 0, (struct sockaddr *) &clientAddr, &clientLength);
        if( recvDataLen<0)
            ServerReceiving_Error();
        recvSeqnum=((string)charBuffer).substr(9,(strlen(charBuffer))-9);
        recvcheckSum=((string)charBuffer).substr(1,8);
        receivedData=charBuffer[0];
        cout<<"Checksum is "<<recvcheckSum<<endl;
        cout<<"Data is "<<receivedData<<endl;
        cout<<"Received Sequence Number is "<<recvSeqnum<<endl;
        cmpCheckSum = computeChecksum(receivedData,recvcheckSum);
        recvdataInt = atoi(recvSeqnum.c_str());
        cout<<"Rf -->"<<Rf<<endl;
        cout<<"Rn--->"<<Rn<<endl;
        int count=0;
        if((cmpCheckSum == 1) && ((seqNumServer%(srWinSize*2))==(recvdataInt)))
        {
            if(seqNumServer>=srWinSize*2)
            {
                 seqNumServer=0;
            }
            for (std::vector<myClass>::iterator it = receiverVector.begin(); it!=receiverVector.end(); it++)
            {
               
                if((*it).intValue == seqNumServer)
                {
                    (*it).boolValue = true;
                    break;
                }
            }
            for (std::vector<myClass>::iterator it = receiverVector.begin(); it!=receiverVector.end(); it++)
            {
                if((*it).boolValue == true)
                    count++;
                else
                    break;
            }
            //if(AcklostRand > 10)
            ackPacket = "ACK:"+ std::to_string(seqNumServer);
            Rf = Rf+count;
            Rn = Rn+count;
            //AcklostRand = 70;
            if(AcklostRand==seqNumServer)
            {
                cout<<"\n-----The Acknowledgement is lost---------\n";
            }
            else
            {
                int a = (sendto(srsocket,ackPacket.c_str(),strlen(ackPacket.c_str()),0,(struct sockaddr *) &clientAddr, sizeof(clientAddr)));
                if ( a < 0)
                    ServerSending_Error();
            }    
            for(int i=1; i<=count; i++)
            {
                ValtoDel = ((Rf - i) % (srWinSize*2));
                for (std::vector<myClass>::iterator it = receiverVector.begin(); it!=receiverVector.end(); it++)
                {
                    if((*it).intValue == ValtoDel)
                    {
                        receiverVector.erase(it);
                        break;
                    }
                }
            }
            for(int i=count; i>=1; i--)
            {
                ValtoAdd = ((Rn - i) % (srWinSize*2));
                receiverVector.push_back({ValtoAdd, false});
            }
            seqNumServer = seqNumServer+ count;
        }
        else if(cmpCheckSum == 1)
        {
           for (std::vector<myClass>::iterator it = receiverVector.begin(); it!=receiverVector.end(); it++)
           {
               if((*it).intValue == recvdataInt)
                {
                   (*it).boolValue = true;
                   break;
                }
           }
            ackPacket = "ACK:"+ std::to_string(recvdataInt);
            //AcklostRand = 70;
            if((AcklostRand)==seqNumServer)
            {
               cout<<"\n-----The Acknowledgement is lost---------\n"; 
            }
            else
            {
                int a = (sendto(srsocket,ackPacket.c_str(),strlen(ackPacket.c_str()),0,(struct sockaddr *) &clientAddr, sizeof(clientAddr)));
                if ( a < 0)
                    ServerSending_Error(); 
            }     
        }
        else
        {
            if(cmpCheckSum != 1)
            {
                cout<<"\nErroneous Checksum detected in the Receiver for "<<recvdataInt;
            }
        }
   }
}

int computeChecksum(string data,string chksum)
{
    const int chkBits=8;
    std::bitset<chkBits> binarychk=int(data[0]);
    string cmpChksum=((~binarychk).to_string<char,std::string::traits_type,std::string::allocator_type>());
    cout<<"Computed checksum at the Server is "<<cmpChksum<<endl;
    if(strcmp(chksum.c_str(),cmpChksum.c_str())==0)
        return 1;
    else
        return 0;
}


void ServerReceiving_Error()
{
        cout<<"Error receiving data";
        cout<<errno<<endl;
        exit(0);
}

void ServerSending_Error()
{
        cout<<"Error Sending data";
        cout<<errno<<endl;
        exit(0);
}