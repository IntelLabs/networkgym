/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "phy-access-control.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PhyAccessControl");

NS_OBJECT_ENSURE_REGISTERED (PhyAccessControl);

PhyAccessControl::PhyAccessControl ()
{
  NS_LOG_FUNCTION (this);
  //Simulator::Schedule(m_rssiScanInterval, &PhyAccessControl::ScanRssi, this);
}


TypeId
PhyAccessControl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PhyAccessControl")
    .SetParent<Object> ()
    .SetGroupName("Gma")
    .AddAttribute ("SimulateLinkDownTime",
       "This timer simulate the time to configure a new wifi AP using single radio, i.e., during this time packets are dropped.",
       TimeValue (MilliSeconds (0)),
       MakeTimeAccessor (&PhyAccessControl::m_simulateLinkDownTime),
       MakeTimeChecker ())
 .AddAttribute ("RoamingLowRssi",
               "Disconnect the current AP if the RSSI is lower than this value"
               "After discunnect client will look for the one with max RSSI",
               DoubleValue (-79.0),
               MakeDoubleAccessor (&PhyAccessControl::SetRoamingLowRssi),
               MakeDoubleChecker<double> ())
  ;
  return tid;
}


PhyAccessControl::~PhyAccessControl ()
{
	NS_LOG_FUNCTION (this);
}

void
PhyAccessControl::SetRoamingLowRssi(double rssi)
{
	m_roamingLowRssi = rssi;
}

void
PhyAccessControl::AddCandidate (Ptr<Socket> socket, const Ipv4Address& phyAddr, const Mac48Address& macAddr, uint8_t apId)
{
	m_idToIpMap[apId] = phyAddr;
	m_macAddrToIdMap[macAddr] = apId;
}

int
PhyAccessControl::GetApIdFromMacAddr (const Mac48Address& macAddr)
{
	if(m_macAddrToIdMap.find(macAddr) == m_macAddrToIdMap.end())
	{
		return -1;
	}
	else
	{
		return m_macAddrToIdMap[macAddr];
	}
}


void
PhyAccessControl::ReportRssi (double rssi, uint8_t apId)
{
	m_idToRssiMap[apId] = rssi;
}

double
PhyAccessControl::GetCurrentApRssi ()
{
	if(m_idToRssiMap.find(m_currentApId) == m_idToRssiMap.end())
	{
		return -1000;//no trace available
	}
	else
	{
		return m_idToRssiMap[m_currentApId];
	}
}

void
PhyAccessControl::ScanRssi()
{
	if(m_idToRssiMap.size() > 0)
	{
		NS_ASSERT_MSG(m_idToRssiMap.find(m_currentApId) != m_idToRssiMap.end(), "cannot find m_currentApId in the map!");
		if(m_simulateLinkDownTime > Seconds(0) && m_idToRssiMap[m_currentApId] > m_roamingLowRssi)
		{
			//single radio and current ap power larger than low thresh... stay in current AP;
			//no action
		}
		else
		{

			double maxRssi = -1000; //random small number;
			int maxApId = m_currentApId;
			std::map < uint8_t, double >::iterator iter = m_idToRssiMap.begin();
			//std::cout << Now().GetSeconds();
			while(iter != m_idToRssiMap.end())
			{
				if(iter->second > maxRssi)
				{
					maxRssi = iter->second;
					maxApId = iter->first;
				}
				//std::cout << " AP:" << +iter->first << " rssi:" << iter->second;
				iter++;
			}
			//std::cout << "\n";
			NS_ASSERT_MSG(m_idToRssiMap.find(m_currentApId) != m_idToRssiMap.end(), "cannot find m_currentApId in the map!");
			if(maxApId != m_currentApId && (maxRssi > (m_idToRssiMap[m_currentApId] + WIFI_ROAMING_RSSI_THRESH)))
			{
				std::cout << " WIFI ROAMING[start]<<<<<<<<<<<<<<<  current AP:" << +m_currentApId <<". Find a better AP:" << +maxApId << "\n";
				m_bestApId = maxApId;
				//NS_ASSERT_MSG(m_idToIpMap.find(maxApId) != m_idToIpMap.end(), "cannot find maxApId in the map!");
				//m_ipAddr = m_idToIpMap[maxApId];
			}
		}
	}
	//Simulator::Schedule(m_rssiScanInterval, &PhyAccessControl::ScanRssi, this);
}

