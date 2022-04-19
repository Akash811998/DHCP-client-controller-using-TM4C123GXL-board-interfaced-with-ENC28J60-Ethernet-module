// Ethernet Example
// Jason Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL w/ ENC28J60
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// ENC28J60 Ethernet controller on SPI0
//   MOSI (SSI0Tx) on PA5
//   MISO (SSI0Rx) on PA4
//   SCLK (SSI0Clk) on PA2
//   ~CS (SW controlled) on PA3
//   WOL on PB3
//   INT on PC6

// Pinning for IoT projects with wireless modules:
// N24L01+ RF transceiver
//   MOSI (SSI0Tx) on PA5
//   MISO (SSI0Rx) on PA4
//   SCLK (SSI0Clk) on PA2
//   ~CS on PE0
//   INT on PB2
// Xbee module
//   DIN (UART1TX) on PC5
//   DOUT (UART1RX) on PC4

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "eeprom.h"
#include "gpio.h"
#include "spi0.h"
#include "uart0.h"
#include "wait.h"
#include "timer.h"
#include "eth0.h"
#include "dhcp.h"

// Pins
#define RED_LED PORTF,1
#define BLUE_LED PORTF,2
#define GREEN_LED PORTF,3
#define PUSH_BUTTON PORTF,4

//-----------------------------------------------------------------------------
// Subroutines                
//-----------------------------------------------------------------------------

int x = 0;
uint8_t dummy[IP_ADD_LENGTH] = {0,0,0,0};


