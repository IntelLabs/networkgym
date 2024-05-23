/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GMA_RX_CONTROL_H
#define GMA_RX_CONTROL_H

#include <ns3/network-module.h>
#include <ns3/internet-module.h>
#include <ns3/virtual-net-device.h>
#include "mx-control-header.h"
#include <ns3/integer.h>
#include "link-state.h"

namespace ns3 {

struct RxMeasurement : public SimpleRefCount<RxMeasurement>
{
  uint8_t m_links; // total number of active links
  double m_measureIntervalThreshS; //T1 threshold for measurement interval.
  double m_measureIntervalDurationS; //actual time spent in a measurement interval.
  std::vector<uint8_t> m_cidList; //connection ID
  std::vector<bool> m_delayThisInterval; // if true, owd is measured in this interval. If false, the owd may come from older intervals.
  std::vector<double> m_delayList; //delay in ms
  std::vector<double> m_minOwdLongTerm; //min one-way delay in ms measured in a long term.
  std::vector<double> m_lossRateList; //loss rate = missing_packet_num/received_packet_num
  std::vector<double> m_highDelayRatioList; //(only for QOS measurement)high delay ratio = high_delay_pkt_num/received_packet_num
  std::vector<uint32_t> m_delayViolationPktNumList; //packet number that violates queueing delay target.
  std::vector<uint32_t> m_totalPktNumList; //the total pkt number in this interval.
  uint32_t m_splittingBurstRequirementEst; //the reference value for splitting burst requirement.
};

struct RlAction : public SimpleRefCount<RlAction>
{
  uint8_t m_links; // total number of active links
  std::vector<uint8_t> m_cidList; //connection ID
  std::vector<uint8_t> m_ratio; //split ratio coming from RL
};

struct SplittingDecision : public SimpleRefCount<SplittingDecision>
{
  bool m_update = false;
  bool m_reverse = false;//true if this decision is for the reverse link!
  std::vector<uint8_t> m_splitIndexList;// <K(1), K(2),..., K(N)>, where K(1) is the number of packets scheduled for link i every K(1)+K(2) + ... + K(N) packets
  std::vector<uint8_t> m_minOwdLongTerm; //"normalized" min owd measurement. We compute it as max(min_owd(i)) - min_owd(i), where i is the i-th link.
  std::vector<uint8_t> m_queueingDelay; //average delay - min owd

};

class GmaRxControl : public Object
{
public:
	GmaRxControl ();

	virtual ~GmaRxControl ();
  static TypeId GetTypeId (void);
  void SetLinkState(Ptr<LinkState> linkstate);
  void AddLinkCid (uint8_t cid);

  /*
    configure the link selection method
  */
  void SetAlgorithm (std::string algorithm);

  Ptr<SplittingDecision> GetTrafficSplittingDecision (Ptr<RxMeasurement> measurement);
  Ptr<SplittingDecision> GetTrafficSplittingDecision (Ptr<RlAction> action);

  Ptr<SplittingDecision> LinkDownTsu (uint8_t cid);//only link down send tsu, link up should wait for delay algorithm to send tsu

  void SetSplittingBurst(uint8_t splittingBurst);

  uint8_t GetSplittingBurst(void);

  void SetDelayThresh(double offset);

  bool QosSteerEnabled ();
  bool QosFlowPrioritizationEnabled ();

  //DL case
  bool QoSTestingRequestCheck (uint8_t cid);
  bool QoSValidCheck (uint8_t cid);//the link is still have valid qos testing, send TSU right way.

  Ptr<SplittingDecision> GenerateTrafficSplittingDecision (uint8_t cid, bool reverse = false);//steer traffic to the cid

  enum GmaRxAlgorithm
  {
    Delay = 0,          // select link with lower delay
    CongDelay = 1,      // if the primary link is congested, select link with lower delay
    FixedHighPriority = 2,      // select the default link
    RlSplit = 3,      // use reinforcement learning for action.
    QosSteer = 4,  // use qos steering mode.
    gma = 5,          // select link with lower delay, same as Delay.
  };
  const double PKT_NUM_WEIGHT = 0.8; //the estimated pkt n = last interval pkt * PKT_NUM_WEIGHT. In the future, we need prediction.
  double m_qosDelayViolationTarget = 1.0;
  double m_qosLossTarget = 1.0;
  int GetMinSplittingBurst ();//for adaptive splitting burst
  int GetMaxSplittingBurst ();//for adaptive splitting burst
  int GetMeasurementBurstRequirement ();
  uint32_t GetQueueingDelayTargetMs();
  void SetQosTarget(double delayTarget, double lossTarge);

private:

