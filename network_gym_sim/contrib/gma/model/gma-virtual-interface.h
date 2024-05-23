/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GMA_VIRTUAL_INTERFACE_H
#define GMA_VIRTUAL_INTERFACE_H

#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/virtual-net-device.h>
#include "gma-header.h"
#include "mx-control-header.h"
#include "qos-measurement-manager.h"
#include "gma-tx-control.h"
#include "link-state.h"
#include "phy-access-control.h"
#include <ns3/integer.h>
#include "ns3/gma-data-processor.h"
#include "ns3/wifi-module.h"

namespace ns3 {

const uint16_t START_PORT_NUM = 1000; //start port number, the port number is assigned as START_PORT_NUM + CID.

  //This class setup a tunnel between two virtual IP address, multiple physic links
  //can be used simultaneously to transmit over this tunnel
class GmaVirtualInterface : public Object
{
public:
	GmaVirtualInterface (Time startTime);
	virtual ~GmaVirtualInterface ();
  static TypeId GetTypeId (void);

  void SetId (Ptr<Node> node, uint32_t nodeId, uint16_t InterfaceId);

  void SetFixedTxCid(uint8_t cid);

  void SetTxMode (bool flag);
  void SetRxMode (bool flag);

  //add a physical link to send packet over this virtual tunnel
  void AddPhyLink (Ptr<Socket> socket, const Ipv4Address& phyAddr, uint8_t cid, int apId = 0);

  void AddPhyCandidate (Ptr<Socket> socket, const Ipv4Address& phyAddr,  const Mac48Address& macAddr, uint8_t cid, int apId = 0);

  //select one of the physic link to transmit this packet
  void Transmit (Ptr<Packet> packet);
  void Receive (Ptr<Packet> packet, const Ipv4Address& phyAddr, uint16_t fromPort);

  //if measure report enabled, measure packet statistics   and the deliver packets
  void MeasureAndForward(Ptr<Packet> packet, const GmaHeader& gmaHeader);

  //if reordering enabled, call this function to reorder packets.
  void InOrderDelivery (Ptr<Packet> packet, const GmaHeader& gmaHeader, uint8_t cid);

  //Set a callback that forward packets back to the gma for delivery
  void SetForwardPacketCallBack(Callback<void, Ptr<Packet> > cb);

  //process control messages.
  void RecvCtrlMsg (Ptr<Packet> packet, const Ipv4Address& phyAddr);

  //send TSU back to GMA transmitter.
  void SendTsu (Ptr<SplittingDecision> decision);
  void RcvReverseTsu (const MxControlHeader& header);//this TSU is from the sender (client device) to trigger a TSU at the receiver side.

  //configure duplicate mode, default false
  void SetDuplicateMode (bool flag);

  //powerRange 0 stands for low, 1 stands for high power
  void WifiPeriodicPowerTrace(uint8_t cid, uint8_t apId, double power);
  void WifiPowerTrace(uint8_t powerRange, uint8_t cid);
  void ConfigureTxAlgorithm(std::string algorithm, uint8_t splittingBurst);
  void ConfigureRxAlgorithm(std::string algorithm, uint8_t splittingBurst);
  void StopReordering (uint8_t reason);//reason = [0,1,2] = [inorder, end marker, timeout]
  void SendEndMarker (uint8_t cid);
  void SetWifiLowPowerThresh (double lowPower);
  void SetWifiHighPowerThresh (double highPower);
  void SetQosRequirement (uint8_t sliceId, uint64_t duration, uint64_t qos_delay, uint64_t tresh1, uint64_t thresh2, double delayTarget, double lossTarget);
  void SetGmaDataProcessor(Ptr<GmaDataProcessor> database);
  void EnableServerRole (uint32_t clientId);
  void EnableClientRole (uint32_t clientId);
  void RespondAck (const MxControlHeader& header);
  void MonitorSniffRx(std::string path, 
                Ptr<const Packet> packet,
                uint16_t channelFreqMhz,
                WifiTxVector txVector,
                MpduInfo aMpdu,
                SignalNoiseDbm signalNoise,
                uint16_t staId);
private:
  void QosTestingSessionEnd();
  void DiscardBackupLinkPackets (bool flag);
  void ReceiveDlSplitWeightAction (const json& action);

  void ReceiveWifiDlDfpAction (const json& action);
  void ReceiveLteDlDfpAction (const json& action);
  void ReceiveNrDlDfpAction (const json& action);
  
  static const int MAX_GMA_SN = 0x00FFFFFF; //max of 3 bytes gma sequence number
  static const int MAX_GMA_LSN = 0x000000FF; //max of 1 byte gma local sequence number

  //every PROBE_INTERVAL, we will send a burst of probes.
  const Time PROBE_INTERVAL = Seconds(1);
  const uint32_t PROBE_BURST_SIZE = 15;
  uint32_t m_probeCounter = 0;
  Time m_probeBurstGap = MilliSeconds(1);
  const Time INITIAL_CONTROL_RTO = PROBE_INTERVAL/2;

  //collect measurement results, and generate report
  void CollectMeasureResults();

