/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "mx-control-header.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MxControlHeader");

NS_OBJECT_ENSURE_REGISTERED (MxControlHeader);

MxControlHeader::MxControlHeader ()
{
  m_type = 0;
  m_connectionId = 0;
  m_msgBytes = 0;
  m_sequenceNumber = 0;
  m_timeStamp = 0;
  m_linkStatus = 0;
  m_probeFlag = 0;
  m_rCid = 0;
  m_flowId = 0;
  m_N = 0;
  m_reverseFlag = 0;
  m_L = 0;
  m_startSn = 0;
  m_delay = 0;
  m_trafficDirection = 0;
  m_testDuration = 0;
  m_channelId = UINT8_MAX;

}

MxControlHeader::~MxControlHeader ()
{
}

TypeId
MxControlHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MxControlHeader")
    .SetParent<Header> ()
    .SetGroupName ("Gma")
    .AddConstructor<MxControlHeader> ()
  ;
  return tid;
}

TypeId
MxControlHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
MxControlHeader::Print (std::ostream &os) const
{
  if (m_type == 1)//Probe
  {
    os<<"Type=["<<(int)m_type <<"] CID=["<<(int)m_connectionId<< "] Sequence #=[" << (int)m_sequenceNumber << "] linkStatus["<< +m_linkStatus
    <<"] ProbeFlag=["<<(int)m_probeFlag<<"] R-CID=[" << (int)m_rCid << "] TimeStamp=[" << (int)m_timeStamp <<"]";
  }
  else if(m_type == 5)//TSU
  {
    os<<"Type=["<<(int)m_type <<"] CID=["<<(int)m_connectionId<< "] Sequence #=[" << (int)m_sequenceNumber << "] linkStaus["<< +m_linkStatus
    <<"] FlowId=[" << (int)m_flowId << "] reverseFlag [" << (int)m_reverseFlag<< "] N=[" << (int)m_N<<"] K=[";

    for(uint8_t ind = 0; ind < m_N; ind++)
    {
    os << (int)m_kVector.at(ind) << " ";
    } 
    os<<"] L=["<<(int)m_L << "]";
    os<<" owd=[";
    for(uint8_t ind = 0; ind < m_N; ind++)
    {
    os << (int)m_owdVector.at(ind) << " ";
    }
    os<<"] queueing delay =[";
    for(uint8_t ind = 0; ind < m_N; ind++)
    {
    os<< (int)m_queueingDelayVector.at(ind) << " ";
    }
    os<<"]";
  }
  else if (m_type == 6)//ACK
  {
    os<<"Type=["<<(int)m_type <<"] CID=["<<(int)m_connectionId<< "] ACK #=[" << (int)m_sequenceNumber
    <<"] TimeStamp=["<<(int)m_timeStamp<<"]";
   
  }
  else if (m_type == 7)//TSA
  {
  os<<"Type=["<<(int)m_type <<"] CID=["<<(int)m_connectionId<< "] ACK #=[" << (int)m_sequenceNumber
   <<"] TimeStamp=["<<(int)m_timeStamp<<"] FlowId=[" << (int)m_flowId  <<"] StartSn=["<<(int)m_startSn <<"] delay=["<<(int)m_delay << "] N=[" << (int)m_N << "] ";

    os<<"owd_offset =[";
    for(uint8_t ind = 0; ind < m_N; ind++)
    {
    os<< (int)m_owdVector.at(ind)-127 << " ";
    }
    os<<"]";
  }
  else
  {
    NS_FATAL_ERROR("ONLY implemented Probe(1), TSU(5), ACK (6), TSA(7) and QOS (8, 9, 10)msgs");
  }
}

uint32_t
MxControlHeader::GetSerializedSize (void) const
{
  return MX_CONTROL_HEADER_LENGTH+m_msgBytes;
}

