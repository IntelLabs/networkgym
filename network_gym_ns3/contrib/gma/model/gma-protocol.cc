/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "gma-protocol.h"
#include "ns3/point-to-point-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GmaProtocol");

NS_OBJECT_ENSURE_REGISTERED (GmaProtocol);

GmaProtocol::GmaProtocol ()
{
  NS_FATAL_ERROR ("should not use this constructor!!");
  NS_LOG_FUNCTION (this);
}


GmaProtocol::GmaProtocol(Ptr<Node> node)
{
	m_node = node;
	m_nodeId = node->GetId();
}

TypeId
GmaProtocol::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GmaProtocol")
    .SetParent<Object> ()
    .SetGroupName("Gma")
  ;
  return tid;
}


GmaProtocol::~GmaProtocol ()
{
	NS_LOG_FUNCTION (this);
}

void
GmaProtocol::AddLocalVirtualInterface (const Ipv4Address& address, Ptr<NetDevice> dev)
{
	std::cout << this<< " local virtual IP:" << address << "\n";
	//copied from virtual device example
	if(dev == nullptr)
	{
		//the forward device is not set, means GMA located at the receiver device, just forward packet to high layers.
		//m_virtualDevice will transmit packets, received packets are forwarded to m_virtualDevice
		NS_ASSERT_MSG(m_virtualDevice == nullptr, "cannot install the same device twice");
		m_virtualDevice = CreateObject<VirtualNetDevice> ();
		//m_TapDevice->SetAddress (Mac48Address ("11:00:01:02:03:01"));
		m_virtualDevice->SetSendCallback (MakeCallback (&GmaProtocol::VirtualSend, this));
		m_node->AddDevice (m_virtualDevice);

		Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
		uint32_t i = ipv4->AddInterface (m_virtualDevice);
		//ipv4->AddAddress (i, Ipv4InterfaceAddress (Ipv4Address ("7.0.1.1"), Ipv4Mask ("255.255.255.0")));
		ipv4->AddAddress (i, Ipv4InterfaceAddress (address, Ipv4Mask ("255.255.255.0")));
		ipv4->SetUp (i);
	}
	else
	{
		//the receiver device is set, forward the packets to the real receiver.
		//virD will transmit packets, received packets will be forwarded to dev
		Ptr<VirtualNetDevice> virD = CreateObject<VirtualNetDevice> ();
		//m_TapDevice->SetAddress (Mac48Address ("11:00:01:02:03:01"));
		virD->SetSendCallback (MakeCallback (&GmaProtocol::VirtualSend, this));
		m_node->AddDevice (virD);

		Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
		uint32_t i = ipv4->AddInterface (virD);
		//ipv4->AddAddress (i, Ipv4InterfaceAddress (Ipv4Address ("7.0.1.1"), Ipv4Mask ("255.255.255.0")));
		ipv4->AddAddress (i, Ipv4InterfaceAddress (address, Ipv4Mask ("255.255.255.0")));
		ipv4->SetUp (i);

		NS_ASSERT_MSG(m_forwardDeviceMap.find(address) == m_forwardDeviceMap.end(), "this Virtual IP is already exits");
		m_forwardDeviceMap[address] = dev;//this set the forward device for packet forwarding
	}
}

Ptr<GmaVirtualInterface>
GmaProtocol::CreateGmaVirtualInterface (Time startTime)
{
	Ptr<GmaVirtualInterface> gmaVirtualInterface = CreateObject<GmaVirtualInterface> (startTime);
	gmaVirtualInterface->SetId(m_node, m_nodeId, m_gmaInterfaceId++);
	gmaVirtualInterface->SetForwardPacketCallBack (MakeCallback (&GmaProtocol::ForwardPacket, this));
	return gmaVirtualInterface;
}

