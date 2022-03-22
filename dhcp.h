// DHCP Library
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

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef DHCP_H_
#define DHCP_H_

#include <stdint.h>
#include <stdbool.h>
#include "eth0.h"

//THES ARE THE 8 MAGIC NUMBERS WHICH WILL BE  OF USE WHEN WE USE A PROTOCL CALLED AS BOOTP  AND AN OPTION CALLED AS 53 WHICH WILL TELL IN WHICH DHCP MESSAGE WE ARE SENDING INFO ON
#define DHCPDISCOVER 1
#define DHCPOFFER    2
#define DHCPREQUEST  3
#define DHCPDECLINE  4
#define DHCPACK      5
#define DHCPNAK      6
#define DHCPRELEASE  7
#define DHCPINFORM   8

//THESE ARE THE STATES
#define DHCP_DISABLED   0
#define DHCP_INIT       1
#define DHCP_SELECTING  2
#define DHCP_REQUESTING 3
#define DHCP_TESTING_IP 4
#define DHCP_BOUND      5
#define DHCP_RENEWING   6
#define DHCP_REBINDING  7
#define DHCP_INITREBOOT 8 // not used since ip not stored over reboot
#define DHCP_REBOOTING  9 // not used since ip not stored over reboot

#define DHCP_DECLINE      10
#define DHCP_RELEASE      11

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void dhcpSendPendingMessages(etherHeader *ether);
void dhcpProcessDhcpResponse(etherHeader *ether);
void dhcpProcessArpResponse(etherHeader *ether);

void dhcpEnable(void);
void dhcpDisable(void);
bool dhcpIsEnabled(void);

void dhcpRequestRenew(void);
void dhcpRequestRebind(void);
void dhcpRequestRelease(void);
void dhcpSetState(uint8_t);
uint8_t dhcpGetState();
uint32_t getT2(void);
uint32_t getT1(void);
uint32_t getLease(void);
void setRequestBit(bool);
bool getRequestBit();

uint32_t dhcpGetLeaseSeconds();

#endif