  void MeasurementGuardIntervalEnd();

  void PeriodicProbeMsg ();//this function send probe messages to all links

  void QosTestingRequestMsg (uint8_t cid, uint8_t durationS, uint8_t direction);//this function prepare a qos testing request msg for the link with input cid

  void QosViolationMsg (uint8_t direction);//this function prepare a qos violation for this cid.

  void QosTestingStartNotifyMsg (const MxControlHeader& header);//this function prepare a qos testing start notify msg for the requested link

  void SendCtrlMsg (const MxControlHeader& header, bool retx = false);//retxAttemp indicate the number of retx, 0 stands for a new msg.

  void RetxCtrlMsgExpires(uint16_t csn); //check if the expired message needs to be retxed

  //if all queues are not empty, compare the sn of first packet and release the one with min SN
  void ReleaseMinSnPacket();

  //release in order packets in reordering qeueue
  void ReleaseInOrderPackets ();

  void ReleaseAllPackets();

  //reordering timeout, release all packets in reordering queue
  void ReorderingTimeout ();

  void UpdateReorderTimeout ();
  //compare the difference of two sequence number considering overflow
  int SnDiff(int x1, int x2); //sn
  int CsnDiff(int x1, int x2); //control sn
  int LsnDiff(int x1, int x2);  //local sn

  static const uint8_t TOS_AC_BE = 0x70; //three most significant bits 011
  static const uint8_t TOS_AC_BK = 0x28; //three most significant bits 001
  static const uint8_t TOS_AC_VI = 0xb8; //three most significant bits 101
  static const uint8_t TOS_AC_VO = 0xc0; //three most significant bits 110

  //LTE uses AC_VI for high priority, AC_BE for low priority
  //Wi-Fi uses AC_VI for high priority, AC_BE for low prioirty and AC_BK for QoS testing.

  //we classify data packet based on qos, control packets are all best effort.
  void SendByCid (uint8_t cid, Ptr<Packet> pkt, uint8_t tos); //we emulate queueing delay here, out of order packet might happen here.
  void SendByCidNow (uint8_t cid, Ptr<Packet> pkt, uint8_t tos);

  //struct queue item for reordering queue. Each packet is tagged with its gma header
  //and whether it is in order (use GSN diff and LSN diff comparison)
  struct RxQueueIterm : public SimpleRefCount<RxQueueIterm>
  {
    Ptr<Packet> m_packet;
    GmaHeader m_gmaHeader;
    bool m_inOrder = false;
    Time m_receivedTime;
  };

  // struct link parameters per physic link.
  struct LinkParams : public SimpleRefCount<LinkParams>
  {
    //Ptr<Socket> m_socket; //socket per link
    uint8_t m_gmaTxLocalSn = 0; //lsn per link
    std::queue< Ptr<RxQueueIterm> > m_reorderingQueue; //reordering queue per link
    uint8_t m_gmaRxLastLocalSn = 255; //LSN of last received packet from this link;
    uint32_t m_gmaRxLastSn = MAX_GMA_SN;//SN of last received packet from this link;
    //Ipv4Address m_ipAddr; //ip address per link
    //int m_apId; //ap id for wifi, we can even add lte?

    Ptr<PhyAccessControl> m_phyAccessContrl;
    double m_qosMarking = 0.0; //0 for false, 1 for true
    uint32_t m_emulateDelay = 0; //unit ms
  };

  uint32_t m_gmaTxSn = 0; //sender tx gma sn per flow

  uint32_t m_lastGsn = MAX_GMA_SN;

  uint16_t m_mxCtrSeqNumber = 0; //sender control gma sn per flow

  uint32_t m_gmaRxExpectedSn = 0; //receiver expected sn per flow.

  uint16_t m_maxTsuSeqNum = 0; // the max sn of TSU message.

  std::map < uint8_t, Ptr<LinkParams> > m_linkParamsMap; // a map of link parameters, the key is the cid of that link

  Callback<void, Ptr<Packet> > m_forwardPacketCallback; //callback that sends packet to GMA to transmit

  uint32_t m_totalQueueSize = 0; //number of packets in all of the reordering queues combined.

  //in andorid app, the value of timeout is configured use the 2*(MAX OWD of all links - MIN OWD of all links)!!!!
  //change it after we do the wifi offset measurement
  const Time MAX_REORDERING_TIMEOUT = MilliSeconds(1000);
  const Time MIN_REORDERING_TIMEOUT = MilliSeconds(10);
  Time m_reorderingTimeout = MAX_REORDERING_TIMEOUT;//initial value

  EventId m_reorderingTimeoutEvent;

  EventId m_stopReorderEvent;

  //tag send time and retx attempts with a control header. C-SN is the key to get this item
  struct TexedCtrQueueItem : public SimpleRefCount<TexedCtrQueueItem>
  {
    MxControlHeader m_mxControlHeader;
    std::map <uint16_t, Time> m_csnToTxtimeMap;
  };

  std::map<uint16_t, Ptr<TexedCtrQueueItem> > m_txedCtrQueue; //stores the contorl messages waiting for ACKS, key = control SN

  EventId m_periodicProbeEvent;

