/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef GMA_HEADER_H
#define GMA_HEADER_H

#include "ns3/header.h"
#include "ns3/log.h"

namespace ns3 {


static const uint16_t GMA_HEADER_LENGTH = 2;

  /*
  The following figure shows the header-based encapsulation GMA PDU
  (protocol data unit) format for the convergence sublayer. The flag
  feild set bit 3, 5, 6 and 7 to 1. If the packet is retransmitted
  over a different link, bit 2 is set to 1 and indicate the original
  connection ID, not the one performing retransmission.

    +---------------------------------------------------------------+
    | IP hdr |  UDP header | GMA Header | (virtual)IP hdr | payload |
    +---------------------------------------------------------------+
                          /              \
      +----------------------------------------------------------+
      | Flag(2B) | TimeStamp(4B) |  SN(3B) | LSN(1B) | FlowID(1B) |
      +----------------------------------------------------------+
                      bit(7)       bit(6)    bit(5)    bit(3)

  The format for control messasge is as following, the flag field is set to 0

    +------------------------------------------------+
    | IP hdr |  UDP header | GMA Header | Control msg|
    +------------------------------------------------+
                          /              \
                      +-----------------------------------+
                      | Flag(2B) | ClientId(1B, Optional) |
                      +-----------------------------------+
                                      bit(3)

  The following figure shows the GMA header format with all the fields
  present. The GMA (Generic Multi-Access) header MUST consist of the
  mandatory flag field. The Receiver SHOULD first decode the Flags field to determine
  the length of the GMA trailer, and then decode each optional field
  accordingly.

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +---------------+---------------+---------------+---------------+
  |     Flags     |                  timeStamp                    
  +---------------+---------------+---------------+---------------+
                  |                 Sequence Number               |
  +---------------+---------------+---------------+---------------+
  |    Local SN   |       FC      |     Flow ID   | Connection ID |
  +---------------+---------------+---------------+---------------+
  |    Checksum   |
  +---------------+
  */

class GmaHeader : public Header
{
public:
  GmaHeader ();
  virtual ~GmaHeader ();

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

  uint16_t GetFlags () const;

  void SetTimeStamp (uint32_t timeStamp);
  uint32_t GetTimeStamp () const;

  void SetSequenceNumber (uint32_t sequenceNumber);
  uint32_t GetSequenceNumber () const;

  void SetLocalSequenceNumber (uint8_t LocalSequenceNumber);
  uint8_t GetLocalSequenceNumber () const;

  void SetFlowId (uint8_t flowId);
  uint8_t GetFlowId () const;

  /*connection ID is set for the retransmitted packet to indetify the connected link*/
  void SetConnectionId (uint8_t connectionId);
  uint8_t GetConnectionId () const;

  void SetClientId (uint16_t clientId);
  uint16_t GetClientId () const;

private:
  //the following parameters stores the data for each field in the GMA trailer.
  
  uint32_t m_timeStamp;
  uint8_t m_localSequenceNumber;
  uint32_t m_sequenceNumber;
  uint32_t m_flowId;
  uint8_t m_connectionId;
  uint16_t m_clientId;
 // the size of the entire optional field in Bytes.
  uint16_t m_optionBytes;
  /*
  Flags (2 Byte): Bit 0 is the least significant bit (LSB), and
        Bit 7 is the most significant bit (MSB).
        + Checksum Present (bit 0): If the Checksum Present bit is set
        to 1, then the Checksum field is present and contains valid
        information.
        + Concatenation Present (bit 1): If the Concatenation Present
        bit is set to 1, then the PDU carries multiple SDUs, and the
        First SDU Length field is present and contains valid
        information.
        + Connection ID Present (bit 2): If the Connection ID Present
        bit is set to 1, then the Connection ID field is present and
        contains valid information.
        + Flow ID Present (bit 3): If the Flow ID Present bit is set
        to 1, then the Flow ID field is present and contains valid
        information.
        + Fragmentation Present (bit 4): If the Fragmentation Present
        bit is set to 1, then the PDU carry a fragment of the SDU and
        the Fragmentation Control field is present and contains valid
        information.
        + Local Sequence Number Present (bit 5): If the Local Sequence
        Number Present bit is set to 1, then the Local Sequence Number
        field is present and contains valid information.
        + Sequence Number Present (bit 6): If the Sequence Number
        Present bit is set to 1, then the Sequence Number field is
        present and contains valid information.
        + Timestamp Present (bit 7): If the Timestamp Present bit is
        set to 1, then the Timestamp field is present and contains
        valid information.
        + Client Id (bit 8): 2Bytes, Server use client Id to find virtual interface.
        + Bit 8-15: reserved
  */
  bool m_flagsBits [16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,};

  //the size of each optional field in Bytes.
  uint8_t OPTION_SIZE_LIST [16] = {1, 0, 1, 1, 1, 1, 3, 4, 2, 0, 0, 0, 0, 0, 0, 0};

};

} //namespace ns3

#endif /* GMA_HEADER_H */
