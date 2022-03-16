// DHCP Library
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: -
// Target uC:       -
// System Clock:    -

// Hardware configuration:
// -

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdio.h>
#include "dhcp.h"
#include "timer.h"


// ------------------------------------------------------------------------------
//  Globals
// ------------------------------------------------------------------------------

uint8_t dhcpState = DHCP_DISABLED;

bool needDiscover=false;
bool needRequest=false;
bool needRelease=false;

uint8_t dhcpOfferedIpAdd[4]={0,0,0,0};
uint8_t dhcpServerIpAdd[4]={0,0,0,0};
//uint8_t dummy[IP_ADD_LENGTH] = {0,0,0,0};

uint32_t T1=0;
uint32_t T2=0;
uint32_t lease;
bool requestBit;

//uint8_t* getv; //used to return getOption data



// ------------------------------------------------------------------------------
//  Structures
// ------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// State functions

void dhcpSetState(uint8_t state)
{
    dhcpState = state;
}

uint8_t dhcpGetState()
{
    return dhcpState;
}
void setRequestBit(bool state)
{
    requestBit=state;
}

bool getRequestBit()
{
    return requestBit;
}

uint32_t getT1()
{
    return T1;
}

uint32_t getT2()
{
    return T2;
}


uint32_t getLease()
{
    return lease;
}


// DHCP control functions

void dhcpEnable()
{
    dhcpState = DHCP_INIT;
    // request new address
}

void dhcpDisable()
{
    dhcpState = DHCP_DISABLED;
    setRequestBit(1);
    //reset all values
//    uint8_t dummy[IP_ADD_LENGTH] = {0,0,0,0};
//    etherSetIpAddress(dummy);
//    etherSetIpSubnetMask(dummy);
//    etherSetIpGatewayAddress(dummy);
//    etherSetIpDnsAddress(dummy);
//    etherSetIpTimeServerAddress(dummy);
//    //dhcpRequestRelease();
//    //stop all timers
//    stopTimer((_callback)discovertimer);
//    stopTimer((_callback)requesttimer);
//    stopTimer((_callback)arptimer);
//    stopTimer((_callback)renewtimer);
//    stopTimer((_callback)renewRequestTimer);
//    stopTimer((_callback)rebindtimer);
//    stopTimer((_callback)leasetimer);

    // set state to disabled, stop all timers
}

bool dhcpIsEnabled()
{
    return (dhcpState != DHCP_DISABLED);
}

void dhcpRequestRenew()//THIS DOES NOT ACTUALLY SEND A MESSAGE CALLED AS RENEW. THEY SET A FLAG THAT REQUESTS FOR A RENEW/RELEASE                      //SOMETIMES YOU MIGHT HAVE TO SEND IT MORE THAN ONCE AS YOU MIGHT NOT GET A RESPONSE AND HENCE USING THE TIMER YOU HAVE TO RESEND IT MORE THAN ONCE(LIKE EVERY MINUTE )
{
    dhcpState=DHCP_RENEWING;
    setRequestBit(1);
}

void dhcpRequestRebind()
{
    dhcpState=DHCP_REBINDING;
}

void dhcpRequestRelease()
{
    dhcpState=DHCP_RELEASE;
}

uint32_t dhcpGetLeaseSeconds()
{
    return lease;
}


