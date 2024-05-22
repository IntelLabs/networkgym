/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "gma-trailer.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GmaTrailer");

NS_OBJECT_ENSURE_REGISTERED (GmaTrailer);

GmaTrailer::GmaTrailer ()
{
  m_nextHeader = 0;
  m_sequenceNumber = 0;
  m_timeStamp = 0;
  m_connectionId = 0;
  m_optionBytes = 0;

}

GmaTrailer::~GmaTrailer ()
{
}

TypeId
GmaTrailer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GmaTrailer")
    .SetParent<Trailer> ()
    .SetGroupName ("Gma")
    .AddConstructor<GmaTrailer> ()
  ;
  return tid;
}

TypeId
GmaTrailer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
GmaTrailer::Print (std::ostream &os) const
{
  os<<"flags=["<<m_flagsBits[0]<<m_flagsBits[1]<<m_flagsBits[2]<<m_flagsBits[3]
  <<m_flagsBits[4]<<m_flagsBits[5]<<m_flagsBits[6]<<m_flagsBits[7]<< "] next header=["<<(int)m_nextHeader<< "]";
}

uint32_t
GmaTrailer::GetSerializedSize (void) const
{
  return GMA_TRAILER_LENGTH+m_optionBytes;
}

void
GmaTrailer::Serialize (Buffer::Iterator start) const
{
  //move the pointer to the begining of the trailer.
  start.Prev (GMA_TRAILER_LENGTH+m_optionBytes);

  // only implemented the sequence # and timestamp filed so far.

  if(m_flagsBits[2] == 1)
  {
    start.WriteU8(m_connectionId);
  }

  if(m_flagsBits[5] == 1)
  {
    start.WriteU8 ((m_sequenceNumber & 0x00ff0000) >> 16);
    start.WriteU8 ((m_sequenceNumber & 0x0000ff00) >> 8);
    start.WriteU8 (m_sequenceNumber & 0x000000ff);
  }

  if(m_flagsBits[6] == 1)
  {
    start.WriteU32(m_timeStamp);
  }

  // convert the flags from bits to integer.
  uint8_t flags = 0;
  for (uint8_t ind = 0; ind < 8; ind++)
  {
    flags += (m_flagsBits[ind] << (8-ind-1));
  }
  start.WriteU8 (flags);

  start.WriteU8 (m_nextHeader);
}

uint32_t
GmaTrailer::Deserialize (Buffer::Iterator start)
{
  //copmute the m_optionBytes;
  CalOptionBytes(start);

  //move the pointer to the begining of the trailer.
  start.Prev (GMA_TRAILER_LENGTH+m_optionBytes);

  //start reading the info from the header.
  if(m_flagsBits[2] == 1)
  {
    m_connectionId = start.ReadU8 ();
  }
  if(m_flagsBits[5] == 1)
  {
    m_sequenceNumber = start.ReadU8 () << 16 | start.ReadU8 () << 8 | start.ReadU8 ();
  }
  if(m_flagsBits[6] == 1)
  {
    m_timeStamp = start.ReadU32 ();
  }
  uint8_t flags = start.ReadU8 ();
  for (uint8_t ind = 0; ind < 8; ind++)
  {
    m_flagsBits [7-ind] = flags%2;
    flags >>= 1;
  }
  m_nextHeader = start.ReadU8 ();
  return GMA_TRAILER_LENGTH+m_optionBytes; // the number of bytes consumed.
}

void
GmaTrailer::CalOptionBytes(Buffer::Iterator start)
{
  start.Prev (GMA_TRAILER_LENGTH);
  uint8_t flags = start.PeekU8 ();

  //check each bit of the flags and compute the size of the optional field.
  for (uint8_t ind = 0; ind < 8; ind++)
  {
    m_flagsBits [7-ind] = flags%2;
    flags >>= 1;
    m_optionBytes += m_optionSize[7-ind]*m_flagsBits [7-ind];
  }
}


void
GmaTrailer::SetNextHeader (uint8_t header)
{
  m_nextHeader = header;
}
uint8_t
GmaTrailer::GetNextHeader() const
{
  return m_nextHeader;
}

void
GmaTrailer::SetSequenceNumber (uint32_t sequenceNumber)
{
  /*
  Sequence Number (3 Bytes): an auto-incremented integer to
        indicate order of transmission of the SDU (e.g. IP packet),
        needed for lossless switching or multi-link (path) aggregation
        or fragmentation. Sequence Number SHALL be generated per flow.
  */
  m_sequenceNumber = sequenceNumber;
  //set the sequence number bits flag to true.
  m_flagsBits[5] = 1;
  //Add the size of sequence number to the option bytes.
  m_optionBytes += m_optionSize[5];
}

uint32_t
GmaTrailer::GetSequenceNumber () const
{
  return m_sequenceNumber;
}

void
GmaTrailer::SetTimeStamp (uint32_t timeStamp)
{
  /*
  Timestamp (4 Bytes): to contain the current value of the
        timestamp clock of the transmitter in the unit of
        milliseconds.
  */
  m_timeStamp = timeStamp;
  //set the sequence number bits flag to true.
  m_flagsBits[6] = 1;
  //Add the size of timestamp to the option bytes.
  m_optionBytes += m_optionSize[6];
}

uint32_t
GmaTrailer::GetTimeStamp () const
{
  return m_timeStamp;
}

void
GmaTrailer::SetConnectionId (uint8_t connectionId)
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
  m_optionBytes += m_optionSize[2];
}

uint8_t
GmaTrailer::GetConnectionId () const
{
  return m_connectionId;
}



} //namespace ns3

