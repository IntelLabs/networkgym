/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef MEASUREMENT_MANAGER_H
#define MEASUREMENT_MANAGER_H

#include <ns3/object.h>
#include <ns3/log.h>
#include <ns3/simulator.h>
#include "gma-rx-control.h"

namespace ns3 {

class MeasureDevice : public Object
{
public:
  MeasureDevice ();
  virtual ~MeasureDevice ();
  static TypeId GetTypeId (void);

  uint8_t m_lastLsn = 255;

  Time m_lastRttUpdateTime = Now();
  uint32_t m_rtt = 100; //ms
  uint32_t m_owdSamePktOfRtt = 100; //owd measured the same pkt of m_rtt.

  uint32_t m_numOfInOrderPacketsPerCycle = 0;
  uint32_t m_numOfMissingPacketsPerCycle = 0;
  uint32_t m_numOfAbnormalPacketsPerCycle = 0;

  uint32_t m_numOfDataPacketsPerInterval = 0;

  //initial to infinity (the max value) in case no packet is received, unit ms
  uint32_t m_lastPacketOwd = UINT32_MAX; // unit ms
  uint64_t m_sumOwdPerInterval = 0; // sum of (OWD per packet), unit ms
  uint32_t m_numOfOwdPacketsPerInterval = 0; // number of packets for owd measurement
  uint32_t m_numOfHighOwdPacketsPerInterval = 0; //number of packets with high owd
  uint32_t m_lastIntervalOwd = UINT32_MAX; // average OWD (ms) from last measure interval, initial to infinity
  double m_lastIntervalOwdDiff = 1.0; // Diff(k) in the last measure interval
  uint8_t m_cid = 0;
  uint32_t m_lastMinOwd = UINT32_MAX;
  uint32_t m_currentMinOwd= UINT32_MAX;

  uint32_t m_lastMaxOwd = 0;
  uint32_t m_currentMaxOwd = 0;
  uint32_t m_numOfDelayViolationDataPacketsPerInterval = 0; //the number of data packets violate queueing delay, e.g., owd - min_owd.
  int m_senderOwdAdjustment = 0; //owd offset adjustment at the sender, notified in the tsa msg.
  MeasureDevice (uint8_t cid);
  MeasureDevice (uint8_t cid, uint32_t owdTarget, uint32_t queueingDelayTargetMs);

  uint8_t GetCid ();

  void InitialMeasurementCycle ();

  //menglei: update OWD of last ACK or data packet
  void UpdateLastPacketOwd (uint32_t timestampMs, bool dataFlag);
  void UpdateRtt (uint32_t rtt, uint32_t owd);
  uint32_t GetRtt ();

  void UpdateLsn (uint8_t lastLsn);
  int LsnDiff(int x1, int x2);

protected:

private:
  uint32_t m_owdTargetMs = 100;
  uint32_t m_queueingDelayTargetMs = 10;

};



//this is the most basic protocol controls send and receive GMA (control and data) packets
class MeasurementManager : public Object
{
public:

  MeasurementManager ();
  virtual ~MeasurementManager ();
  static TypeId GetTypeId (void);

  void AddDevice (uint8_t cid);
  void AddDevice (uint8_t cid, uint32_t owdTarget);

  Ptr<MeasureDevice> GetDevice (uint8_t cid);

  bool IsMeasurementOn();
  void DisableMeasurement();
  virtual void DataMeasurementSample(uint32_t owdMs, uint8_t lsn, uint8_t cid);

  void UpdateRtt(uint32_t rtt, uint32_t owd, uint8_t cid);

  virtual void UpdateOwdFromProbe(uint32_t owdMs, uint8_t cid);
  virtual void UpdateOwdFromAck(uint32_t owdMs, uint8_t cid);

  void UpdateAllMinOwd();//this should be called to update all min owd, which happens periodically.
  void UpdateReducedMinOwd(); //this should be called to update the min owd if current min owd is much smaller than previous interval, e.g., 10ms

  void UpdateMinOwdCheck(); //check if one way delay needs to be updated

  uint32_t GetMaxRttMs();

  uint32_t GetMaxOwdMs();
  uint32_t GetMinOwdMs();

  //menglei: TSA triggers a measureCycle start.
  void MeasureCycleStartByTsa(uint32_t measureStartSn, uint8_t delay, std::vector<uint8_t> owdVector); //Measurement cycle start triggered by TSA
  void MeasureCycleStart(uint32_t measureStartSn); //restart measurement due to no update needed, no tsu.


  //menglei: check if measurement should start
  virtual void MeasureIntervalStartCheck(Time t, uint32_t sn, uint32_t expectedSn);

   //menglei: start a measurement interval
  void MeasureIntervalStart(Time t);
  //check if this interval should end after receiving a packet
  virtual void MeasureIntervalEndCheck(Time t);
   //menglei: a measurement interval end, collect measurement results
  void MeasureIntervalEnd (Time t);

  void SetSendTsuCallback(Callback<void, Ptr<SplittingDecision> > cb);
  void SetRxControlApp (Ptr<GmaRxControl> control);
  int SnDiff(int x1, int x2);

protected:
  std::vector < Ptr<MeasureDevice> > m_deviceList;
  uint8_t m_measureIntervalIndex = 0; // current measure interval index
  bool m_measureIntervalStarted = false;
  Time m_measureIntervalStartTime;
  Time m_measureIntervalThresh;
  bool m_measurementOn = true; //true stands for a measurement cycle is started
  Ptr<GmaRxControl> m_rxControl;
  Callback<void, Ptr<SplittingDecision> > m_sendTsuCallback; //callback that sends packet to GMA to transmit
  uint32_t m_lastIntervalStartSn = 0;
  bool m_senderSideOwdAdjustment = true; //enable this will report the raw owd to the rx controller, but will send the min owd measurement to server and delay packets accordingly.
private:

  //menglei: parameters for one way delay measurement
  //all delay unit is ms
  uint32_t m_measureStartSn = 0;
  const double OWD_CONVERGE_THRESHOLD = 0.1;
  uint8_t m_maxMeasureInterval = 10;
  const Time OWD_OFFSET_UPDATE_INTERVAL = Seconds(12.0); //slightly larger than BBR's probe RTT interval, 10s

  const Time MAX_INTERVAL_DURATION = Seconds(1.0);
  const Time MIN_INTERVAL_DURATION = MilliSeconds(10);

  static const int MAX_GMA_SN = 0x00FFFFFF; //max of 3 bytes gma sequence number
  std::vector<uint32_t> m_previousOwdValue;
  uint8_t m_delayMeasurement = 0;
  EventId m_delayMeasurementEvent;
  Time m_lastMinOwdUpdateTime = Seconds(0.0);
  uint32_t m_splittingBurstRequirementEst = 0;
};

}

#endif /* MEASUREMENT_MANAGER_H */

