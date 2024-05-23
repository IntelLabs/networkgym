/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 *
 */

#ifndef POISSON_UDP_CLIENT_H
#define POISSON_UDP_CLIENT_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/random-variable-stream.h"
namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup udpclientserver
 *
 * \brief A Udp client. Sends UDP packet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class PoissonUdpClient : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  PoissonUdpClient ();

  virtual ~PoissonUdpClient ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Address ip, uint16_t port);
  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address addr);

  /**
   * \return the total bytes sent by this app
   */
  uint64_t GetTotalTx () const;

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void ReadTraceFile (void);
  void UpdateInterval (void);
  /**
   * \brief Send a packet
   */
  void Send (void);

  uint32_t m_count = 0; //!< Maximum number of packets the application will send
  Time m_interval; //!< Packet inter-send time
  uint32_t m_size = 0; //!< Size of the sent packet (including the SeqTsHeader)
  uint32_t m_burst = 0; //!< size of the packet burst
  uint32_t m_burstCounter = 0; //!< number of packet been transmitted in one packet burst
  
  uint32_t m_sent = 0; //!< Counter for sent packets
  uint64_t m_totalTx = 0; //!< Total bytes sent
  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort = 0; //!< Remote peer port
  EventId m_sendEvent; //!< Event to send the next packet
  bool m_enablePoissonArrival = false; //packet arrival is a poisson process
  std::string m_rateTraceFileName; //the path of the send trace file
  std::vector<double> m_trueRateList; //!< send rate
  std::vector<double> m_automlRateList; //!< send rate 
  std::vector<double> m_naiveRateList; //!< send rate 
  Time m_traceUpdateInterval;
  uint32_t m_userNum = 0; //!< num of users
  Ptr<UniformRandomVariable> m_urv;
};

} // namespace ns3

#endif /* POISSON_UDP_CLIENT_H */