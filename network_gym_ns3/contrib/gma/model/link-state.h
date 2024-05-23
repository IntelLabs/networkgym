/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef LINK_STATE_H
#define LINK_STATE_H

#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/virtual-net-device.h>
#include "mx-control-header.h"
#include <ns3/integer.h>

namespace ns3 {

const int NETWORK_CID = -1; //CID for all network
const int WIFI_CID = 0; //CID for Wi-Fi.
const int CELLULAR_NR_CID = 2; //CID for 5G NR.
const int CELLULAR_LTE_CID = 11; //CID for 4G LTE. //by default, larger cid -> higher priority

class LinkState : public Object
{
public:
	LinkState ();
	virtual ~LinkState ();
  static TypeId GetTypeId (void);
  void SetFixedDefaultCid(uint8_t cid);

  bool IsLinkUp(uint8_t cid);
  void SetId(uint32_t id);
  void NoDataDown(uint8_t cid); // if the splitting ratio > 0, but receives 0 packets, markt it as no data down..
  bool CtrlMsgDown(uint8_t cid);//link down due to ctrl msg failure, return true if default cid updates
  bool CtrlMsgUp(uint8_t cid);//link up due to ctrl msg received, return true if default cid updates

  bool LowRssiDown(uint8_t cid);//link down due to low RSSI, return true if default cid updates
  bool HighRssiUp(uint8_t cid);//link up due to high RSSI, return true if default cid updates

  bool LinkBitMapDown(uint8_t cid);//link down due to link bit map indicates down, return true if default cid updates
  bool LinkBitMapUp(uint8_t cid);//link down due to link bit map indicates down, return true if default cid updates

  bool UpdateDefaultLink();

  void AddLinkCid (uint8_t cid);
  std::vector<uint8_t> GetCidList();
  uint8_t GetHighPriorityLinkCid ();
  uint8_t GetLowPriorityLinkCid ();

  uint8_t GetBackupLinkCid ();
  std::map<uint8_t, Time> m_cidToLastTestFailTime; //qos testing fail timestamp
  std::map<uint8_t, Time> m_cidToValidTestExpireTime; //when a valid testing expires..
  uint8_t m_qosTestDurationUnit100ms = 100;//duration for qos testing
  const Time MIN_QOS_TESTING_INTERVAL = Seconds (5); //min time between 2 failed QoS testing request.
  const Time MAX_QOS_VALID_INTERVAL = Seconds (5); //for idle flow, we assume the link still meet the qos requirement within this interval.
  std::map<uint8_t, uint8_t> m_cidToIndexMap;//get the index from cid
  //convert format from int to string
  static std::string ConvertCidFormat(int cid);
  //convert format from string to int
  static int ConvertCidFormat(std::string cidStr);
  void UpdateLinkQueueingDelay (std::vector<uint8_t> queueingDelayVector); //if a link has high queueing delay, we skip this link for a short period to drain the queue.
  bool IsLinkLowQueueingDelay (uint8_t cid);
  void StopLinkSkipping (); //if all up links have high queueing delay, stop skipping links.
private:
  uint8_t m_highPriorityLinkCid = CELLULAR_LTE_CID;
  uint8_t m_lowPriorityLinkCid = WIFI_CID;
  std::map<uint8_t, bool> m_ctrFailedLinkMap;  //input cid, return true means link is failed
  std::map<uint8_t, bool> m_lowQualityLinkMap;  //input cid, return true means link quality (signal strength) is low
  std::map<uint8_t, bool> m_bitmapFailedLinkMap;// input cid, return true means link is set down by TSU, we only alow one side to set link map in the TSU, and the other side the read from it
  std::map<uint8_t, uint32_t> m_skipCidMap; //the key is the cid, the value is when to start sending packet again. if current time < the map value, skip this link.
  uint32_t m_nodeId = 0;
  bool m_fixDefaultLink = false;
  std::vector<uint8_t> m_cidList; //the list of cid for connected physic links
  bool m_preferLargeCidValue = false; //if true: large the cid -> high priority.
};

}

#endif /* LINK_STATE_H */