void
MxControlHeader::Serialize (Buffer::Iterator start) const
{

  start.WriteU8 (m_type);
  start.WriteU8 (m_connectionId);

  if (m_type == 1) //Prob MSG
  {
    start.WriteU16 (m_sequenceNumber);
    start.WriteU8 (m_linkStatus);
    start.WriteU8 (m_probeFlag);
    start.WriteU8 (m_rCid);
    start.WriteU32 (m_timeStamp);
  }
  else if (m_type == 5) //TSU MSG
  {
      start.WriteU16 (m_sequenceNumber);
      start.WriteU8 (m_linkStatus);
      start.WriteU8 (m_reverseFlag);
      start.WriteU8 (m_flowId);
      start.WriteU8 (m_N);
      for (uint8_t ind = 0; ind < m_N; ind++)
      {
        start.WriteU8 (m_kVector.at(ind));
      }
      start.WriteU8 (m_L);
      for (uint8_t ind = 0; ind < m_N; ind++)
      {
        start.WriteU8 (m_owdVector.at(ind));
      }
      for (uint8_t ind = 0; ind < m_N; ind++)
      {
        start.WriteU8 (m_queueingDelayVector.at(ind));
      }
  }
  else if (m_type == 6) //ACK
  {
    start.WriteU16 (m_sequenceNumber);
    start.WriteU32 (m_timeStamp);
  }
  else if (m_type == 7) // TSA MSG
  {
      start.WriteU16 (m_sequenceNumber);
      start.WriteU32 (m_timeStamp);
      start.WriteU8 (m_flowId);
      start.WriteU8 ((m_startSn & 0x00ff0000) >> 16);
      start.WriteU8 ((m_startSn & 0x0000ff00) >> 8);
      start.WriteU8 (m_startSn & 0x000000ff);
      start.WriteU8 (m_delay);
      start.WriteU8 (m_N);
      for (uint8_t ind = 0; ind < m_N; ind++)
      {
        start.WriteU8 (m_owdVector.at(ind));
      }
        
  }
  else
  {
      NS_FATAL_ERROR("ONLY implemented Probe(1), TSU(5), ACK(6), TSA(7), and QOS (8, 9, 10) msgs");
  }

}

uint32_t
MxControlHeader::Deserialize (Buffer::Iterator start)
{
  m_type = start.ReadU8 ();
  m_connectionId = start.ReadU8 ();

  if (m_type == 1)
  {
    m_msgBytes = 9;
  }
  else if (m_type == 5)
  {
    m_msgBytes = 7; //the 3*N bytes is added later;
  }
  else if (m_type == 6)
  {
    m_msgBytes = 6;
  }
  else if (m_type == 7)
  {
    m_msgBytes = 12;//the N bytes is added later;
  }
  else if (m_type == 8)
  {
    m_msgBytes = 10;
  }
  else if (m_type == 9)
  {
    m_msgBytes = 9;
  }
  else if (m_type == 10)
  {
    m_msgBytes = 8;
  }
  else
  {
      NS_FATAL_ERROR("ONLY implemented Probe(1), TSU(5), ACK(6), TSA(7) and QOS(8,9, 10) msgs");
  }

  if (m_type == 1) //Prob MSG
  {
    m_sequenceNumber = start.ReadU16 ();
    m_linkStatus = start.ReadU8 ();
    m_probeFlag = start.ReadU8 ();
    m_rCid = start.ReadU8 ();
    m_timeStamp = start.ReadU32 ();
  }
  else if (m_type == 5) //TSU MSG
  {

    m_sequenceNumber = start.ReadU16 ();
    m_linkStatus = start.ReadU8 ();
    m_reverseFlag = start.ReadU8 ();
    m_flowId = start.ReadU8 ();
    m_N = start.ReadU8 ();
    m_kVector.clear();
    for (uint8_t ind = 0; ind < m_N; ind++)
      {
        m_kVector.push_back(start.ReadU8 ());
      }
    m_L = start.ReadU8 ();
    m_owdVector.clear();
    for (uint8_t ind = 0; ind < m_N; ind++)
    {
      m_owdVector.push_back(start.ReadU8 ());
    }
    m_queueingDelayVector.clear();
    for (uint8_t ind = 0; ind < m_N; ind++)
    {
      m_queueingDelayVector.push_back(start.ReadU8 ());
    }
    m_msgBytes += 3*m_N;
  }
  else if (m_type == 6) //ACK
  {
    m_sequenceNumber = start.ReadU16 ();
    m_timeStamp = start.ReadU32 ();
  }
  else if (m_type == 7) // TSA MSG
  {
    m_sequenceNumber = start.ReadU16 ();
    m_timeStamp = start.ReadU32 ();
    m_flowId = start.ReadU8 ();
    m_startSn = start.ReadU8 () << 16 | start.ReadU8 () << 8 | start.ReadU8 ();
    m_delay = start.ReadU8 ();
    m_N = start.ReadU8 ();
    for (uint8_t ind = 0; ind < m_N; ind++)
    {
      m_owdVector.push_back(start.ReadU8 ());
    }
    m_msgBytes += m_N;
  }
  else
  {
      NS_FATAL_ERROR("ONLY implemented Probe(1), TSU(5), ACK(6), TSA(7) and QOS (8, 9, 10) msgs");
  }
  return MX_CONTROL_HEADER_LENGTH+m_msgBytes;
}

