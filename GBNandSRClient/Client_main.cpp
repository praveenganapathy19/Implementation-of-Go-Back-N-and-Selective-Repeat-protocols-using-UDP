
/*Header Files*/
#include <cstdlib>
#include <stdio.h>
#include<iostream>
#include <bitset>
#include <sstream>
#include <unistd.h>
#include<string.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<fstream>
#include<time.h>
#include<algorithm>
#include<vector>

struct dataSegment
{
    char sendChar;
    std::string checkSum;    
};

/*Global Variables*/
  std::string inputFile;
  static int portNumber,lostSeqnum;;
  int totalPackets;
  bool FlaggingVar = false;
  int WindowCheck = 0;
  
/*This class contains variables, accessible to all the client functions*/

using namespace std;
class tcp_client{
    private:
    
    std::string address;
    int port;
    
     
public:
    struct sockaddr_in server;
    int sock;
    tcp_client();
    void gobackn(int segmentSize, int windowSize);
    int Send_Data_Segment(int sockClient,int clientWinSize,int seqnum,string sendDatastring,int pktlostRand, int chkerrRand);
    void SelRep(int segmentSize, int windowSize);
    int Send_SR_Segment(int sockClient,int clientWinSize,int seqnum,string sendDatastring);
    void Sending_Error();
    void Receiving_Error();
    bool conn(int);
    bool send_data(string command,string filePath,string data);
    string receive(int);
    
};
tcp_client::tcp_client()
{
    sock = -1;
    port = 0;
    address = "";
}

/**
 * 
 * @param address--client IP address, passed from main();
 * @param port--Port info from the main()
 * @return boolean value
 */
bool tcp_client::conn(int port)
{
    string address = "localhost";
    if(sock == -1)
    {
        //Create socket
        sock = socket(AF_INET , SOCK_DGRAM , 0);
        if (sock == -1)
        {
            perror("Could not create socket");
            exit(0);
        }
        cout<<"Socket created\n";
    }
    else    
    {   
        /* OK , nothing */  
    }
     //setting up address structure
    if(inet_addr(address.c_str()) == -1)
    {
        struct hostent *hostEntity;
        struct in_addr **addr_list;
        //resolves the host name;handles the errors
        if ( (hostEntity = gethostbyname( address.c_str() ) ) == NULL)
        {
            //gethostbyname failed
            herror("gethostbyname");
            cout<<"Failed to resolve hostname\n";  
            exit(0);
            return false;
        }
         
        //Cast the h_addr_list to in_addr , since h_addr_list also has the IP address in long format
        addr_list = (struct in_addr **) hostEntity->h_addr_list;
        for(int i = 0; addr_list[i] != NULL; i++)
        {
            server.sin_addr = *addr_list[i];
            cout<<address<<" resolved to "<<inet_ntoa(*addr_list[i])<<endl;
            break;
        }
    }
    //In case of a correct IP address format
    else
    {
        server.sin_addr.s_addr = inet_addr( address.c_str() );
    } 
    server.sin_family = AF_INET;
    server.sin_port = htons( port );
    cout<<"Connected\n";
    return true;
}


/**
 * main() function of client
 * @param argc--takes integer value; gives the number of arguments
 * @param argv--double character pointer--has five parameters
 * executable file
     * Server IP address--client IP address
 * Port Number--should correlate with the server port to make a successful connection
 * Command--GET or PUT
 * File--file to send to server
 * @return integer value
 */