  Time m_ctrRto; // waiting time before a retx timeout

  const uint32_t RTO_SCALER = 3;
  const uint8_t MAX_RETX_ATTEMPT = 5;

  ObjectFactory m_measurementFactory;
  Ptr<MeasurementManager> m_measurementManager;

  uint8_t m_lastTsuNoneZeroLink = 0;
  bool m_enableReordering = false;
  bool m_useLsnReordering = false;
  bool m_receiveDuplicateFlow = false;

  Ptr<GmaRxControl> m_gmaRxControl;
  Ptr<GmaTxControl> m_gmaTxControl;

  bool m_txMode = false;
  bool m_rxMode = false;

  uint16_t m_maxTsaSn = 0;

  bool m_enableMeasureReport = false;
  bool m_saveToFile = true;

  bool m_fileTile = false;
  uint64_t m_receivedBytes = 0;
  uint64_t m_reorderingTimeoutCounter = 0;
  uint64_t m_tsuCounter = 0;

  struct MeasureParam : public SimpleRefCount<MeasureParam>
  {
    uint64_t m_rcvBytes = 0;;
    uint32_t m_minOwd = UINT32_MAX;
    uint32_t m_maxOwd = 0;
    uint64_t m_owdSum = 0;
    uint64_t m_count = 0;
    uint64_t m_numOfHighDelayPkt = 0;
    uint64_t m_numofT1DelayPkt = 0;
    uint64_t m_numofT2DelayPkt = 0;
    uint64_t m_numOfInOrderPacketsForReport = 0;
    uint64_t m_numOfMissingPacketsForReport = 0;
    uint64_t m_numOfAbnormalPacketsForReport = 0;
  };

  struct MeasureSn : public SimpleRefCount<MeasureSn>
  {
    uint8_t m_lastLsn = MAX_GMA_LSN;
    uint64_t m_lastGsn = MAX_GMA_SN;
  };


  std::map < uint8_t, Ptr<MeasureParam> > m_measureParamPerCidMap;
  std::map < uint8_t, Ptr<MeasureSn> > m_measureSnPerCidMap;


  uint16_t m_gmaInterfaceId = 0;
  uint32_t m_nodeId = 0;
  uint32_t m_clientId = UINT32_MAX;//use it to identify client... we will pass imsi.

  Ptr<MeasureParam> m_flowParam;

  uint64_t m_numOfTxPacketsPerFlow = 0;
  uint64_t m_TxBytesPerFlow = 0;
  uint64_t m_numOfInOrderPacketsPerFlow = 0;
  uint64_t m_numOfMissingPacketsPerFlow = 0;
  uint64_t m_numOfAbnormalPacketsPerFlow = 0;
  uint64_t m_acceptableDelay = 1000000;
  uint64_t m_delaythresh1 = 1000000;
  uint64_t m_delaythresh2 = 1000000;
  uint8_t m_sliceId = 0; // right now we only support one slice per interface, therefore, gma control msg will use the same slice.


  bool m_duplicateMode = false;
  Ptr<Node> m_node;
  double m_lastLtePower = 0;
  bool m_wifiPowerAvailable = false;
  bool m_ltePowerAvailable = false;

  uint16_t m_maxLinkMapUpdateSn = 0; // the max sn of message that updates link up down.
  Ptr<LinkState> m_linkState;
  bool m_enableMarkCtrlLinkMap = false;//disable will makr all links as 1. enable only if cross-layer power trace is enabled...

  //steer mode reodering is triggered by packet arrived from different connection
  //Reordering end if 1> the packet from the new link is inorder; or 2> the end marker arrives; or 3> time expires..
  uint8_t m_newLinkCid = UINT8_MAX; //UINT8_MAX stands for the current link is terminated, e.g., by end marker.
  uint8_t m_wifiPowerRange = 1;
  double m_wifiLowPowerThreshDbm = 0.0;
  double m_wifiHighPowerThreshDbm = 1.0;

  bool m_simulateSingleRadio = true;
  Time m_simulateLinkDownTime;

  Ptr<GmaDataProcessor> m_gmaDataProcessor;
  Time m_measurementInterval;
  Time m_measurementGuardInterval;

  Ptr<SplittingDecision> m_lastDecision;
  uint32_t m_missedAction = 0;

  static const uint8_t BEST_EFFORT_FLOW_ID = 0;
  static const uint8_t DUPLICATE_FLOW_ID = 3;
  static const uint8_t QOS_FLOW_ID = 4;

  bool m_clientRoleEnabled = false;
  bool m_serverRoleEnabled = false;

  bool m_discardBackupLinkPackets = false;//flag to drop packets over the backup link during testing

  bool m_qosClientTestingActive = false; //we only support qos testing for one session at a time for client, i.e., one cid and one direction.

  bool m_qosTestingAllowed = false; //whether to use the GMA based qos testing. Start QoS testing aftert reveive the first empty action.

  bool m_qosSteerActionReceived = false;
  Time m_nextReorderingUpdateTime = Seconds(0.0);
};

}
#endif /* GMA_VIRTUAL_INTERFACE_H */