  Ptr<SplittingDecision> CongDelayAlgorithm (Ptr<RxMeasurement> measurement);

  Ptr<SplittingDecision> DelayAlgorithm (Ptr<RxMeasurement> measurement, bool useMinOwd = false);
  Ptr<SplittingDecision> DelayViolationAlgorithm (Ptr<RxMeasurement> measurement);

  Ptr<SplittingDecision> QosSteerAlgorithm (Ptr<RxMeasurement> measurement);

  //Ptr<RxCtrlParams> DelayAndCongestionAlgorithm (Ptr<RxMeasurement> measurement);


  enum GmaRxAlgorithm m_algorithm = GmaRxAlgorithm::Delay;
  uint8_t m_splittingBurst = 32;
  std::vector<uint8_t> m_lastSplittingIndexList;
  std::vector<double> m_lastRatio;
  std::vector<uint8_t> m_decreaseCounter;//step size from 1, 2, 3, 4, 5... it is controlled by the link with max delay!!
  std::vector<int> m_lastOwd; //the owd of last decision

  /*for delay algorithm*/
  double m_delayThresh = 1;

  const double CONGESTION_LOSS_THRESHOLD = 0.0001;

  const double LOSS_ALGORITHM_BOUND = 2;
  int m_splittingBurstScalar = 2;

  bool m_enableAdaptiveStep = false;
  const int STEP_THRESH  = 4; // increase step if keep decreasing more than STEP_THRESH time
  bool m_enableStableAlgorithm = true;
  bool m_enableLossAlgorithm = false;
  Ptr<LinkState> m_linkState;

  bool m_enableQosFlowPrioritization = false;
  /*for delay and congestion algorithm*/
  /*uint8_t m_congestCounter;
  double m_minDelay; //min delay of the primary link
  uint16_t m_minDelayCounter;
  const double DELAY_THRESH = 5; //threshold to detect congestion, uint ms
  const double CONGEST_INTERVAL = 0; //the number of interval to stay in congestion state after detect congestion*/
  uint32_t m_queueingDelayTargetMs = 10;
  double CONGESTION_SCALER_MAX = 0.5;
  double CONGESTION_SCALER_MIN = 0.1;
  double m_congestionScaler = 0.5; //the scaler a. we use it when the bandwidth estimation is not available..

  bool m_adaptiveCongestionScaler = true; //if enabled, the congestion scaler will be computed dynamically and within the range of [CONGESTION_SCALER_MIN, m_congestionScaler].
  double m_relocateScaler = 0.1; //when there is no congestion, we move some packets from low traffic link to high traffic link. Every time we can move up to m_relocateScaler*total_packets.
  int minSplitAdjustmentStep = 1; //we compute m_relocateScaler = minSplitAdjustmentStep/m_splittingBurst;
  bool m_enableBwEstimate = true;
  uint32_t m_bwHistSize = 10; //track the n(i) for the past 10 intervals.
  std::map< uint8_t, std::vector<double> > m_cidToBwHistMap; // the key is the cid of the link and the value is the history of past bandwidth (packets/s) n(i)/interval_duration measurement.
  std::map< uint8_t, std::vector<uint32_t> > m_cidToKiHistMap; // the key is the cid of the link and the value is the history of past k(i), make sure it is the same size as m_cidToBwHistMap.

  bool m_flowActive = false; //true if a flow is actively sending data
  double m_updateThreshold; //if the traffic splitting ratio update is smaller than this threshold, we do not send tsu.
  bool m_adaptiveSplittingBurst = false; //set the splitting burst to 0 will enable adaptive splitting burst.
  double m_roundingScaler = -0.3; //set to a negative value will reduce the splitting ratio s(i) more aggressively. For example, with m_roundingScaler = -0.3, the int value of s(i) = 2.7 will be round to round(2.7-0.3) = 2.
};


}

#endif /* GMA_RX_CONTROL_H */