int main(int argc, char** argv) 
{
   //Check the number of parameters
    if (argc < 4) 
    {
        fprintf(stderr,"ERROR, One or more of 3 parameters missing!\n");
        exit(1);
    }
    //assign the parameters to local variables
  inputFile = argv[1];
  portNumber = atoi(argv[2]);
  totalPackets = atoi(argv[3]);
  tcp_client c;
  int i=0;
  ifstream inFile;
  string inputParam[5],protName,totalBits,timeOut,segSize;
  string winSize;
  
  /*Getting the parameters from the Input File and assigning to variables*/
  inFile.open(inputFile);
  while(inFile>>inputParam[i])
  {
      if(i==0) 
      {
          protName=inputParam[0];
      }
      else if(i==1)
      {
          totalBits=inputParam[1];
      }
      else if(i==2)
      {
          winSize=inputParam[2];
      }
      else if(i==3)
      {
          timeOut=inputParam[3];
      }
      else if(i==4)
      {
          segSize=inputParam[4];
      }
      i++;
  }
  
  cout <<"========================="<<endl<<"            "<<protName<<endl<<"========================="<<endl;
  cout << "Input Parameter File: " << inputFile << endl;
  cout << "Port Number: " << portNumber << endl;
  cout << "Total Packets: " << totalPackets << endl;
  cout <<"Total Number of Bits(m) = "<<totalBits<<endl;
  cout<<"Window Size = "<<winSize<<endl;
  cout <<"Time Out Value (in milliseconds) = "<<timeOut<<endl;
  
  //connecting to host
  c.conn(portNumber);
  cout<<endl;
  
  /*Sending all the parameter information to the Server*/
  
  cout<<"Sending Total number of packets,Protocol Name,Segment and Window size to Server"<<endl;
  if((sendto(c.sock,&totalPackets,sizeof(totalPackets),0,(struct sockaddr *) &c.server , sizeof(c.server)))<0)
        c.Sending_Error();
  
  if((sendto(c.sock,protName.c_str(),strlen(protName.c_str()),0,(struct sockaddr *) &c.server , sizeof(c.server)))<0)
        c.Sending_Error();
  
  if((sendto(c.sock,segSize.c_str(),strlen(segSize.c_str()),0,(struct sockaddr *) &c.server , sizeof(c.server)))<0)
        c.Sending_Error();
  
  if((sendto(c.sock,winSize.c_str(),strlen(winSize.c_str()),0,(struct sockaddr *) &c.server , sizeof(c.server)))<0)
        c.Sending_Error();
  
  cout<<endl;
  cout<<"Sending "<<totalPackets<<" packets to Server through "<<protName<<" protocol"<<endl<<endl;
  if(protName == "GBN")
  {
      c.gobackn(stoi(segSize,nullptr,10),stoi(winSize,nullptr,10));
  }
  else if(protName == "SR")
    {
      c.SelRep(stoi(segSize,nullptr,10),stoi(winSize,nullptr,10));
    }
  else
  cout<<"Enter one of the following protocols: GBN or SR"<<endl;
return 0;
}

/*Implementation function for Go-Back N protocol*/
void tcp_client::gobackn(int segmentSize, int windowSize)
{
    /*Fetching the bytes from a text file 
     * through ifstream function and resizing 
     * it to match the total packets in the command argument*/
    std::ifstream ifs("Textfile.txt");
    string sendData;
    sendData.assign((std::istreambuf_iterator<char>(ifs) ),(std::istreambuf_iterator<char>()));
    sendData.resize(totalPackets,'c');
    sendData.resize((sendData.size()-1),'\0');
    
    /*Initializing the variables*/
    int packetSize=segmentSize/2;
    int i,flagIncrement;
    int winLast=windowSize;
    int seqnumClient=0,sf=0,sn=0,lastAcked=0,randPkt=-1,randChk=-1,prevlastAck=-1,ackCount=0;
    int lostsyncFlag=0;
    char recvCharbuffer[7];
       
    socklen_t serverLength;
    serverLength=sizeof(server);
    
    /* Two while loops implemented for sending packets
     before the ACK comes from the server and after ACK reception*/
        while(sn<windowSize)
        {
            sn=Send_Data_Segment(sock,windowSize,sn,sendData,randPkt,randChk);
        }
        
        while(lastAcked<(windowSize+1))
        {
            memset(recvCharbuffer, 0, sizeof(recvCharbuffer));
            
            //Receive ACK information from server
            int e=recvfrom(sock,recvCharbuffer, sizeof(recvCharbuffer), 0, (struct sockaddr *) &server , &serverLength);
            if( e<0)
                Receiving_Error();
            
            lastAcked=atoi((((string)recvCharbuffer).substr(4,(strlen(recvCharbuffer))-4)).c_str());
            cout<<"Last Acked Value is "<<lastAcked<<endl;
            lostsyncFlag=0;
            
            /*Introducing Random variables*/
            srand(time(0));
            randPkt=rand() % 5 + 17;//(windowSize+3) + (windowSize+2);
            srand(time(0));
            randChk=rand() % 5 + 27;//((3*windowSize)) + (3*windowSize);
            
            /*Handling function for Lost Packet and Checksum Error*/
            if(prevlastAck==lastAcked)
            {
                ackCount++;
                if(ackCount>=(windowSize-1))
                {
                    randPkt=-1,randChk=-1;
                    lostsyncFlag=1;
                    ackCount=0;
                    sn=lostSeqnum;
                    cout<<endl<<"Timeout Occured"<<endl;
                    cout<<"Hence resending the Packet "<<sn<<endl;
                    sn=Send_Data_Segment(sock,windowSize,sn,sendData,randPkt,randChk);
                    cout<<"-------End of the Packet-------"<<endl;
                } 
            }
                
            prevlastAck=lastAcked;
            
            /*Updating the values of Sf, Sn and Window last element*/
            flagIncrement=0;
            flagIncrement=(lastAcked+1)-(sf%(windowSize+1));
            if(flagIncrement>0 && lostsyncFlag==0)
            {
                sf=sf+flagIncrement;
                winLast=winLast+flagIncrement;
            }
            while((sn<totalPackets) && ((sn >= sf)&&(sn < winLast)) && lostsyncFlag==0)
            {
                cout<<"Next Packet to be sent is "<<sn<<endl;
                sn=Send_Data_Segment(sock,windowSize,sn,sendData,randPkt,randChk);
                cout<<"-------End of the Packet-------"<<endl;
            }
        }
     }