void
MxControlHeader::SetType (uint8_t type)
{
  /*
  Type (1 Byte): the type of the MX control message
     + 0: Keep-Alive
     + 1: Probe REQ/ACK
     + 2: Packet Loss Report (PLR)
     + 3: First Sequence Number (FSN)
     + 4: Coded MX SDU (CMS)
     + 5: Traffic Splitting Update (TSU)
     + 6: ACK
     + 7: Traffic Splitting Acknowledgement (TSA)
     + 8: QoS Testing Request
     + 9: QoS Testing Start Notify
     + 10: QoS Violation Notify
     + Others: reserved
  */
  m_type = type;
  if (m_type == 1)
  {
    m_msgBytes = 9;
  }
  else if (m_type == 5)
  {
    //it is configured when setting the kVector
  }
  else if (m_type == 6)
  {
    m_msgBytes = 6;
  }
  else if (m_type == 7)
  {
    //it is configured when setting the owdVector
  }
  else if (m_type == 8)
  {
    m_msgBytes = 10;
  }
  else if (m_type == 9)
  {
    m_msgBytes = 9;
  }
  else if (m_type == 10)
  {
    m_msgBytes = 8;
  }
  else
  {
      NS_FATAL_ERROR("ONLY implemented Probe(1), TSU(5), ACK(6), TSA(7) and QOS(8,9,10) msgs");
  }
}

uint8_t
MxControlHeader::GetType () const
{
  return m_type;
}


void
MxControlHeader::SetConnectionId (uint8_t connectionId)
{
  /*
  CID (1 Byte): the connection ID of the delivery connection for
  sending out the MX control message
  */
  m_connectionId = connectionId;
}

uint8_t
MxControlHeader::GetConnectionId () const
{
  return m_connectionId;
}

/*for Probe, TSU & TSA msg*/
void
MxControlHeader::SetSequenceNumber (uint16_t seqNum)
{
  m_sequenceNumber = seqNum;
}

uint16_t
MxControlHeader::GetSequenceNumber() const
{
  return m_sequenceNumber;
}

/*for Prob and ACK msg*/
void
MxControlHeader::SetTimeStamp (uint32_t timeStamp)
{
  m_timeStamp = timeStamp;
}

uint32_t 
MxControlHeader::GetTimeStamp() const
{
  return m_timeStamp;
}

/*for Probe mssage*/
void
MxControlHeader::SetProbeFlag (uint8_t probeFlag)
{
  m_probeFlag = probeFlag;
}

uint8_t
MxControlHeader::GetProbeFlag () const
{
  return m_probeFlag;
}

void
MxControlHeader::SetRCid (uint8_t rCid)
{
  m_rCid = rCid;
}

