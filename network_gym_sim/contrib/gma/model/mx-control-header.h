/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef MX_CONTROL_HEADER_H
#define MX_CONTROL_HEADER_H

#include "ns3/header.h"
#include "ns3/log.h"

namespace ns3 {


static const uint16_t MX_CONTROL_HEADER_LENGTH = 2;

/*
The following figure shows the MX convergence control protocol stack, and MX
control PDU goes through the MX adaptation sublayer the same way as MX
data PDU.

                        <----MX Control PDU Payload --------------->
                        +------------------------------------------+
                        | Type | CID |       MX Control Message    |
                        +------------------------------------------+
                                    /                               \
                                  /                                   \  

                      +-----------------------------------------------------------------------------------+
Probe message(type 1) | Sequence Number(2B) | Link Status(1B) | Probe Flag(1B)| R-CID(1B) | TimeStamp(4B) |
                      +-----------------------------------------------------------------------------------+

                      +----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
TSU message(type 5)   | Sequence Number(2B) | Link Status(1B) | Flow ID(1B) | reverseFlag (1B) | N(1B) | K[1](1B) |  K[2](1B) | ... | K[N] (1B) | L(1B) | owd[1](1B) |  owd[2](1B) | ... | owd[N] (1B) | Queueing Delay[1](1B) |  Queueing Delay[2](1B) | ... | Queueing Delay[N] (1B) |
size = 7+3*N bytes    +--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+

                      +-------------------------------------+
ACK(type 6)           | Sequence Number(2B) | TimeStamp(4B) |
                      +-------------------------------------+

                      +----------------------------------------------------------------------------------------------------------------------------------------------------------+
TSA message(type 7)   | Sequence Number(2B) | TimeStamp(4B) | Flow ID(1B) | StartSN (3B) | delay (1B) | N(1B) | owd offset[1](1B) | owd offset[2](1B) | ... | owd offset[N] (1B) |
size = 12+N bytes     +----------------------------------------------------------------------------------------------------------------------------------------------------------+

                              +------------------------------------------------------------------------------------------------------------------------------+
QoS Testing Request (type 8)  | Sequence Number(2B) | TimeStamp(4B) | Flow ID(1B) | Traffic Direction (1B) | Test Duration 100ms Unit (1B) | Channel ID (1B) |  
                              +------------------------------------------------------------------------------------------------------------------------------+

                                     +------------------------------------------------------------------------------------------------------------+
QoS Testing Start Notify (type 9)    | Sequence Number(2B) | TimeStamp(4B) | Flow ID(1B) | Traffic Direction (1B) | Test Duration 100ms Unit (1B) |  
                                     +------------------------------------------------------------------------------------------------------------+

                                     +----------------------------------------------------------------------------+
QoS Violation Notify (type 10)       | Sequence Number(2B) | TimeStamp(4B) | Flow ID(1B) | Traffic Direction (1B) | 
                                     +----------------------------------------------------------------------------+
*/


class MxControlHeader : public Header
{
public:
  MxControlHeader ();
  virtual ~MxControlHeader ();

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  TypeId GetInstanceTypeId (void) const;
  void Print (std::ostream &os) const;
  uint32_t GetSerializedSize (void) const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void SetType (uint8_t type);
  uint8_t GetType () const;
  void SetConnectionId (uint8_t connectionId);
  uint8_t GetConnectionId () const;

  /*for Probe, TSU & TSA msg*/
  void SetSequenceNumber (uint16_t seqNum);
  uint16_t GetSequenceNumber() const;

  /*for Prob and ACK msg*/
  void SetTimeStamp (uint32_t timeStamp);
  uint32_t GetTimeStamp() const;

  /*for Probe mssage*/
  void SetProbeFlag (uint8_t probeFlag);
  uint8_t GetProbeFlag () const;
  void SetRCid (uint8_t rCid);
  uint8_t GetRCid () const;

  /*for probe and TSU*/
  void SetLinkStatus (uint8_t status);
  uint8_t GetLinkStatus () const;


  /*for TSU msg*/
  void SetReverseFlag (uint8_t flag);
  uint8_t GetReverseFlag () const;

  void SetFlowId (uint8_t flowId);
  uint8_t GetFlowId() const;

  std::vector<uint8_t> GetKVector() const;
  void SetTsuKandMeasureVectors(const std::vector<uint8_t> kVector, const std::vector<uint8_t> owdVector, const std::vector<uint8_t> queuingDelayVector);
  std::vector<uint8_t> GetOwdVector() const;
  std::vector<uint8_t> GetQueueingDelayVector() const;

  void SetL (uint8_t L);
  uint8_t GetL() const;

  /*for TSA msg*/
  void SetStartSn (uint32_t startSn);
  uint32_t GetStartSn() const;

  void SetDelay(uint8_t delay);
  uint8_t GetDelay() const;
  void SetTsaOwdVectors (std::vector<uint8_t> owdVector);

  /*for QOS msg*/
  void SetTrafficDirection (uint8_t direction);
  uint8_t GetTrafficDirection () const;

  void SetTestDuration (uint8_t duration);
  uint8_t GetTestDuration () const;

  void SetChannelId (uint8_t channelId);
  uint8_t GetChannelId () const;

private:
  uint8_t m_type;
  uint8_t m_connectionId;
  uint32_t m_msgBytes;

  /*for Probe, TSU, ACK and TSA msg*/
  uint16_t m_sequenceNumber;

  /*for Probe and ACK and TSA msg*/
  uint32_t m_timeStamp;

  /*for probe and TSU*/
  uint8_t m_linkStatus;

  /*for Probe msg*/
  uint8_t m_probeFlag;
  uint8_t m_rCid;

  /*for TSU and TSA msg*/
  uint8_t m_flowId;
  uint8_t m_N;
  std::vector<uint8_t> m_owdVector;

  /*for TSU */
  uint8_t m_reverseFlag = 0;
  std::vector<uint8_t> m_kVector;
  uint8_t m_L;
  std::vector<uint8_t> m_queueingDelayVector;

  /*for TSA msg*/
  uint32_t m_startSn;
  uint8_t m_delay;
  /*for QOS msg*/
  uint8_t m_trafficDirection; //0 downlink (server->client) 1 uplink (client->server) 2 (both)
  uint8_t m_testDuration; //seconds
  uint8_t m_channelId = UINT8_MAX;

};

} //namespace ns3

#endif /* GMA_TRAILER_H */