/*Implementation function for sending the data segment through GBN protocol*/
int tcp_client::Send_Data_Segment(int sockClient,int clientWinSize,int seqnum,string sendDatastring,int pktlostRand, int chkerrRand)
{
    /*Declaring local variables*/
	int seqnumClient=0;
        const int compBits=8;
	char sendChar;
	string checkSum,totalDataPkt;
        string chkLost="^";
	
        /*If string has a null character, exit*/
	if(sendDatastring[seqnum]== '\0')
	{
                cout<<"Packets sent"<<endl<<"********===========********"<<endl;
		cout<<"End of Packets"<<endl;
                cout<<"********===========********"<<endl;
		exit(0);
	}
        
        seqnumClient=seqnum%(clientWinSize+1);
	cout<<"Sending Packet "<<std::to_string(seqnumClient)<<" in the window"<<endl;
       
        /*Handling functions for Packet lost and Checksum error*/
       if(seqnum==pktlostRand || seqnum==chkerrRand)
       {
            if(seqnum==pktlostRand)	
            {
                cout<<"Packet Lost for Packet: "<<seqnum<<endl;
                lostSeqnum=seqnum;
                seqnum++;
            }
        
            else if(seqnum==chkerrRand)
            {
                lostSeqnum=seqnum;
                std::bitset<compBits> binary=int(chkLost[0]);
                sendChar=sendDatastring[seqnum];
                checkSum=((~binary).to_string<char,std::string::traits_type,std::string::allocator_type>());
                totalDataPkt=sendDatastring[seqnum] + checkSum + (std::to_string(seqnumClient));
                cout<<"\n\n--------Checksum Error has occured----------"<<endl;
                cout<<"Data sent: "<<sendChar<<endl;
                int c=(sendto(sockClient,totalDataPkt.c_str(),strlen(totalDataPkt.c_str()),0,(struct sockaddr *) &server , sizeof(server)));
                if ( c < 0)
                    Sending_Error();
                seqnum++;
            }
        }
        /*Normal Execution without any errors*/
      else
        {
        std::bitset<compBits> binary=int(sendDatastring[seqnum]);
	sendChar=sendDatastring[seqnum];
	checkSum=((~binary).to_string<char,std::string::traits_type,std::string::allocator_type>());
	totalDataPkt=sendDatastring[seqnum] + checkSum + (std::to_string(seqnumClient));
        cout<<"Data sent: "<<sendChar<<endl;
        int c=(sendto(sockClient,totalDataPkt.c_str(),strlen(totalDataPkt.c_str()),0,(struct sockaddr *) &server , sizeof(server)));
        if ( c < 0)
            Sending_Error();
        seqnum++;
        }
	
        //initializing the string back to empty
	totalDataPkt="";    
	return seqnum;
}