void
PhyAccessControl::SetSocket (Ptr<Socket> socket)
{
	m_socket = socket;
}

void
PhyAccessControl::SetPortNum (uint16_t port)
{
	m_portNumber = port;
}

void
PhyAccessControl::SetIp (const Ipv4Address& phyAddr, bool linkDown)
{
	m_ipAddr = phyAddr;
	if(linkDown)
	{
		m_ipChangeTime = Now();
	}
}

Ipv4Address&
PhyAccessControl::GetIp ()
{
	return m_ipAddr;
}

void
PhyAccessControl::SetApId (uint8_t id)
{
	m_currentApId = id;
	m_bestApId = id;
}

uint8_t
PhyAccessControl::GetApId ()
{
	return m_currentApId;
}

bool
PhyAccessControl::ProbeAcked(uint16_t sn)
{
	if(m_testProbeAcked == false)
	{
		std::cout << "Wait for probe ack sn:" << m_testProbeSn << "&&&&&&&&&&&&& acked sn:" << sn << "\n";

		if(m_testProbeSn == sn)
		{
			//acked. we can change to the new AP... the server side will be updated when receicing the next probe...
			std::cout << " WIFI ROAMING[STOP]<<<<<<<<<<<<<<<<< change from AP:" << +m_currentApId <<" to AP:" << +m_bestApId << "\n";
			//but the IP address will be changed at the next probe....
			m_currentApId = m_bestApId;
			NS_ASSERT_MSG(m_idToIpMap.find(m_currentApId) != m_idToIpMap.end(), "cannot find currentip in the map!");
			m_ipAddr = m_idToIpMap[m_currentApId];
			m_ipChangeTime = Now();
			m_testProbeAcked = true;
			return true;//update rto;
		}
	}
	return false;//dont update rto
}

void
PhyAccessControl::TestProbeTimeout()
{
	if(m_testProbeAcked == false)
	{
		//test probe failed, no update
		m_bestApId = m_currentApId;
		m_testProbeAcked = true;
	}
}