Ptr<GmaVirtualInterface>
GmaProtocol::CreateGmaRxVirtualInterface (Time startTime)
{
	Ptr<GmaVirtualInterface> gmaVirtualInterface = CreateObject<GmaVirtualInterface> (startTime);
	gmaVirtualInterface->SetId(m_node, m_nodeId, m_gmaInterfaceId++);
	gmaVirtualInterface->SetTxMode(false);
	gmaVirtualInterface->SetRxMode(true);
	gmaVirtualInterface->SetForwardPacketCallBack (MakeCallback (&GmaProtocol::ForwardPacket, this));
	return gmaVirtualInterface;
}

Ptr<GmaVirtualInterface>
GmaProtocol::CreateGmaTxVirtualInterface (Time startTime)
{
	Ptr<GmaVirtualInterface> gmaVirtualInterface = CreateObject<GmaVirtualInterface> (startTime);
	gmaVirtualInterface->SetId(m_node, m_nodeId, m_gmaInterfaceId++);
	gmaVirtualInterface->SetTxMode(true);
	gmaVirtualInterface->SetRxMode(false);
	gmaVirtualInterface->SetForwardPacketCallBack (MakeCallback (&GmaProtocol::ForwardPacket, this));
	return gmaVirtualInterface;
}

void
GmaProtocol::AddRemoteVirIp (Ptr<GmaVirtualInterface> interface, const Ipv4Address& virIp, uint32_t nodeId)
{
	//create one socket per connected link
	m_gamVirtualInterfaceMap[virIp] = interface;
	m_gmaNodeIdToVirIpMap[nodeId] = virIp;
}

void
GmaProtocol::CreateSocket (uint8_t cid)
{
	//create one socket per connected link
	uint16_t port = START_PORT_NUM+cid;
	Ptr<Socket> socket = Socket::CreateSocket (m_node, TypeId::LookupByName ("ns3::UdpSocketFactory"));
	socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), port));
  	//socket->Connect (InetSocketAddress (phyAddr, port));
	socket->SetRecvCallback (MakeCallback (&GmaProtocol::VirtualRecv, this));
	m_socketMap[cid] = socket;
}

void
GmaProtocol::AddRemotePhyIp (const Ipv4Address& virIp, const Ipv4Address& phyAddr, uint8_t cid, int apId)
{
	//std::cout << this<< " remote VIR IP:" << virIp << " / PHY IP:" << phyAddr << "\n";
	if(m_socketMap.find(cid) == m_socketMap.end())
	{
		CreateSocket (cid);
	}
	NS_ASSERT_MSG(m_gamVirtualInterfaceMap.find(virIp) != m_gamVirtualInterfaceMap.end(),
	 "this virtual interface doesn't exist");
	m_gamVirtualInterfaceMap[virIp]->AddPhyLink(m_socketMap[cid], phyAddr, cid, apId);
	//delete the previous phy to virip map...

	//map new phy add to vir ip
	/*ipCidkey_t tempKey = std::make_pair(virIp, cid);

	if(m_virCidToPhyIpMap.find(tempKey) == m_virCidToPhyIpMap.end())
	{
		//no wifi ip mapped to this vir ip and cid
		m_virCidToPhyIpMap[tempKey] = phyAddr;
	}
	else
	{
		std::cout << "update the the PHY IP mapped to this vir ip and cid" << +cid <<"\n";
		m_phyToVirIpMap.erase(m_virCidToPhyIpMap[tempKey]);//remove the current wifi to virtual ip mapping.
		m_virCidToPhyIpMap[tempKey] = phyAddr;//this is will be the only Wi-Fi IP associated to the virtual IP.

		if(m_linkDownTime > MilliSeconds(0))
		{
			//enable link down time after ap switch
			m_IpToRxTime[phyAddr] = Now()+m_linkDownTime;//this will stop RX for m_linkDownTime interval
			m_gamVirtualInterfaceMap[virIp]->StopTxTemp(cid, m_linkDownTime);//virtual interface will stop transmitting packets to link cid for m_linkDownTime interval
		}
	}*/

	m_phyToVirIpMap[phyAddr] = virIp;
}

