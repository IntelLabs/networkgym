/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "qos-measurement-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QosMeasurementManager");

NS_OBJECT_ENSURE_REGISTERED (QosMeasurementManager);

//meausrement manager

QosMeasurementManager::QosMeasurementManager ()
{
	NS_LOG_FUNCTION (this);
}

TypeId
QosMeasurementManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QosMeasurementManager")
    .SetParent<Object> ()
    .SetGroupName("Gma")
	.AddConstructor<QosMeasurementManager> ()
  ;
  return tid;
}

QosMeasurementManager::~QosMeasurementManager ()
{
	NS_LOG_FUNCTION (this);
}

void QosMeasurementManager::MeasureIntervalStartCheck(Time t, uint32_t sn, uint32_t expectedSn)
{
  return;
}

void
QosMeasurementManager::DataMeasurementSample(uint32_t owdMs, uint8_t lsn, uint8_t cid)
{
  return;
}

void
QosMeasurementManager::MeasureIntervalEndCheck(Time t)
{ 
  return;
}

bool
QosMeasurementManager::QosMeasurementStart (uint8_t cid, uint8_t duration)
{
  return false;

}

void
QosMeasurementManager::QosMeasurementStop (uint8_t cid, uint8_t duration)
{
  return;

}

void
QosMeasurementManager::QuitQosMeasurement()
{
   return;
}

}