bool
PhyAccessControl::SendPacket (Ptr<Packet> pkt, uint8_t tos)
{
	bool linkDownFlag = false;
	if(m_simulateLinkDownTime > Seconds(0))//single radio enabled...
	{
		if(Now() < m_ipChangeTime + m_simulateLinkDownTime)
		{
			std::cout << Now().GetSeconds() << " (2) simulate link down, drop packet at TX SIDE!!!\n";
			return linkDownFlag;
		}
	}

	
	Ptr<Packet> pcopy = pkt->Copy();
	GmaHeader gmaHeader;
	pcopy->RemoveHeader(gmaHeader);

	if(gmaHeader.GetFlags () == 256)
	{
		//control
		MxControlHeader mxHeader;
		pcopy->RemoveHeader(mxHeader);

		if(mxHeader.GetType() == 1)//probe message
		{
			ScanRssi(); //check is the best AP is changed...
			if(m_currentApId != m_bestApId)//test probe over best Ap
			{
				NS_ASSERT_MSG(m_idToRssiMap.find(m_currentApId) != m_idToRssiMap.end(), "cannot find m_currentApId in the map!");
				if(m_simulateLinkDownTime > Seconds(0))
				{
					//single radio, no test probe
					m_currentApId = m_bestApId;
					NS_ASSERT_MSG(m_idToIpMap.find(m_currentApId) != m_idToIpMap.end(), "cannot find currentip in the map!");
					m_ipAddr = m_idToIpMap[m_currentApId];
					m_ipChangeTime = Now();
					linkDownFlag = true;
					std::cout << " WIFI ROAMING[STOP]<<<<<<<<<<<<<<<<< change from AP:" << +m_currentApId <<" to AP:" << +m_bestApId << "\n";
				}
				else if(m_idToRssiMap[m_currentApId] < m_roamingLowRssi)
				{
					//RSSI of current AP is low, switch without test probe....
					m_currentApId = m_bestApId;
					NS_ASSERT_MSG(m_idToIpMap.find(m_currentApId) != m_idToIpMap.end(), "cannot find currentip in the map!");
					m_ipAddr = m_idToIpMap[m_currentApId];
					m_ipChangeTime = Now();
					linkDownFlag = true;
					std::cout << " WIFI ROAMING[STOP]<<<<<<<<<<<<<<<<< change from AP:" << +m_currentApId <<" to AP:" << +m_bestApId << "\n";
				}
				else
				{
					if(m_testProbeAcked == true)
					{
						std::cout << Now().GetSeconds() <<"This is a probe msg sn: " << mxHeader.GetSequenceNumber() << "  current:" << +m_currentApId << " best:" << +m_bestApId << "\n";
						//send a test probe...

						MxControlHeader newMxHeader = mxHeader;
						newMxHeader.SetProbeFlag(0);

						Ptr<Packet> newP = Create<Packet>();
						newP->AddHeader (newMxHeader);

						GmaHeader newGmaHeader;
						newGmaHeader.SetClientId(gmaHeader.GetClientId());
						newP->AddHeader (newGmaHeader);

						if(!m_socket)
						{
							std::cout << Now().GetSeconds() << " socket not setup yet! not TX" << "\n";
						}
						else
						{
							//probe the best ap
							NS_ASSERT_MSG(m_idToIpMap.find(m_bestApId) != m_idToIpMap.end(), "cannot find bestApId in the map!");
							m_socket->SendTo (newP, 0 ,InetSocketAddress (m_idToIpMap[m_bestApId], m_portNumber));
							m_testProbeAcked = false;
							m_testProbeSn = mxHeader.GetSequenceNumber();
							Simulator::Schedule(Seconds(1), &PhyAccessControl::TestProbeTimeout, this);
						}
						return linkDownFlag;
					}
				}
			}
		}
	}


	if(!m_socket)
	{
		std::cout << Now().GetSeconds() << " socket not setup yet! not TX" << "\n";
	}
	else
	{
		//m_socket->SendTo (pkt, 0 ,InetSocketAddress (m_ipAddr, m_portNumber));
		InetSocketAddress destAddr (m_ipAddr, m_portNumber);
		destAddr.SetTos(tos);		

		//std::cout << " tos print:" << +destAddr.GetTos() << std::endl;
		m_socket->SendTo (pkt, 0 ,destAddr);
	}
	return linkDownFlag;
}

int
PhyAccessControl::CsnDiff(int x1, int x2)
{
	int diff = x1 - x2;
	if (diff > 32768)
	{
		diff = diff - 65536;
	}
	else if (diff < -32768)
	{
		diff = diff + 65536;
	}
	return diff;
}

bool
PhyAccessControl::ReceiveOk (const Ipv4Address& phyAddr)
{
	bool receiveOk = true;
	if(m_simulateLinkDownTime > Seconds(0))//single radio enabled...
	{
		if(phyAddr != m_ipAddr)
		{
			std::cout << Now().GetSeconds() << " (1) simulate single radio, drop packet!!!\n";
			receiveOk = false;
		}
		else if(Now() < m_ipChangeTime + m_simulateLinkDownTime)
		{
			std::cout << Now().GetSeconds() << " (2) simulate link down, drop packet!!!\n";
			receiveOk = false;
		}
	}
	return receiveOk;
}

Time
PhyAccessControl::GetLinkDownTime ()
{
	return m_simulateLinkDownTime;
}


}