void
GmaProtocol::AddRemotePhyIpCandidate (const Ipv4Address& virIp, const Ipv4Address& phyAddr, const Mac48Address& macAddr, uint8_t cid, int apId)
{
	if(m_socketMap.find(cid) == m_socketMap.end())
	{
		CreateSocket (cid);
	}
	m_gamVirtualInterfaceMap[virIp]->AddPhyCandidate(m_socketMap[cid], phyAddr, macAddr, cid, apId);
    m_phyToVirIpMap[phyAddr] = virIp;
}

bool
GmaProtocol::VirtualSend (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
	Ipv4Header ipv4Header;
	packet->PeekHeader (ipv4Header);
	//std::cout << " IP header:" << ipv4Header << "\n";

	NS_ASSERT_MSG(m_gamVirtualInterfaceMap.find(ipv4Header.GetDestination()) != m_gamVirtualInterfaceMap.end(),
	 "this virtual interface doesn't exist");

	m_gamVirtualInterfaceMap[ipv4Header.GetDestination()]->Transmit(packet);
	return true;
}


void
GmaProtocol::VirtualRecv (Ptr<Socket> socket)
{
	Address from;//the sender Address(IP, port)
	Ptr<Packet> packet = socket->RecvFrom (65535, 0 ,from);
	InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (from);

	Ptr<Packet> pcopy = packet->Copy();
	GmaHeader gmaHeader;
	pcopy->RemoveHeader(gmaHeader);

	if(gmaHeader.GetFlags () == 256)//probe mgs need to set the clientID big
	{
		//probe msg, we can use it to update PHY address.
		MxControlHeader mxHeader;
		pcopy->RemoveHeader(mxHeader);
		//std::cout << " gma header:" << gmaHeader << " ----- mx header:" << mxHeader << "\n";

		if(mxHeader.GetType() == 1 )
		{
			m_phyToVirIpMap[iaddr.GetIpv4()] = m_gmaNodeIdToVirIpMap[gmaHeader.GetClientId()];
			if(mxHeader.GetProbeFlag() == 1)
			{
				NS_ASSERT_MSG(m_gmaNodeIdToVirIpMap.find(gmaHeader.GetClientId()) != m_gmaNodeIdToVirIpMap.end(), "cannot find in the map");
				AddRemotePhyIp(m_gmaNodeIdToVirIpMap[gmaHeader.GetClientId()], iaddr.GetIpv4(), mxHeader.GetConnectionId());
				//std::cout << "GMA protocol RX client:" << +gmaHeader.GetClientId() << " ProbeFlag:" << +mxHeader.GetProbeFlag() << ". Add PHY:" << iaddr.GetIpv4()
				//<< " to vir:" <<m_gmaNodeIdToVirIpMap[gmaHeader.GetClientId()] << " cid:" << +mxHeader.GetConnectionId() <<"!!!\n";

			}
			else
			{
				//std::cout << "GMA protocol RX client:" << +gmaHeader.GetClientId() << " ProbeFlag:" << +mxHeader.GetProbeFlag() << ". [TESTING] PHY:" << iaddr.GetIpv4()
				//<< " to vir:" <<m_gmaNodeIdToVirIpMap[gmaHeader.GetClientId()] << " cid:" << +mxHeader.GetConnectionId() <<"!!!\n";
			}

		}
	}
	else if(gmaHeader.GetFlags () == 0)//all other control msgs
	{
		MxControlHeader mxHeader;
		pcopy->RemoveHeader(mxHeader);
		//std::cout << " gma header:" << gmaHeader << " ----- mx header:" << mxHeader << "\n";
		if(mxHeader.GetType() == 8)//qos testing request
		{
			std::cout << "QoS Request Received at GMA Protocol Time:" << Now().GetSeconds() <<std::endl;
			//std::cout << "addr:" << iaddr.GetIpv4() << "port:" << iaddr.GetPort() << std::endl;
			//std::cout << "mxheader:" << mxHeader <<std::endl;
			if(mxHeader.GetChannelId() == UINT8_MAX)
			{
				//send this msg directly.
				if(m_phyToVirIpMap.find(iaddr.GetIpv4()) != m_phyToVirIpMap.end()){

					//check if this link is down...
					Ipv4Address virIpAdd = m_phyToVirIpMap[iaddr.GetIpv4()];
					NS_ASSERT_MSG(m_gamVirtualInterfaceMap.find(virIpAdd) != m_gamVirtualInterfaceMap.end(),
					"this virtual interface doesn't exist");

					m_gamVirtualInterfaceMap[virIpAdd]->Receive(packet, iaddr.GetIpv4(), iaddr.GetPort());


				}
				else
				{
					//NS_FATAL_ERROR("No PHY to VIR IP mapping (should not happen!!!!!!!!!), pkt drop!!");
					std::cout <<"No PHY to VIR IP mapping (Wait for Probe arrives to update PHY IP), pkt drop!!!\n";
					//no phy to vir ip mapping, pkt dropped. this happen because of Wi-Fi ap switch...
				}
				return;
			}
			auto iter = m_channelToQoSRequestInfo.find(mxHeader.GetChannelId());
			if (iter != m_channelToQoSRequestInfo.end() && iter->second->m_qosTestProhibitTimer.IsRunning())
			{
				//there is a qos testing active for this channel ID, we cannot start a new qos testing....add to the pending queue
				Ptr<QosRequestItem> item = Create<QosRequestItem>();
				item->m_mxHeader = mxHeader;
				item->m_packet = packet->Copy();
				item->m_addr = from;
				iter->second->m_qosRequestList.push_back(item);
				std::cout<< "QoS testing for channel :" << +mxHeader.GetChannelId() << " is running."
				<< " Add to Request List. Size: " << iter->second->m_qosRequestList.size() << std::endl;

				//send Ack for the Qos Request to prevent Ack timeout...
				if(m_phyToVirIpMap.find(iaddr.GetIpv4()) != m_phyToVirIpMap.end()){

					//check if this link is down...
					Ipv4Address virIpAdd = m_phyToVirIpMap[iaddr.GetIpv4()];
					NS_ASSERT_MSG(m_gamVirtualInterfaceMap.find(virIpAdd) != m_gamVirtualInterfaceMap.end(),
					"this virtual interface doesn't exist");

					m_gamVirtualInterfaceMap[virIpAdd]->RespondAck(mxHeader);
				}
				else
				{
					//NS_FATAL_ERROR("No PHY to VIR IP mapping (should not happen!!!!!!!!!), pkt drop!!");
					std::cout <<"No PHY to VIR IP mapping (Wait for Probe arrives to update PHY IP), pkt drop!!!\n";
					//no phy to vir ip mapping, pkt dropped. this happen because of Wi-Fi ap switch...
				}
				return;
			}
			else
			{
				if(iter == m_channelToQoSRequestInfo.end())
				{
					//we can start a new qos testing... start the qos testing prohibit timer to hold the new incoming requests
					Ptr<QoSRequestInfo> qosInfo = Create<QoSRequestInfo>();
					qosInfo->m_qosTestProhibitTimer = Simulator::Schedule(MilliSeconds(mxHeader.GetTestDuration()*100)+m_QosTestingGuardTime, &GmaProtocol::QosTestingTimerExpire, this, mxHeader.GetChannelId());
					m_channelToQoSRequestInfo[mxHeader.GetChannelId()] = qosInfo;
				}
				else
				{
					NS_FATAL_ERROR("we should delete this map when the event ends()!!!");
				}
			}

		}
	}
	//else data packets

	if(m_phyToVirIpMap.find(iaddr.GetIpv4()) != m_phyToVirIpMap.end()){

		//check if this link is down...
		Ipv4Address virIpAdd = m_phyToVirIpMap[iaddr.GetIpv4()];
		NS_ASSERT_MSG(m_gamVirtualInterfaceMap.find(virIpAdd) != m_gamVirtualInterfaceMap.end(),
		 "this virtual interface doesn't exist");

		m_gamVirtualInterfaceMap[virIpAdd]->Receive(packet, iaddr.GetIpv4(), iaddr.GetPort());


	}
	else
	{
		//NS_FATAL_ERROR("No PHY to VIR IP mapping (should not happen!!!!!!!!!), pkt drop!!");
		std::cout <<"No PHY to VIR IP mapping (Wait for Probe arrives to update PHY IP), pkt drop!!!\n";
		//no phy to vir ip mapping, pkt dropped. this happen because of Wi-Fi ap switch...
	}
}

