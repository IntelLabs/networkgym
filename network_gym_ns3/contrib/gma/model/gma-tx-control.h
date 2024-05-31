/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GMA_TX_CONTROL_H
#define GMA_TX_CONTROL_H

#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/virtual-net-device.h>
#include "mx-control-header.h"
#include <ns3/integer.h>
#include "link-state.h"
namespace ns3 {


class GmaTxControl : public Object
{
public:
	GmaTxControl ();

	virtual ~GmaTxControl ();
  static TypeId GetTypeId (void);

  void SetLinkState(Ptr<LinkState> linkstate);
  void SetAlgorithm(std::string algorithm);
  /*
    configure the link selection method
  */
  //void SetAlgorithm (std::string algorithm);

  uint8_t GetDeliveryLinkCid ();
  uint8_t GetNextCidFromTsu (); //this method get links by checking the cid of tsu msg.
  uint8_t GetSplittingBurstFromTsu (); //return 0 if no tsu is received.

  std::vector<uint8_t> GetDuplicateLinkCidList ();
  void StartQosTesting (uint8_t cid, uint8_t testDuration);
  void StopQosTesting (uint8_t cid);
  bool QoSTestingRequestCheck (uint8_t cid);
  /*rx side algorithm*/
  void RcvCtrlMsg (const MxControlHeader& header);
  void SetQosSteerEnabled (bool flag);
  bool QosSteerEnabled ();
  enum GmaTxAlgorithm
  {
    TxSide = 0,     //traffic splitting control based on infomation only from TX side, not implemented yet
    RxSide = 1,      //traffic splitting control based on infomation only from RX side (TSU).
    FixedHighPriority = 2, //stay on high priority link.
  };

private:

  uint8_t RxSideAlgorithm ();

  enum GmaTxAlgorithm m_algorithm = GmaTxAlgorithm::RxSide;

  std::vector<uint8_t> m_cidVector;
  std::vector<uint8_t> m_cidDupVector; //the cid list for duplicated packets, only used in steer mode.

  uint8_t m_paramL = 0;
  uint8_t m_splittingIndex;
  Ptr<LinkState> m_linkState;
  bool m_qosSteerEnabled = false;

};

}

#endif /* GMA_TX_CONTROL_APP_H */

