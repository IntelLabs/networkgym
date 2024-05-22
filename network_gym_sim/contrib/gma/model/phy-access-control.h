/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef PHY_ACCESS_CONTROL_H
#define PHY_ACCESS_CONTROL_H

#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/virtual-net-device.h>
#include "mx-control-header.h"
#include <ns3/integer.h>
#include "gma-header.h"
namespace ns3 {

// This class selects the best access point for a RAT. For example, the best Wi-Fi AP for Wi-Fi connection.
// It will perform handover to the best candidate AP using probe messages, it also reordering packets (if enabled) during handover.
// GMA will perceive one radio acess technology (RAT) as a connection, e.g., Wi-Fi or LTE, regardless of which AP is connected.
// Therefore, GMA does not need to be aware of AP updates

class PhyAccessControl : public Object
{
public:
	PhyAccessControl ();
	virtual ~PhyAccessControl ();
  	static TypeId GetTypeId (void);
    //add candidate will be called only by client... Server will cal setIP and get IP.
  	void AddCandidate (Ptr<Socket> socket, const Ipv4Address& phyAddr, const Mac48Address& macAddr, uint8_t apId);
  	void ReportRssi (double rssi, uint8_t apInd);
    double GetCurrentApRssi ();
  	void ScanRssi ();
  	void SetSocket (Ptr<Socket> socket);
  	void SetPortNum (uint16_t port);
  	void SetIp (const Ipv4Address& phyAddr, bool linkDown = false);
    Ipv4Address& GetIp ();
  	void SetApId (uint8_t apId);
  	uint8_t GetApId ();
  	bool SendPacket (Ptr<Packet> pkt, uint8_t tos);//return true if virtual interface need to set control msg flag down...
    bool ProbeAcked (uint16_t sn);//return true if gma need to reset rto due to link ip change.
    
    int CsnDiff(int x1, int x2); //control sn
    bool ReceiveOk (const Ipv4Address& phyAddr);//this is for simulate single radio with donw time...
    Time GetLinkDownTime ();
    void SetRoamingLowRssi(double rssi);
    void TestProbeTimeout ();
	int GetApIdFromMacAddr (const Mac48Address& macAddr);
private:
	std::map < uint8_t, Ipv4Address > m_idToIpMap;
	std::map < Mac48Address ,uint8_t > m_macAddrToIdMap;

	std::map < uint8_t, double > m_idToRssiMap;
	Time m_rssiScanInterval = Seconds(1.0);
	uint8_t m_currentApId = UINT8_MAX;
  uint8_t m_bestApId = 0;
	Ipv4Address m_ipAddr; //ip address per link
	Ptr<Socket> m_socket; //socket per link
	uint16_t m_portNumber = 0; //port number for transmit pkt
  double WIFI_ROAMING_RSSI_THRESH = 3; //roaming happens if current AP's RSSI + TRESH < max RS`SI of other APs.
  bool m_testProbeAcked = true;
  uint16_t m_testProbeSn = 0;
  Time m_simulateLinkDownTime;
  Time m_ipChangeTime = Seconds(0);
  double m_roamingLowRssi = 0.0;
};

}

#endif /* PHY_ACCESS_CONTROL_H */

