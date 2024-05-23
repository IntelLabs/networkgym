/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "gma-header.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GmaHeader");

NS_OBJECT_ENSURE_REGISTERED (GmaHeader);

GmaHeader::GmaHeader ()
{
  m_timeStamp = 0;
  m_localSequenceNumber = 0;
  m_sequenceNumber = 0;
  m_flowId = 0;
  m_connectionId = 0;
  m_clientId = 0;
  m_optionBytes = 0;
}

GmaHeader::~GmaHeader ()
{
}

TypeId
GmaHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GmaHeader")
    .SetParent<Header> ()
    .SetGroupName ("Gma")
    .AddConstructor<GmaHeader> ()
  ;
  return tid;
}

TypeId
GmaHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
GmaHeader::Print (std::ostream &os) const
{
  os<<"flags=["<<m_flagsBits[0]<<m_flagsBits[1]<<m_flagsBits[2]<<m_flagsBits[3]
  <<m_flagsBits[4]<<m_flagsBits[5]<<m_flagsBits[6]<<m_flagsBits[7]
  <<m_flagsBits[8]<<m_flagsBits[9]<<m_flagsBits[10]<<m_flagsBits[11]
  <<m_flagsBits[12]<<m_flagsBits[13]<<m_flagsBits[14]<<m_flagsBits[15]<< "]";
}

uint32_t
GmaHeader::GetSerializedSize (void) const
{
  return GMA_HEADER_LENGTH+m_optionBytes;
}

void
GmaHeader::Serialize (Buffer::Iterator start) const
{
  // convert the flags from bits to integer.
  uint16_t flags = 0;
  for (uint8_t ind = 0; ind < 16; ind++)
  {
    flags += (m_flagsBits[ind] << (ind));
  }

  start.WriteU16 (flags);

  if(m_flagsBits[8] == 1)
  {
    start.WriteU16(m_clientId);
  }

  if(m_flagsBits[7] == 1)
  {
    start.WriteU32(m_timeStamp);
  }

  if(m_flagsBits[6] == 1)
  {
    start.WriteU8 ((m_sequenceNumber & 0x00ff0000) >> 16);
    start.WriteU8 ((m_sequenceNumber & 0x0000ff00) >> 8);
    start.WriteU8 (m_sequenceNumber & 0x000000ff);
  }

  if(m_flagsBits[5] == 1)
  {
    start.WriteU8(m_localSequenceNumber);
  }

  if(m_flagsBits[3] == 1)
  {
    start.WriteU8(m_flowId);
  }

  if(m_flagsBits[2] == 1)
  {
    start.WriteU8(m_connectionId);
  }

  if(m_flagsBits[0] == 1 || m_flagsBits[1] == 1 || m_flagsBits[4] == 1 || 
     m_flagsBits[9] == 1 || m_flagsBits[10] == 1||
     m_flagsBits[11] == 1 || m_flagsBits[12] == 1 || m_flagsBits[13] == 1||
     m_flagsBits[14] == 1 || m_flagsBits[15] == 1)
  {
    NS_FATAL_ERROR("this feild in GMA header is not implemented yet!!!!!!");
  }



}

uint32_t
GmaHeader::Deserialize (Buffer::Iterator start)
{
  uint16_t flags = start.ReadU16 ();

  uint32_t optionBytes = 0;
  for (uint8_t ind = 0; ind < 16; ind++)
  {
    m_flagsBits [ind] = flags%2;
    flags >>= 1;
    optionBytes += OPTION_SIZE_LIST[ind]*m_flagsBits [ind];
  }

  //start reading the info from the header.

  if(m_flagsBits[8] == 1)
  {
    m_clientId = start.ReadU16();
  }

  if(m_flagsBits[7] == 1)
  {
    m_timeStamp = start.ReadU32();
  }

  if(m_flagsBits[6] == 1)
  {
    m_sequenceNumber = start.ReadU8 () << 16 | start.ReadU8 () << 8 | start.ReadU8 ();
  }

  if(m_flagsBits[5] == 1)
  {
   m_localSequenceNumber = start.ReadU8();
  }

  if(m_flagsBits[3] == 1)
  {
    m_flowId = start.ReadU8();
  }

  if(m_flagsBits[2] == 1)
  {
    m_connectionId = start.ReadU8();
  }

  return GMA_HEADER_LENGTH+optionBytes; // the number of bytes consumed.
}