// Initialize Hardware
void initHw()
{
    // Initialize system clock to 40 MHz
    initSystemClockTo40Mhz();

    // Enable clocks
    enablePort(PORTF);
    _delay_cycles(3);

    // Configure LED and pushbutton pins
    selectPinPushPullOutput(RED_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(BLUE_LED);
    selectPinDigitalInput(PUSH_BUTTON);
    enablePinPullup(PUSH_BUTTON);
}
void reset_values()
{
    etherSetIpAddress(dummy);
    etherSetIpSubnetMask(dummy);
    etherSetIpGatewayAddress(dummy);
    etherSetIpDnsAddress(dummy);
    etherSetIpTimeServerAddress(dummy);
}
void discovertimer()
{
    dhcpSetState(DHCP_INIT);
}
void requesttimer()
{
    setRequestBit(1);
}

void arptimer()
{
//    if(getRequestBit()==1)
//    {
//        reset_values();
//        dhcpSetState(DHCP_INIT);
//    }
//    else
//    {
    dhcpSetState(DHCP_BOUND);
    startOneshotTimer((_callback)renewtimer,getT1());
    startOneshotTimer((_callback)rebindtimer,getT2());
    startOneshotTimer((_callback)leasetimer,getLease());
////    startOneshotTimer((_callback)renewtimer,5);
//    startOneshotTimer((_callback)rebindtimer,10);
////  startOneshotTimer((_callback)leasetimer,15);

}

void renewtimer()
{
    dhcpRequestRenew();
    setRequestBit(1);
}

void renewRequestTimer()
{
    dhcpRequestRenew();
    setRequestBit(1);
}

void rebindtimer()
{
    dhcpRequestRebind();
}

void leasetimer()
{
    dhcpSetState(DHCP_RELEASE);

}

void displayConnectionInfo()
{
    uint8_t i;
    char str[10];
    uint8_t mac[6];
    uint8_t ip[4];
    etherGetMacAddress(mac);
    putsUart0("  HW:    ");
    for (i = 0; i < 6; i++)
    {
        sprintf(str, "%02x", mac[i]);
        putsUart0(str);
        if (i < 6-1)
            putcUart0(':');
    }
    putsUart0("\n\r");
    etherGetIpAddress(ip);
    putsUart0("  IP:    ");
    for (i = 0; i < 4; i++)
    {
        sprintf(str, "%u", ip[i]);
        putsUart0(str);
        if (i < 4-1)
            putcUart0('.');
    }
    if (dhcpIsEnabled())
        putsUart0(" (dhcp)");
    else
        putsUart0(" (static)");
    putsUart0("\n\r");
    etherGetIpSubnetMask(ip);
    putsUart0("  SN:    ");
    for (i = 0; i < 4; i++)
    {
        sprintf(str, "%u", ip[i]);
        putsUart0(str);
        if (i < 4-1)
            putcUart0('.');
    }
    putsUart0("\n\r");
    etherGetIpGatewayAddress(ip);
    putsUart0("  GW:    ");
    for (i = 0; i < 4; i++)
    {
        sprintf(str, "%u", ip[i]);
        putsUart0(str);
        if (i < 4-1)
            putcUart0('.');
    }
    putsUart0("\n\r");
    etherGetIpDnsAddress(ip);
    putsUart0("  DNS:   ");
    for (i = 0; i < 4; i++)
    {
        sprintf(str, "%u", ip[i]);
        putsUart0(str);
        if (i < 4-1)
            putcUart0('.');
    }
    putsUart0("\n\r");
    etherGetIpTimeServerAddress(ip);
    putsUart0("  Time:  ");
    for (i = 0; i < 4; i++)
    {
        sprintf(str, "%u", ip[i]);
        putsUart0(str);
        if (i < 4-1)
            putcUart0('.');
    }
    putsUart0("\n\r");
    if (dhcpIsEnabled())
    {
        putsUart0("  Lease: ");
        uint32_t s, m, h, d;
        s = dhcpGetLeaseSeconds();
        d = s / (24*60*60);
        s -= d * (24*60*60);
        h = s / (60*60);
        s -= h * (60*60);
        m = s / 60;
        sprintf(str, "%ud;%02uh:%02um\r", d, h, m);
        putsUart0(str);
        putsUart0("\n\r");
    }
    if (etherIsLinkUp())
        putsUart0("\rLink is up\n\r");
    else
        putsUart0("\rLink is down\n\r");
}

void readConfiguration()
{
    uint32_t temp;
    uint8_t* ip;

    if (readEeprom(1) == 0xFFFFFFFF) //EEPROM that has never been programmed are always ones. if all ones then by default enable dhcp
    {
        dhcpEnable();
    }
    else
    {
        dhcpDisable(); //if dhcp is modified, then take the IP address from 2nd location in the EEPROM
        temp = readEeprom(2);
        if (temp != 0xFFFFFFFF)
        {
            ip = (uint8_t*)&temp;
            etherSetIpAddress(ip);
        }
        temp = readEeprom(3); //IP subnet mask  from 3rd location
        if (temp != 0xFFFFFFFF)
        {
            ip = (uint8_t*)&temp;
            etherSetIpSubnetMask(ip);
        }
        temp = readEeprom(4); //IP gateway address  from 4th location
        if (temp != 0xFFFFFFFF)
        {
            ip = (uint8_t*)&temp;
            etherSetIpGatewayAddress(ip);
        }
        temp = readEeprom(5);  //IP DNS address  from 5th location
        if (temp != 0xFFFFFFFF)
        {
            ip = (uint8_t*)&temp;
            etherSetIpDnsAddress(ip);
        }
        temp = readEeprom(6); //IP TIME SERVER address  from 6th location
        if (temp != 0xFFFFFFFF)
        {
            ip = (uint8_t*)&temp;
            etherSetIpTimeServerAddress(ip);
        }
    }
}

#define MAX_CHARS 80
char strInput[MAX_CHARS+1];
char* token;
uint8_t count = 0;

uint8_t asciiToUint8(const char str[])
{
    uint8_t data;
    if (str[0] == '0' & tolower(str[1]) == 'x')
        sscanf(str, "%hhx", &data);
    else
        sscanf(str, "%hhu", &data);
    return data;
}

void processShell()
{
    bool end;
    char c;
    uint8_t i;
    uint8_t ip[4];
    uint32_t* p32;

    if (kbhitUart0())
    {
        c = getcUart0();

        end = (c == 13) || (count == MAX_CHARS);
        if (!end)
        {
            if ((c == 8 || c == 127) & count > 0)
                count--;
            if (c >= ' ' & c < 127)
                strInput[count++] = c;
        }
        else
        {
            strInput[count] = '\0';
            count = 0;
            token = strtok(strInput, " ");
            if (strcmp(token, "dhcp") == 0)
            {
                token = strtok(NULL, " ");
                if (strcmp(token, "renew") == 0)
                {
                    dhcpRequestRenew();
                }
                else if (strcmp(token, "release") == 0)
                {
                    dhcpRequestRelease();
                }
                else if (strcmp(token, "on") == 0)
                {
                    dhcpEnable();
                    writeEeprom(1, 0xFFFFFFFF);
                }
                else if (strcmp(token, "off") == 0)
                {
                    dhcpDisable();
                    writeEeprom(1, 0);
                }
                else
                    putsUart0("Error in dhcp argument\r\n");
            }
            if (strcmp(token, "ifconfig") == 0)
            {
                displayConnectionInfo();
            }
            if (strcmp(token, "reboot") == 0)
            {
                NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            }
            if (strcmp(token, "set") == 0)
            {
                token = strtok(NULL, " ");
                if (strcmp(token, "ip") == 0)
                {
                    for (i = 0; i < 4; i++)
                    {
                        token = strtok(NULL, ".");
                        ip[i] = asciiToUint8(token);
                    }
                    etherSetIpAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(2, *p32);
                }
                if (strcmp(token, "sn") == 0)
                {
                    for (i = 0; i < 4; i++)
                    {
                        token = strtok(NULL, ".");
                        ip[i] = asciiToUint8(token);
                    }
                    etherSetIpSubnetMask(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(3, *p32);
                }
                if (strcmp(token, "gw") == 0)
                {
                    for (i = 0; i < 4; i++)
                    {
                        token = strtok(NULL, ".");
                        ip[i] = asciiToUint8(token);
                    }
                    etherSetIpGatewayAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(4, *p32);
                }
                if (strcmp(token, "dns") == 0)
                {
                    for (i = 0; i < 4; i++)
                    {
                        token = strtok(NULL, ".");
                        ip[i] = asciiToUint8(token);
                    }
                    etherSetIpDnsAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(5, *p32);
                }
                if (strcmp(token, "time") == 0)
                {
                    for (i = 0; i < 4; i++)
                    {
                        token = strtok(NULL, ".");
                        ip[i] = asciiToUint8(token);
                    }
                    etherSetIpTimeServerAddress(ip);
                    p32 = (uint32_t*)ip;
                    writeEeprom(6, *p32);
                }
            }

            if (strcmp(token, "help") == 0)
            {
                putsUart0("\n\rCommands:\r\n");
                putsUart0("  dhcp on|off|renew|release\r\n");
                putsUart0("  ifconfig\r\n");
                putsUart0("  reboot\r\n");
                putsUart0("  set ip|gw|dns|time|sn w.x.y.z\r\n");
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

// Max packet is calculated as:
// Ether frame header (18) + Max MTU (1500) + CRC (4)
#define MAX_PACKET_SIZE 1522

int main(void)
{
    initHw();//configure ports and LEDS's
    uint8_t* udpData;
    uint8_t buffer[MAX_PACKET_SIZE];
    etherHeader *data = (etherHeader*) buffer;

    // Init controller


    // Setup UART0
    initUart0(); //initialise UART for terminal interfacing
    setUart0BaudRate(115200, 40e6);

    // Init timer
//    initTimer(); //initialise the timer

    // Init ethernet interface (eth0)
    putsUart0("\n\rStarting eth0\n\r");
    etherInit(ETHER_UNICAST | ETHER_BROADCAST | ETHER_HALFDUPLEX);
    etherSetMacAddress(2, 3, 4, 5, 6, 117);

    // Init EEPROM
    initEeprom();
    readConfiguration();

    setPinValue(GREEN_LED, 1);
    waitMicrosecond(100000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(100000);

    // Main Loop
    // RTOS and interrupts would greatly improve this code,
    // but the goal here is simplicity
    while (true)
    {
        // Put terminal processing here
        processShell();

        // DHCP maintenance
        if (dhcpIsEnabled() || getRequestBit())
        {
            dhcpSendPendingMessages(data); //where discovery, request and release is sent depending upon the state. check for states and then change th state
            //variables as required

            // Packet processing
            if (etherIsDataAvailable())
            {
                if (etherIsOverflow())
                {
                    setPinValue(RED_LED, 1);
                    waitMicrosecond(100000);
                    setPinValue(RED_LED, 0);
                }

                // Get packet
                etherGetPacket(data, MAX_PACKET_SIZE);

                // Handle ARP request
                if (etherIsArpRequest(data))
                    etherSendArpResponse(data);

                // Handle ARP response
                if (etherIsArpResponse(data))
                {
//                    if(dhcpGetState()==DHCP_TESTING_IP)
//                    {
//
//                    }
                    dhcpProcessArpResponse(data);
//                    if(dhcpGetState()==DHCP_TESTING_IP)
//                        setRequestBit(1);
                }

                // Handle IP datagram
                if (etherIsIp(data))
                {
                    if (etherIsUdp(data))
                    {
                        if (etherIsDhcpResponse(data))
                            dhcpProcessDhcpResponse(data); //where offer is sent
                    }
                    else if(etherIsIpUnicast(data))
                    {
                        // Handle ICMP ping request
                        if (etherIsPingRequest(data))
                        {
                            etherSendPingResponse(data);
                        }

                        // Process UDP datagram
                        // test this with a udp send utility like sendip
                        //   if sender IP (-is) is 192.168.1.198, this will attempt to
                        //   send the udp datagram (-d) to 192.168.1.199, port 1024 (-ud)
                        // sudo sendip -p ipv4 -is 192.168.1.198 -p udp -ud 1024 -d "on" 192.168.1.199
                        // sudo sendip -p ipv4 -is 192.168.1.198 -p udp -ud 1024 -d "off" 192.168.1.199
                        if (etherIsUdp(data))
                        {
                            udpData = etherGetUdpData(data);
                            if (strcmp((char*)udpData, "on") == 0)
                                setPinValue(GREEN_LED, 1);
                            if (strcmp((char*)udpData, "off") == 0)
                                setPinValue(GREEN_LED, 0);
                            etherSendUdpResponse(data, (uint8_t*)"Received", 9);
                        }
                    }
                }
            }
        }
    }
}