uint8_t
MxControlHeader::GetRCid () const
{
  return m_rCid;
}

void
MxControlHeader::SetLinkStatus (uint8_t status)
{
  m_linkStatus = status;
}

uint8_t
MxControlHeader::GetLinkStatus () const
{
  return m_linkStatus;
}

/*for TSU msg*/
void
MxControlHeader::SetReverseFlag (uint8_t flag)
{
  m_reverseFlag = flag;
}

uint8_t
MxControlHeader::GetReverseFlag() const
{
  return m_reverseFlag;
}

void
MxControlHeader::SetFlowId (uint8_t flowId)
{
  m_flowId = flowId;
}

uint8_t
MxControlHeader::GetFlowId() const
{
  return m_flowId;
}

void
MxControlHeader::SetTsuKandMeasureVectors (std::vector<uint8_t> kVector, std::vector<uint8_t> owdVector, std::vector<uint8_t> queuingDelayVector)
{
  m_kVector = kVector;
  m_N = kVector.size();
  NS_ASSERT_MSG(m_N, "the size of kVector cannot be zero!");

  if (owdVector.size() == 0)
  {
    for (uint8_t ind = 0; ind < m_N; ind++)
    {
      m_owdVector.push_back(UINT8_MAX);//no delay measurement.
    }
  }
  else
  {
    if (kVector.size() != owdVector.size())
    {
      NS_FATAL_ERROR("the size of the splitting ratio and min owd measurement is not the same!");
    }
    m_owdVector = owdVector;
  }

  if (queuingDelayVector.size() == 0)
  {
    for (uint8_t ind = 0; ind < m_N; ind++)
    {
      m_queueingDelayVector.push_back(UINT8_MAX);//no delay measurement.
    }
  }
  else
  {
    if (kVector.size() != queuingDelayVector.size())
    {
      NS_FATAL_ERROR("the size of the splitting ratio and min owd measurement is not the same!");
    }
    m_queueingDelayVector = queuingDelayVector;
  }

  m_msgBytes = 7+3*m_N;
}

void
MxControlHeader::SetTsaOwdVectors (std::vector<uint8_t> owdVector)
{
  m_owdVector = owdVector;
  m_N = m_owdVector.size();
  NS_ASSERT_MSG(m_N, "the size of owdVector cannot be zero!");
  m_msgBytes = 12+m_N;
}

std::vector<uint8_t> 
MxControlHeader::GetKVector() const
{
  return m_kVector;
}

std::vector<uint8_t> 
MxControlHeader::GetOwdVector() const
{
  return m_owdVector;
}

std::vector<uint8_t> 
MxControlHeader::GetQueueingDelayVector() const
{
  return m_queueingDelayVector;
}

void
MxControlHeader::SetL (uint8_t L)
{
  m_L = L;
}

uint8_t
MxControlHeader::GetL() const
{
  return m_L;
}

/*for TSA msg*/
void
MxControlHeader::SetStartSn (uint32_t startSn)
{
  m_startSn = startSn;
}

uint32_t
MxControlHeader::GetStartSn() const
{
  return m_startSn;
}

void
MxControlHeader::SetDelay(uint8_t delay)
{
  m_delay = delay;
}

uint8_t
MxControlHeader::GetDelay() const
{
  return m_delay;
}


/*for QOS msg*/
void
MxControlHeader::SetTrafficDirection (uint8_t direction)
{
  m_trafficDirection = direction;
}

uint8_t
MxControlHeader::GetTrafficDirection() const
{
  return m_trafficDirection;
}

void
MxControlHeader::SetTestDuration (uint8_t duration)
{
  m_testDuration = duration;
}

uint8_t
MxControlHeader::GetTestDuration() const
{
  return m_testDuration;
}


void
MxControlHeader::SetChannelId (uint8_t channelId)
{
  m_channelId = channelId;
}

uint8_t
MxControlHeader::GetChannelId() const
{
  return m_channelId;
}


} //namespace ns3