uint16_t
GmaHeader::GetFlags() const
{
  uint16_t flags = 0;
  for (uint8_t ind = 0; ind < 16; ind++)
  {
    flags += (m_flagsBits[ind] << (ind));
  }
  return flags;

}

void
GmaHeader::SetTimeStamp (uint32_t timeStamp)
{
  /*
  Timestamp (4 Bytes): to contain the current value of the
        timestamp clock of the transmitter in the unit of
        milliseconds.
  */
  m_timeStamp = timeStamp;
  //set the sequence number bits flag to true.
  m_flagsBits[7] = 1;
  //Add the size of timestamp to the option bytes.
  m_optionBytes += OPTION_SIZE_LIST[7];
}

uint32_t
GmaHeader::GetTimeStamp () const
{
  return m_timeStamp;
}

void
GmaHeader::SetSequenceNumber (uint32_t sequenceNumber)
{
  /*
  Sequence Number (3 Bytes): an auto-incremented integer to
        indicate order of transmission of the SDU (e.g. IP packet),
        needed for lossless switching or multi-link (path) aggregation
        or fragmentation. Sequence Number SHALL be generated per flow.
  */
  m_sequenceNumber = sequenceNumber;
  //set the sequence number bits flag to true.
  m_flagsBits[6] = 1;
  //Add the size of sequence number to the option bytes.
  m_optionBytes += OPTION_SIZE_LIST[6];
}

uint32_t
GmaHeader::GetSequenceNumber () const
{
  return m_sequenceNumber;
}

void
GmaHeader::SetLocalSequenceNumber (uint8_t localSequenceNumber)
{
  /*
  Local Sequence Number (1 Byte): an auto-incremented integer to
        indicate order of transmission of the SDU (e.g. IP packet)
        over the same connection. Sequence Number SHALL be generated
        per flow per connection.
  */
  m_localSequenceNumber = localSequenceNumber;
  //set the local sequence number bits flag to true.
  m_flagsBits[5] = 1;
  //Add the size of sequence number to the option bytes.
  m_optionBytes += OPTION_SIZE_LIST[5];
}

uint8_t
GmaHeader::GetLocalSequenceNumber () const
{
  return m_localSequenceNumber;
}

void
GmaHeader::SetFlowId (uint8_t flowId)
{
  /*
  Flow ID (1 Byte): an unsigned integer to identify the IP flow
          that a PDU belongs to, for example Data Radio Bearer (DRB) ID
          [LWIPEP] for a cellular (e.g. LTE) connection.
  */
  m_flowId = flowId;
  //set the flow ID bits flag to true.
  m_flagsBits[3] = 1;
  //Add the size of timestamp to the option bytes.
  m_optionBytes += OPTION_SIZE_LIST[3];
}

uint8_t
GmaHeader::GetFlowId () const
{
  return m_flowId;
}

void
GmaHeader::SetConnectionId (uint8_t connectionId)
{
  /*
  Connection ID (1 Byte): an unsigned integer to identify the
        anchor and delivery connection of the GMA PDU.
        + Anchor Connection ID (MSB 4 Bits): an unsigned integer to
        identify the anchor connection
        + Delivery Connection ID (LSB 4 Bits): an unsigned integer to
        identify the delivery connection
  */
  m_connectionId = connectionId;
  //set the connection ID bits flag to true.
  m_flagsBits[2] = 1;
  //Add the size of timestamp to the option bytes.
  m_optionBytes += OPTION_SIZE_LIST[2];
}

uint8_t
GmaHeader::GetConnectionId () const
{
  return m_connectionId;
}

void
GmaHeader::SetClientId (uint16_t clientId)
{
  /*
  Client ID (2 Byte): ID per client
  */
  m_clientId = clientId;
  //set the connection ID bits flag to true.
  m_flagsBits[8] = 1;
  //Add the size of timestamp to the option bytes.
  m_optionBytes += OPTION_SIZE_LIST[8];
}

uint16_t
GmaHeader::GetClientId () const
{
  return m_clientId;
}



} //namespace ns3

