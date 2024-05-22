/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */


#ifndef GMA_TRAILER_H
#define GMA_TRAILER_H

#include "ns3/trailer.h"
#include "ns3/log.h"

namespace ns3 {


static const uint16_t GMA_TRAILER_LENGTH = 2;

  /*
  The following figure shows the trailer-based encapsulation GMA PDU (protocol
  data unit) format for the convergence sublayer. A (GMA) PDU may
  carry one or multiple IP packets, aka (GMA) SDUs (service data
  unit), in the payload, or a fragment of the SDU.

          +------------------------------------------------------+
          | IP hdr |        IP payload             | GMA Trailer |
          +------------------------------------------------------+


  The following figure shows the GMA trailer format with all the fields
  present. The GMA (Generic Multi-Access) trailer MUST consist of the
  following two mandatory fields. The Next Header field is always the
  last octet at the end of the PDU, and the Flags field is the second
  last. The Receiver SHOULD first decode the Flags field to determine
  the length of the GMA trailer, and then decode each optional field
  accordingly.

   0                   1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                Timestamp                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                Sequence   Number              |   FC          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   Flow ID     | Connection ID |    First SDU Length           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   Checksum    |   Flags       | Next Header   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  */

class GmaTrailer : public Trailer
{
public:
  GmaTrailer ();
  virtual ~GmaTrailer ();

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
  void SetNextHeader (uint8_t header);
  uint8_t GetNextHeader() const;
  void SetSequenceNumber (uint32_t sequenceNumber);
  uint32_t GetSequenceNumber () const;
  void SetTimeStamp (uint32_t timeStamp);
  uint32_t GetTimeStamp () const;
  void SetConnectionId (uint8_t connectionId);
  uint8_t GetConnectionId () const;

private:
  /*
  this function compute the total size of the optional field in Bytes.
  */
  void CalOptionBytes(Buffer::Iterator start);

  //the following parameters stores the data for each field in the GMA trailer.
  uint8_t m_nextHeader;
  uint32_t m_sequenceNumber;
  uint32_t m_timeStamp;
  uint8_t m_connectionId;
  
 // the size of the entire optional field in Bytes.
  uint16_t m_optionBytes;
  /*
  Flags (1 Byte): Bit 0 is the least significant bit (LSB), and
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
        + Sequence Number Present (bit 5): If the Sequence Number
        Present bit is set to 1, then the Sequence Number field is
        present and contains valid information.
        + Timestamp Present (bit 6): If the Timestamp Present bit is
        set to 1, then the Timestamp field is present and contains
        valid information.
        + Bit 7: reserved
  */
  bool m_flagsBits [8] = {0, 0, 0, 0, 0, 0, 0, 0};

  //the size of each optional field in Bytes.
  uint8_t m_optionSize [8] = {1, 2, 1, 1, 1, 3, 4, 1};

};

} //namespace ns3

#endif /* GMA_TRAILER_H */