void
GmaProtocol::ForwardPacket (Ptr<Packet> packet)
{
	Ipv4Header ipv4Header;
	packet->PeekHeader(ipv4Header);

	if(m_forwardDeviceMap.size() != 0)
	{
		//The GMA client is not the application client, forward it to the application client
		NS_ASSERT_MSG(m_forwardDeviceMap.find(ipv4Header.GetDestination()) != m_forwardDeviceMap.end(), "this Virtual IP is not exit");
		m_forwardDeviceMap[ipv4Header.GetDestination()]->Send (packet, 
			m_forwardDeviceMap[ipv4Header.GetDestination()]->GetBroadcast (), Ipv4L3Protocol::PROT_NUMBER);		
	}
	else
	{
		//receiver is the same device, fowrad up to application
		m_virtualDevice->Receive (packet, 0x0800, m_virtualDevice->GetAddress (), m_virtualDevice->GetAddress (), NetDevice::PACKET_HOST);
	}
}

void
GmaProtocol::QosTestingTimerExpire (uint8_t channelId)
{
	//std::cout << Now().GetSeconds() << "channel:" << +channelId << " timer expires!!" << std::endl;
	auto iter = m_channelToQoSRequestInfo.find(channelId);
	if (iter != m_channelToQoSRequestInfo.end())
	{
		if(iter->second->m_qosRequestList.size() == 0)
		{
			//already processed all qos request. no action.
			m_channelToQoSRequestInfo.erase(channelId);
			return;
		}

		//process the  first request in the list.
		Ptr<QosRequestItem> qosItem = iter->second->m_qosRequestList.front();
		iter->second->m_qosRequestList.pop_front();
		InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (qosItem->m_addr);
		//send the packet again!!!
		if(m_phyToVirIpMap.find(iaddr.GetIpv4()) != m_phyToVirIpMap.end()){

			//check if this link is down...
			Ipv4Address virIpAdd = m_phyToVirIpMap[iaddr.GetIpv4()];
			NS_ASSERT_MSG(m_gamVirtualInterfaceMap.find(virIpAdd) != m_gamVirtualInterfaceMap.end(),
			"this virtual interface doesn't exist");

			m_gamVirtualInterfaceMap[virIpAdd]->Receive(qosItem->m_packet, iaddr.GetIpv4(), iaddr.GetPort());

		}
		else
		{
			//NS_FATAL_ERROR("No PHY to VIR IP mapping (should not happen!!!!!!!!!), pkt drop!!");
			std::cout <<"No PHY to VIR IP mapping (Wait for Probe arrives to update PHY IP), pkt drop!!!\n";
			//no phy to vir ip mapping, pkt dropped. this happen because of Wi-Fi ap switch...
		}

		//start Qos Testing prohibit timer to indicate Active for this channel ID.
		iter->second->m_qosTestProhibitTimer = Simulator::Schedule(MilliSeconds(qosItem->m_mxHeader.GetTestDuration()*100)+m_QosTestingGuardTime, &GmaProtocol::QosTestingTimerExpire, this, channelId);
	
	}
	else
	{
		NS_FATAL_ERROR("the qos request info does not exits!!!");
	}
}


}