//Contains a vector with a pair of values
struct myClass 
{
      int   intValue;
      bool  boolValue;
      myClass( const int& iVal, const bool& bVal ):
               intValue( iVal ),
               boolValue( bVal) {}
};
   
void tcp_client::SelRep(int segmentSize, int windowSize)
{
    std::vector<myClass> SenderMap;
    string sendData="11234567890987asdsadsadsadsadsadsafgdsfdfds65432112345";
    int flagIncrement=0, ValtoDel=0,ValtoAdd=0;
    int winLast=windowSize;
    int sf=0,sn=0,lastAcked;
    char recvCharbuffer[7];
    socklen_t serverLength;
    serverLength=sizeof(server);
    //Initializes values into the vector
    for (int i=sf; i<winLast;i++)
    {
       SenderMap.push_back(myClass(i,false));
    }
    cout << "Input Parameter File: " << inputFile << endl;
    cout << "Port Number: " << portNumber << endl;
    cout << "Total Packets: " << totalPackets << endl;
    while(sn<windowSize)
    {
	sn=Send_SR_Segment(sock,windowSize,sn,sendData);
    }
    while(lastAcked<(windowSize*2) )
    {
        memset(recvCharbuffer, 0, sizeof(recvCharbuffer));
        int e=recvfrom(sock,recvCharbuffer, sizeof(recvCharbuffer), 0, (struct sockaddr *) &server , &serverLength);
        if( e<0)
                Receiving_Error();
            
        lastAcked=atoi((((string)recvCharbuffer).substr(4,(strlen(recvCharbuffer))-4)).c_str());
        cout<<"\nLast Acked Value is "<<lastAcked<<endl;
        if((flagIncrement%(windowSize*2))==(lastAcked))
        {
            if(flagIncrement>=windowSize*2)
            {
                flagIncrement=0;
            }
            
            for (std::vector<myClass>::iterator it = SenderMap.begin(); it!=SenderMap.end(); it++)
            {
               
                if((*it).intValue == flagIncrement)
                {
                    (*it).boolValue = true;
                    break;
                }
            }
            
            int count=0;
            for (std::vector<myClass>::iterator it = SenderMap.begin(); it!=SenderMap.end(); it++)
            {
               
                if( (*it).boolValue == true )
                    count++;
                else
                    break;
            }
            if(count>0)
            {
                sf = sf+count;
                winLast = winLast+count; 
                cout<<"\nUpdated Sf -->"<<sf<<endl;
                cout<<"\nUpdated WinLast--->"<<winLast<<endl;
                for(int i=1; i<=count; i++)
                {
                    ValtoDel = ((sf - i) % (windowSize*2));
                    for (std::vector<myClass>::iterator it = SenderMap.begin(); it!=SenderMap.end(); it++)
                    {
                        if((*it).intValue == ValtoDel)
                        {
                            SenderMap.erase(it);
                            break;
                        }
                    }
                    
                }
                for(int i=count; i>=1; i--)
                {
                    ValtoAdd = ((winLast - i) % (windowSize*2));
                    SenderMap.push_back({ValtoAdd, false});
                }
                 while((sn<totalPackets) && ((sn >= sf) && (sn < winLast)))
                {
                    cout<<"\nNext Packet to be sent is "<<sn<<endl; 
                    sn=Send_SR_Segment(sock,windowSize,sn,sendData);
                }
                if(FlaggingVar == true)
                {
                     WindowCheck = winLast - 1;
                     for (std::vector<myClass>::iterator it = SenderMap.begin(); it!=SenderMap.end(); it++)
                     {
                         if((*it).intValue == WindowCheck%(windowSize*2))
                         {
                             if((*it).boolValue == true)
                             {
                                const int compBits=8;
                                int seqnumClient = 0;
                                string checkSum, totalDataPkt;
                                seqnumClient=flagIncrement%(windowSize*2);
                                std::bitset<compBits> binary=int(sendData[flagIncrement]);
                                checkSum=((~binary).to_string<char,std::string::traits_type,std::string::allocator_type>());
                                totalDataPkt=sendData[flagIncrement] + checkSum + (std::to_string(seqnumClient));
                                cout<<"\nA TimeOut Occured\n";
                                cout<<"\nThis is a RESENT Package\n";
                                int c=(sendto(sock,totalDataPkt.c_str(),strlen(totalDataPkt.c_str()),0,(struct sockaddr *) &server , sizeof(server)));
                                if ( c < 0)
                                     Sending_Error();
                                else
                                     FlaggingVar = false;
                             }
                         }
                     }

                 }
                else
                {
                    int count = 0;
                    for (std::vector<myClass>::iterator it = SenderMap.end(); it!=SenderMap.begin(); it--)  
                    {
                        if((*it).boolValue == true)
                            count++;
                        else
                            break;
                    }
                    if(count == windowSize - 1)
                    {
                        const int compBits=8;
                        int seqnumClient = 0;
                        string checkSum, totalDataPkt;
                        seqnumClient=flagIncrement%(windowSize*2);
                        std::bitset<compBits> binary=int(sendData[flagIncrement]);
                        checkSum=((~binary).to_string<char,std::string::traits_type,std::string::allocator_type>());
                        totalDataPkt=sendData[flagIncrement] + checkSum + (std::to_string(seqnumClient));

                        cout<<"\nA TimeOut Occured due to a Lost ACK\n";
                        cout<<"\nThe value of the RESENT packet is "<<flagIncrement;
                        int c=(sendto(sock,totalDataPkt.c_str(),strlen(totalDataPkt.c_str()),0,(struct sockaddr *) &server , sizeof(server)));
                        if ( c < 0)
                             Sending_Error();
                    }
                }
            }
            flagIncrement = ((flagIncrement+ count)%(windowSize*2));
        }
        else
        {
            
           for (std::vector<myClass>::iterator it = SenderMap.begin(); it!=SenderMap.end(); it++)
           {
               if((*it).intValue == lastAcked)
               {
                   (*it).boolValue = true;
                   break;
               }
           }
           if(FlaggingVar == true)
           {
                if(winLast <= totalPackets)
                    WindowCheck = winLast - 1;
                else
                    WindowCheck = totalPackets-1;
                for (std::vector<myClass>::iterator it = SenderMap.begin(); it!=SenderMap.end(); it++)
                {
                    if((*it).intValue == (WindowCheck%(windowSize*2)))
                    {
                        if((*it).boolValue == true)
                        {
                           const int compBits=8;
                           int seqnumClient = 0;
                           string checkSum, totalDataPkt;
                           seqnumClient=flagIncrement%(windowSize*2);
                           std::bitset<compBits> binary=int(sendData[flagIncrement]);
                           checkSum=((~binary).to_string<char,std::string::traits_type,std::string::allocator_type>());
                           totalDataPkt=sendData[flagIncrement] + checkSum + (std::to_string(seqnumClient));
                           
                           cout<<"\nThis is a RESENT Packet\n";
                           int c=(sendto(sock,totalDataPkt.c_str(),strlen(totalDataPkt.c_str()),0,(struct sockaddr *) &server , sizeof(server)));
                           if ( c < 0)
                                Sending_Error();
                           else
                                FlaggingVar = false;
                        }
                    }
                }
                
            }
            else
            {
                int count = 0;
                for (std::vector<myClass>::reverse_iterator rit = SenderMap.rbegin(); rit!=SenderMap.rend(); ++rit)  
                {
                    if((*rit).boolValue == true)
                        count++;
                    else
                        break;
                }
                if(count == windowSize - 1)
                {
                    const int compBits=8;
                    int seqnumClient = 0;
                    string checkSum, totalDataPkt;
                    seqnumClient=flagIncrement%(windowSize*2);
                    std::bitset<compBits> binary=int(sendData[flagIncrement]);
                    checkSum=((~binary).to_string<char,std::string::traits_type,std::string::allocator_type>());
                    totalDataPkt=sendData[flagIncrement] + checkSum + (std::to_string(seqnumClient));

                    cout<<"\nA Timeout occurred because of a Lost ACK\n";
                    cout<<"\nThe value of the RESENT packet is "<<flagIncrement;
                    int c=(sendto(sock,totalDataPkt.c_str(),strlen(totalDataPkt.c_str()),0,(struct sockaddr *) &server , sizeof(server)));
                    if ( c < 0)
                         Sending_Error();
                }
            }
        }
       
    }
        
}

