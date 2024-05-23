/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GMA_PROTOCOL_H
#define GMA_PROTOCOL_H

#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/virtual-net-device.h>
#include "gma-header.h"
#include "mx-control-header.h"
#include "gma-virtual-interface.h"

#include <ns3/integer.h>

namespace ns3 {

//this is the most basic protocol controls send and receive GMA (control and data) packets
class GmaProtocol : public Object
{
public:

  GmaProtocol ();
  virtual ~GmaProtocol ();
  static TypeId GetTypeId (void);
	//install GMA protocol to this node, only one gma protocol per node.
  GmaProtocol (Ptr<Node> node);

  //configure the virtual IP addresss for the local device
  //the input dev is the forward interface in case the packet needs to be forwarded to other node, it == 0 if the receive device is the GMA receiver, e.g., the user GMA
  void AddLocalVirtualInterface (const Ipv4Address& address, Ptr<NetDevice> dev = 0);

  //Create one virtual interface per remote device. 
  //For example, for a 1 server 2 user case. Server GMA need to create 2 virtual interface. Client GMA creats 1 virtual interface.
  Ptr<GmaVirtualInterface> CreateGmaVirtualInterface (Time startTime);

  //same as CreateGmaVirtualInterface function, but this interface only do receive related control functions, e.g., measurements and generate TSU feedback.
  Ptr<GmaVirtualInterface> CreateGmaRxVirtualInterface (Time startTime);

  //same as CreateGmaVirtualInterface function, but this interface only do transmit related control functions, e.g., control split ratio
  Ptr<GmaVirtualInterface> CreateGmaTxVirtualInterface (Time startTime);

  //Add one virtual link IP for this interface
  void AddRemoteVirIp (Ptr<GmaVirtualInterface> interface, const Ipv4Address& virIp, uint32_t nodeId = 0);
  
  //Add one physical link IP, CID for this interface
  void AddRemotePhyIp (const Ipv4Address& virIp, const Ipv4Address& phyAddr, uint8_t cid, int apId = 255);

  void AddRemotePhyIpCandidate (const Ipv4Address& virIp, const Ipv4Address& phyAddr, const Mac48Address& macAddr, uint8_t cid, int apId);

  //A packet is received at gma virtual interface,
  //it will be forward to real receive device.
  void ForwardPacket (Ptr<Packet> packet);
  
private:

  void QosTestingTimerExpire (uint8_t channelId);
  //key is the primary destination PHY IP, return value is the interface for that virtual IP.
  std::map < Ipv4Address, Ptr<GmaVirtualInterface> > m_gamVirtualInterfaceMap;

  //key is the PHY IP, the return value is the Virtual IP
  //only have two keys, one for LTE phy IP, one for WiFi phy IP.
  std::map < Ipv4Address, Ipv4Address > m_phyToVirIpMap;

  typedef std::pair<Ipv4Address, uint8_t> ipCidkey_t; //the first one is the virtual ip, the second one is cid.

 std::map < ipCidkey_t, Ipv4Address > m_virCidToPhyIpMap;//for each <virtual ip, cid> pair, there should be only one phy IP.
  //key is the cid, return value is the socket for that cid
  std::map < uint8_t, Ptr<Socket> > m_socketMap; 

  //overite the socket send function
  bool VirtualSend (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);
  void VirtualRecv (Ptr<Socket> socket);
  void CreateSocket (uint8_t cid);

  Ptr<Node> m_node;//the node that installs this GMA protocl

  Ptr<VirtualNetDevice> m_virtualDevice; //the device handles virtual IP packet

  std::map < Ipv4Address, Ptr<NetDevice> > m_forwardDeviceMap; //device that forward packets to real application client

  uint16_t m_gmaInterfaceId = 0; //max of 65536 interfaces.
  uint32_t m_nodeId;
  std::map<uint32_t, Ipv4Address> m_gmaNodeIdToVirIpMap;

  struct QosRequestItem : public SimpleRefCount<QosRequestItem>
  {
    MxControlHeader m_mxHeader;
    Ptr<Packet> m_packet;
    Address m_addr;
  };

  struct QoSRequestInfo : public SimpleRefCount<QoSRequestInfo>
  {
    EventId m_qosTestProhibitTimer; //is running if there is a qos testing active
    std::list< Ptr<QosRequestItem>> m_qosRequestList;//pending qos testing request
  };

  std::map<uint8_t, Ptr<QoSRequestInfo> > m_channelToQoSRequestInfo;

  Time m_QosTestingGuardTime = Seconds(0.1);//guard time between 2 QoS testing
};


}

#endif /* GMA_PROTOCOL_H */

