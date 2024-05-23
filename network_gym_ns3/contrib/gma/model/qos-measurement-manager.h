/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef QOS_MEASUREMENT_MANAGER_H
#define QOS_MEASUREMENT_MANAGER_H

#include "measurement-manager.h"

namespace ns3 {

//this is the most basic protocol controls send and receive GMA (control and data) packets
class QosMeasurementManager : public MeasurementManager
{
public:

  QosMeasurementManager ();
  virtual ~QosMeasurementManager ();
  static TypeId GetTypeId (void);
  void MeasureIntervalStartCheck(Time t, uint32_t sn, uint32_t expectedSn);//this function do nothing in qos measurement
  void DataMeasurementSample(uint32_t owdMs, uint8_t lsn, uint8_t cid);
  void MeasureIntervalEndCheck(Time t);//we use it for early violation detection

  //qos testing -> duration is not zero, we will save this value to m_qosDuration
  //qos monitoring -> during is set to zero, we will load the value from m_qosDuration
  bool QosMeasurementStart (uint8_t cid, uint8_t duration);//return false if there is error, e.g., measurement is already running
  void QosMeasurementStop (uint8_t cid, uint8_t duration);
  void QuitQosMeasurement ();
private:

  uint8_t m_qosDuration = 0; //this param will be overwrite by the qos testing request msg
  std::map<uint8_t, EventId> m_cidToQosMeasurementMap;//key is the cid
  std::map<uint8_t, uint64_t> m_cidToLastPacketNMap; //key is the cid, return value is the number of pkt in the last measurement.
};

}

#endif /* QOS_MEASUREMENT_MANAGER_H */