//This member function is used to process a selective repeat segment
int tcp_client::Send_SR_Segment(int sockClient,int clientWinSize,int seqnum,string sendDatastring)
{
	int seqnumClient=0,pktlostRand,chkerrRand;
	const int compBits=8;
	char sendChar;
	string checkSum,totalDataPkt;
        //Used for intentionally changing the checksum
        string chkLost="^";
        srand(time(0));
        //Sets this variable to a random value and uses this for Lost Packet Error
	pktlostRand=rand() % 4 + 24;//rand() % clientWinSize + (clientWinSize*2);
        //Sets this variable to a random value and uses this for Checksum Error
        chkerrRand=rand() % 4 + 37;//rand() % ((clientWinSize*7)/2) + (clientWinSize*3);
        //Checks end of all the packets
	if(sendDatastring[seqnum]== '\0')
	{
                cout<<"****************************"<<endl;
		cout<<"End of Sending Original Packets"<<endl;
                cout<<"****************************"<<endl;
		return ++seqnum;	
        }
        //Sequence Number Space
        seqnumClient=seqnum%(clientWinSize*2);
	cout<<"Sending Packet "<<std::to_string(seqnumClient)<<" in the window"<<endl;
        
        //LOST PACKET OR CHECKSUM ERROR
        if(seqnum==pktlostRand || seqnum==chkerrRand)
        {
             //This is a case where the packet is not lost, hence needs to be resent
            if (seqnum==pktlostRand)
            {
                cout<<"\n\n--------------------------------Packet Lost for Packet: "<<seqnum<<"---------------"<<endl;
                seqnum++;
                pktlostRand=-1;
                FlaggingVar = true;
            }
            //This is a case where the checksum is intentionally modified to test the behavior of the Receiver
            else if(seqnum==chkerrRand)
            {
                std::bitset<compBits> binary=int(chkLost[0]);
                sendChar=sendDatastring[seqnum];
                checkSum=((~binary).to_string<char,std::string::traits_type,std::string::allocator_type>());
                totalDataPkt=sendDatastring[seqnum] + checkSum + (std::to_string(seqnumClient));
                cout<<"--------------------------Checksum Error has occurred for Packet "<<seqnum<<"------------------"<<endl;
                int c=(sendto(sockClient,totalDataPkt.c_str(),strlen(totalDataPkt.c_str()),0,(struct sockaddr *) &server , sizeof(server)));
                if ( c < 0)
                    Sending_Error();
                //Setting the flag to TRUE
                FlaggingVar = true;
                chkerrRand=-1;
                seqnum++;
            }
        }
        else
        {
            std::bitset<compBits> binary=int(sendDatastring[seqnum]);
            sendChar=sendDatastring[seqnum];
            //Finding 1's complement of the input packet and appending it to a string
            checkSum=((~binary).to_string<char,std::string::traits_type,std::string::allocator_type>());
            totalDataPkt=sendDatastring[seqnum] + checkSum + (std::to_string(seqnumClient));
            int c=(sendto(sockClient,totalDataPkt.c_str(),strlen(totalDataPkt.c_str()),0,(struct sockaddr *) &server , sizeof(server)));
            if ( c < 0)
                Sending_Error();
            seqnum++;
        }
	totalDataPkt="";    //initializing the string back to empty
	return seqnum;
}


void tcp_client::Sending_Error()
{
    perror("Data Sending failed. Error");
            cout<<errno<<endl;
            close(sock);
            exit(0);
}

void tcp_client::Receiving_Error()
{
    perror("Data Receiving failed. Error");
            cout<<errno<<endl;
            close(sock);
            exit(0);
}