// Send DHCP message
void dhcpSendMessage(etherHeader *ether, uint8_t type) //this is generic for sending discover o renew. it will be same for ebertyhtin
{
    uint32_t sum=0;
    uint8_t i;
    //uint8_t opt;
    uint16_t tmp16;
    uint8_t mac[6];
    uint16_t udpLength;
    uint8_t state=dhcpGetState();
    // Ether frame
    etherGetMacAddress(mac); //get the ethernet MAC address


//    if(state==DHCP_INIT || state==DHCP_SELECTING || state==DHCP_REQUESTING)//SELECTING WHEN YOU ARE SENDING A REQUEST. IT WILL STILL BE IN
//    {
    for (i = 0; i < HW_ADD_LENGTH; i++) //then fill up the ethernet dest and source address
    {
        ether->destAddress[i] = 0xFF;
        ether->sourceAddress[i] = mac[i];
    }
//    }
//    else if(state==DHCP_RENEWING)
//    {
//

    ether->frameType = htons(0x800);  // this is IP-udp

    // IP header
    ipHeader* ip = (ipHeader*)ether->data;
    ip->revSize = 0x45; //size is 5 hence 5*4, the size of the IP is 20 bytes
    ip->typeOfService = 0;
    ip->id = 0;
    ip->flagsAndOffset = 0;
    ip->ttl = 128;
    ip->protocol = 17;
    ip->headerChecksum = 0;

    if(state==DHCP_RENEWING  || state==DHCP_RELEASE)
    {
        uint8_t temp[4];
        etherGetIpAddress(temp);
        for (i = 0; i < IP_ADD_LENGTH; i++)
        {
            ip->destIp[i] = dhcpServerIpAdd[i];
            ip->sourceIp[i] =temp[i];
        }
    }
    else if(state==DHCP_REBINDING)
    {
        uint8_t temp[4];
        etherGetIpAddress(temp);
        for (i = 0; i < IP_ADD_LENGTH; i++)
        {
            ip->destIp[i] = 0xFF;  //during rebinding the destination IP should be broadcast
            ip->sourceIp[i] =temp[i];
        }
    }
    else
    {
        for (i = 0; i < IP_ADD_LENGTH; i++)
        {
            ip->destIp[i] = 0xFF;
            ip->sourceIp[i] =0x00;
        }
    }

    // UDP header
    udpHeader* udp = (udpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    udp->sourcePort = htons(68);
    udp->destPort = htons(67);

    // DHCP
    dhcpFrame* dhcp = (dhcpFrame*)udp->data;
    dhcp->op = 1;
    // continue dhcp message here
    dhcp->htype=0x01; //hardware address type which denotes 10mb ethernet
    dhcp->hlen=6;  //6 for 10mb ethernet
    dhcp->hops=0;
    dhcp->xid=htonl(0x76465375);    ////make sure the transmitted and received values are same. this can be a random number and you have to make sure that the values are same when it is sent and received
    dhcp->secs=0;
    dhcp->flags=htons(0x8000);  //the first bit is for broadcast
    if(state==DHCP_RENEWING ||  state==DHCP_REBINDING)
    {
        uint8_t temp[4];
        etherGetIpAddress(temp);
        for(i=0;i<4;i++)
        {
            dhcp->ciaddr[i]=temp[i]; //during renew the ciaddr must be filled into extend the lease
            dhcp->yiaddr[i]=0;
            dhcp->siaddr[i]=0;
            dhcp->giaddr[i]=0;
        }
    }
    else
    {
        for(i=0;i<4;i++)
        {
            dhcp->ciaddr[i]=0;
            dhcp->yiaddr[i]=0;
            dhcp->siaddr[i]=0;
            dhcp->giaddr[i]=0;
        }
    }
    dhcp->chaddr[0]=2;  //client hardware address. only the first 6 will be used and the rest will be zeross
    dhcp->chaddr[1]=3;
    dhcp->chaddr[2]=4;
    dhcp->chaddr[3]=5;
    dhcp->chaddr[4]=6;
    dhcp->chaddr[5]=117;
    for(i=6;i<16;i++)
    {
        dhcp->chaddr[i]=0;
    }
    for(i=0;i<192;i++)
    {
        dhcp->data[i]=0;
    }
    dhcp->magicCookie=htonl(0x63825363); //magic cookie is 0x63825363
    dhcp->options[0]=53;
    dhcp->options[1]=1;


    // send requested ip (50) as needed. only required  in DHCP_requesting
    // send server ip (54) as needed. this is  also required only in requesting state i guess
    // send parameter request list (55)
    uint8_t j=2;
    if(type==DHCP_INIT)//22
    {
        dhcp->options[j++]=DHCPDISCOVER; //for DHCP DISCOEVR MESSAGE
        dhcp->options[j++]=61; //client identifier address
        dhcp->options[j++]=7;
        dhcp->options[j++]=0x01; //this is the MAC address of my device
        dhcp->options[j++]=0x02;
        dhcp->options[j++]=0x03;
        dhcp->options[j++]=0x04;
        dhcp->options[j++]=0x05;
        dhcp->options[j++]=0x06;
        dhcp->options[j++]=0xAB;
        dhcp->options[j++]=55;  //parameter request list
        dhcp->options[j++]=8; // length
        dhcp->options[j++]=1;  // subnet mask
        dhcp->options[j++]=2;  //time offset
        dhcp->options[j++]=3;  // router option
        dhcp->options[j++]=4;  //time server option
        dhcp->options[j++]=6; //  DNS OPTION
        dhcp->options[j++]=54; //ip address of the server
        dhcp->options[j++]=58;  ///T1 VALUE
        dhcp->options[j++]=59;  //T2 VALUE
        dhcp->options[j++]=255;
    }
    else if(type==DHCP_REQUESTING || type==DHCP_RENEWING || type==DHCP_REBINDING)
    {
        dhcp->options[j++]=DHCPREQUEST;   //for dhcp request
        dhcp->options[j++]=61; //client identifier address
        dhcp->options[j++]=7;
        dhcp->options[j++]=0x01;
        dhcp->options[j++]=0x02;
        dhcp->options[j++]=0x03;
        dhcp->options[j++]=0x04;
        dhcp->options[j++]=0x05;
        dhcp->options[j++]=0x06;
        dhcp->options[j++]=0xAB;
        dhcp->options[j++]=50; //requested IP address
        dhcp->options[j++]=0x4;
        dhcp->options[j++]=dhcpOfferedIpAdd[0];
        dhcp->options[j++]=dhcpOfferedIpAdd[1];
        dhcp->options[j++]=dhcpOfferedIpAdd[2];
        dhcp->options[j++]=dhcpOfferedIpAdd[3];
        dhcp->options[j++]=54; //ip address of the server
        dhcp->options[j++]=0x4;
        etherGetDhcpServerAddress(dhcpServerIpAdd);
        dhcp->options[j++]=dhcpServerIpAdd[0];
        dhcp->options[j++]=dhcpServerIpAdd[1];
        dhcp->options[j++]=dhcpServerIpAdd[2];
        dhcp->options[j++]=dhcpServerIpAdd[3];


        //REQUEST FOR 54 IN PARAMATER LIST TO DIFFERENTIATE FROM REPOSNE TO OFFER AND LEASE EXTENSIONS
        dhcp->options[j++]=55;  //parameter request list
        dhcp->options[j++]=8; // length
        dhcp->options[j++]=1;  // subnet mask
        dhcp->options[j++]=2;  //time offset
        dhcp->options[j++]=3;  // router option
        dhcp->options[j++]=4;  //time server option
        dhcp->options[j++]=6; //  DNS OPTION
        dhcp->options[j++]=54;
        dhcp->options[j++]=58;  ///T1 VALUE
        dhcp->options[j++]=59;  //T2 VALUE
        dhcp->options[j++]=255;
    }
    else if(type==DHCP_RELEASE)
    {
        dhcp->options[j++]=DHCPRELEASE;  //for dhcp release
        dhcp->options[j++]=61; //client identifier address
        dhcp->options[j++]=7;
        dhcp->options[j++]=0x01;
        dhcp->options[j++]=0x02;
        dhcp->options[j++]=0x03;
        dhcp->options[j++]=0x04;
        dhcp->options[j++]=0x05;
        dhcp->options[j++]=0x06;
        dhcp->options[j++]=0xAB;
        dhcp->options[j++]=255;
    }
    if(((j+1)&2)==1) //padding
    {
        dhcp->options[j++]=0x00;
    }
//    dhcp->options[j++]=55;  //parameter request list
//    dhcp->options[j++]=5; // length
//    dhcp->options[j++]=1;  // subnet mask
//    dhcp->options[j++]=2;  //time offset
//    dhcp->options[j++]=3;  // router option
//    dhcp->options[j++]=4;  //time server option
//    dhcp->options[j++]=6; //  DNS OPTION
//    dhcp->options[j++]=58;  ///T1 VALUE
//    dhcp->options[j++]=59;  //T2 VALUE
//    dhcp->options[j++]=255;

    // calculate dhcp size, update ip and udp lengths

    ip->length = htons(((ip->revSize & 0xF) * 4) + 8 + 240 + j);//size of IP header + udp header+ dhcp + options
    // calculate ip header checksum, calculate udp checksum
    sum=0;
    etherSumWords(&ip->revSize, 10, &sum);
    etherSumWords(ip->sourceIp, ((ip->revSize & 0xF) * 4) - 12,&sum);
    ip->headerChecksum=getEtherChecksum(sum);

    sum=0;

    //udp checksum, pseduoheader header contains IP source and dest address and protocol
    udpLength=(8 + 240 + j);
    udp->length = htons(udpLength);
    etherSumWords(ip->sourceIp,8,&sum);
    tmp16 = ip->protocol;
    sum += (tmp16 & 0xff) << 8;
    etherSumWords(&udp->length, 2, &sum);
    udp->check = 0; //making this zero so that when calculating checksum for udp header, the value of the checksum is not considered
    //calculate checksum for whole udp header and the data(which inclides dhcp)
    etherSumWords(udp, udpLength, &sum);
    udp->check = getEtherChecksum(sum);
    // send packet with size = ether hdr + ip header + udp hdr + dhcp_size

    etherPutPacket(ether, 14 + ((ip->revSize & 0xF) * 4) + 8 + 240 + j);
    // send packet
}
//return the pointer to the option ID
//extract the subnet mask/
uint8_t* getOption(etherHeader *ether, uint8_t option, uint8_t length)
{
    ipHeader* ip = (ipHeader*)ether->data;
    udpHeader* udp = (udpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    dhcpFrame* dhcp = (dhcpFrame*)&udp->data;
    // suggest this function to programatically extract the field value

    uint8_t i=0;
    bool flag=0;
    while(!(dhcp->options[i]==255) && flag==0)
    {
        if((dhcp->options[i]==3 || dhcp->options[i]==4 || dhcp->options[i]==6) && dhcp->options[i]==option)  //for dns, gw and time server as they have  variable lengths
        {
            flag=1;
            i+=2;
        }
        else if(dhcp->options[i]==option)
        {
            if(dhcp->options[++i]==length)
            {
                flag=1;
                i++; //refer to the options value
            }
        }
        else
        {
            i+=dhcp->options[++i]+1;
        }
    }
    if(flag==1)
        return &dhcp->options[i];
    else
        return NULL;
}
// Determines whether packet is DHCP offer response to DHCP discover
// Must be a UDP packet
//if it is an offer then point that msg(idk if message) to the offeredadd variable. if it is an offer it will extract out the IP address and provide it to you
//this is needed because next you will sending out an ARP request message. store the address that is offered. store it in ipOfferedAdd[].
//not only tests if the message is an offer but also stores the offered IP address
//if it is an offer turn on the red led or something on the TI board
bool dhcpIsOffer(etherHeader *ether, uint8_t ipOfferedAdd[])
{
    ipHeader* ip = (ipHeader*)ether->data;
    udpHeader* udp = (udpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    dhcpFrame* dhcp = (dhcpFrame*)&udp->data;
    uint8_t* off;
    bool ok;
    uint32_t sum=0;
    uint16_t tmp16=0;
    ok  = (ip->protocol == 17);   //checkign if it is a UDPs
//    ok &= (dhcp->chaddr[0] == 2);
//    ok &= (dhcp->chaddr[1] == 3);
//    ok &= (dhcp->chaddr[2] == 4);
//    ok &= (dhcp->chaddr[3] == 5);
//    ok &= (dhcp->chaddr[4] == 6);
//    ok &= (dhcp->chaddr[3] == 117);

    // return true if destport=68 and sourceport=67, op=2, xid correct, and offer msg
    ok &=(udp->sourcePort==htons(67) && udp->destPort==htons(68) && dhcp->op==2 && dhcp->xid==htonl(0x76465375));
    //ok &=(dhcp->options[0] == 53) && (dhcp->options[2] == DHCPOFFER);
    off=((uint8_t*)getOption(ether,53,1));
    if((*off)==DHCPOFFER)
        ok &=1;
    else
        ok &=0;

    //not required as checksum will already be calculated in when it is checked for UDP in the main function
//    if(ok)
//    {
//        etherSumWords(&ip->revSize, (ip->revSize & 0xF) * 4,&sum);
//        ok = (getEtherChecksum(sum) == 0);
//        sum = 0;
//        etherSumWords(ip->sourceIp, 8,&sum);
//        tmp16 = ip->protocol;
//        sum += (tmp16 & 0xff) << 8;
//        etherSumWords(&udp->length, 2,&sum);
//        // add udp header and data
//        etherSumWords(udp, ntohs(udp->length),&sum);
//        //etherSumWords(&udp->data, ntohs(240 + 34));
//        ok &= (getEtherChecksum(sum) == 0);
//    }
    if(ok)
    {
        //store the IP address offered to me
        ipOfferedAdd[0]=dhcp->yiaddr[0];
        ipOfferedAdd[1]=dhcp->yiaddr[1];
        ipOfferedAdd[2]=dhcp->yiaddr[2];
        ipOfferedAdd[3]=dhcp->yiaddr[3];

        //store the dhcp IP address
        etherSetDhcpServerAddress(getOption(ether,54,4));
//        dhcpServerIpAdd[0]=ip->sourceIp[0];
//        dhcpServerIpAdd[1]=ip->sourceIp[1];
//        dhcpServerIpAdd[2]=ip->sourceIp[2];
//        dhcpServerIpAdd[3]=ip->sourceIp[3];

    }
    return ok;

}

// Determines whether packet is DHCP ACK response to DHCP request
// Must be a UDP packet
bool dhcpIsAck(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    udpHeader* udp = (udpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    dhcpFrame* dhcp = (dhcpFrame*)&udp->data;
    // return true if destport=68 and sourceport=67, op=2, xid correct, and offer msg
    bool ok;
    uint8_t* ack;
    uint32_t sum=0;
    uint16_t tmp16=0;
    ok  = (ip->protocol == 17);   //checking if it is a UDP
    ok &= (dhcp->chaddr[0] == 2);
    ok &= (dhcp->chaddr[1] == 3);
    ok &= (dhcp->chaddr[2] == 4);
    ok &= (dhcp->chaddr[3] == 5);
    ok &= (dhcp->chaddr[4] == 6);
    ok &= (dhcp->chaddr[5] == 117);

    ok &=(udp->sourcePort==htons(67) && udp->destPort==htons(68) && dhcp->op==2 && dhcp->xid==htonl(0x76465375));
    //ok &=(dhcp->options[0] == 53) && (dhcp->options[2] == DHCPACK);

    ack=getOption(ether,53,1);
    if((*ack)==DHCPACK)
        ok&=1;
    else
        ok&=0;

    if(ok)
    {
        etherSumWords(&ip->revSize, (ip->revSize & 0xF) * 4,&sum);
        ok = (getEtherChecksum(sum) == 0);
        sum = 0;
        etherSumWords(ip->sourceIp, 8,&sum);
        tmp16 = ip->protocol;//protocol is a 8 bit number, we have to sign extend it to 16 bit number
        sum += (tmp16 & 0xff) << 8;
        etherSumWords(&udp->length, 2,&sum);
        // add udp header and data
        etherSumWords(udp, ntohs(udp->length),&sum);
        //etherSumWords(&udp->data, ntohs(240 + 34));
        ok &= (getEtherChecksum(sum) == 0);
    }
    return ok;
}

// Handle a DHCP ACK
void dhcpHandleAck(etherHeader *ether)
{
    ipHeader* ip = (ipHeader*)ether->data;
    udpHeader* udp = (udpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    dhcpFrame* dhcp = (dhcpFrame*)&udp->data;
    //uint8_t* extract[];
    uint32_t lease=0;
    uint8_t i;
    uint8_t l1,l2,l3,l4;
    // extract offered IP address
    for (i = 0; i < IP_ADD_LENGTH; i++)
    {
        dhcpOfferedIpAdd[i] = dhcp->yiaddr[i]; //store the offered IP address
        dhcpServerIpAdd[i] = ip->sourceIp[i];  //store the server IP address
    }
    // store sn, gw, dns, and time from options..use getoption()
    //subnet
    etherSetIpSubnetMask(getOption(ether,1,4));

    // store dns server address for later use
    etherSetIpDnsAddress(getOption(ether,6,4));

    //store the time sserver address
    etherSetIpTimeServerAddress(getOption(ether,4,4));

    //store the IP gateway address. dont know if this is the right option
    etherSetIpGatewayAddress(getOption(ether,3,4));

    //store the IP address in device
    etherSetIpAddress(dhcpOfferedIpAdd);
    // store lease, t1, and t2
    //retrieve from option 51
    uint8_t* leaseTime=getOption(ether,51,4);
    l1=leaseTime[0];
    l2=leaseTime[1];
    l3=leaseTime[2];
    l4=leaseTime[3];

    lease=(0x000000FF & l1) | (0x0000FF00 & l2) | (0xFF0000 & l3) | (0xFF000000 & l4);

//    leaseTime=getOption(ether,58,4);
//    T1=(0xFF & *(leaseTime+3)) | (0xFF00 & *(leaseTime+2)) (0xFF0000 & *(leaseTime+1)) | (0xFF000000 & *(leaseTime+0));
//
//    leaseTime=getOption(ether,59,4);
//    T2=(0xFF & *(leaseTime+3)) | (0xFF00 & *(leaseTime+2)) (0xFF0000 & *(leaseTime+1)) | (0xFF000000 & *(leaseTime+0));

    T1=lease/2;
    T2=(7*lease)/8;

    // stop new address needed timer, t1 timer, t2 timer
    // start t1, t2, and lease end timers
}

void dhcpSendPendingMessages(etherHeader *ether) // this is the only place where we will code to send messages
{
    // if discover needed, send discover, enter selecting state
    uint8_t state=dhcpGetState();
    if(state==DHCP_INIT)
    {
        dhcpSendMessage(ether,DHCP_INIT);
        initTimer();
        dhcpState=DHCP_SELECTING;
        startPeriodicTimer((_callback)discovertimer, 15);
        //START TIMER
    }
    // if request needed, send request. this is for renew maybe
    else if(state==DHCP_REQUESTING && getRequestBit())
    {
        dhcpSendMessage(ether,DHCP_REQUESTING);
        dhcpState=DHCP_REQUESTING;
        requestBit=0;
    }
    else if(state==DHCP_RENEWING && getRequestBit())
    {
        dhcpSendMessage(ether,DHCP_RENEWING);
        startPeriodicTimer((_callback)renewRequestTimer, 15);
        requestBit=0;
    }
    else if(state==DHCP_REBINDING)
    {
        dhcpSendMessage(ether,DHCP_REBINDING);
    }
    else if(state==DHCP_DISABLED && (getRequestBit()==1))
    {
        dhcpSendMessage(ether,DHCP_RELEASE);
        uint8_t dummy[IP_ADD_LENGTH] = {0,0,0,0};
        etherSetIpAddress(dummy);
        etherSetIpSubnetMask(dummy);
        etherSetIpGatewayAddress(dummy);
        etherSetIpDnsAddress(dummy);
        etherSetIpTimeServerAddress(dummy);
        //dhcpRequestRelease();
        //stop all timers
        stopTimer((_callback)discovertimer);
        stopTimer((_callback)requesttimer);
        stopTimer((_callback)arptimer);
        stopTimer((_callback)renewtimer);
        stopTimer((_callback)renewRequestTimer);
        stopTimer((_callback)rebindtimer);
        stopTimer((_callback)leasetimer);
        setRequestBit(0);

    }
    else if(state==DHCP_RELEASE)
    {
        dhcpSendMessage(ether,DHCP_RELEASE);
        //reset_values();
        uint8_t dummy[IP_ADD_LENGTH] = {0,0,0,0};
        etherSetIpAddress(dummy);
        etherSetIpSubnetMask(dummy);
        etherSetIpGatewayAddress(dummy);
        etherSetIpDnsAddress(dummy);
        etherSetIpTimeServerAddress(dummy);
        dhcpState=DHCP_INIT;
    }
    // if release needed, send release

}

void dhcpProcessDhcpResponse(etherHeader *ether)
{
    // if offer, send request and enter requesting state
    if(dhcpIsOffer(ether,dhcpOfferedIpAdd))
    {
        stopTimer((_callback)discovertimer);
        dhcpSendMessage(ether,DHCP_REQUESTING);
        dhcpState=DHCP_REQUESTING;
        startPeriodicTimer((_callback)requesttimer, 15);
    }
    // if ack, call handle ack, send arp request, enter ip conflict test state
    else if(dhcpIsAck(ether))
    {
        if(dhcpState==DHCP_REQUESTING)
        {
            stopTimer((_callback)requesttimer);
            dhcpHandleAck(ether);
            etherSendArpRequest(ether,dhcpOfferedIpAdd,dhcpServerIpAdd);
            dhcpState=DHCP_TESTING_IP;
            startOneshotTimer((_callback)arptimer,2); //don't know how many seconds
            //dhcpSendMessage(ether,53);

        }
        else if(dhcpState==DHCP_RENEWING  || dhcpState==DHCP_REBINDING)
        {
            stopTimer((_callback)renewRequestTimer);
            dhcpState=DHCP_BOUND;
            dhcpHandleAck(ether);
            startOneshotTimer((_callback)renewtimer,getT1());
            startOneshotTimer((_callback)rebindtimer,getT2());
            startOneshotTimer((_callback)leasetimer,getLease());
        }
    }

}

void dhcpProcessArpResponse(etherHeader *ether)
{
    arpPacket *arp = (arpPacket*)ether->data;
    //    ipHeader* ip = (ipHeader*)ether->data;
    //    udpHeader* udp = (udpHeader*)((uint8_t*)ip + ((ip->revSize & 0xF) * 4));
    //    dhcpFrame* dhcp = (dhcpFrame*)&udp->data;
    bool ok;

    ok =(dhcpOfferedIpAdd[0]==(arp->sourceIp[0]));
    ok&=(dhcpOfferedIpAdd[1]==(arp->sourceIp[1]));
    ok&=(dhcpOfferedIpAdd[2]==(arp->sourceIp[2]));
    ok&=(dhcpOfferedIpAdd[3]==(arp->sourceIp[3]));

    if(ok)
    {
        stopTimer((_callback)arptimer);
        dhcpState=DHCP_RELEASE;
        dhcpSendMessage(ether,DHCP_RELEASE);
        dhcpState=DHCP_INIT;
    }
    // if in conflict resolution, if a response matches the offered add,
    //  send decline and request new address
}
