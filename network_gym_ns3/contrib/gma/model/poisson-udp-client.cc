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
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "poisson-udp-client.h"
#include "../../src/applications/model/seq-ts-header.h"
#include <cstdlib>
#include <cstdio>
#include "ns3/string.h"
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PoissonUdpClient");

NS_OBJECT_ENSURE_REGISTERED (PoissonUdpClient);

TypeId
PoissonUdpClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PoissonUdpClient")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<PoissonUdpClient> ()
    .AddAttribute ("MaxPackets",
                   "The maximum number of packets the application will send",
                   UintegerValue (100),
                   MakeUintegerAccessor (&PoissonUdpClient::m_count),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Interval",
                   "The time to wait between packet burst", TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&PoissonUdpClient::m_interval),
                   MakeTimeChecker ())
    .AddAttribute ("RemoteAddress",
                   "The destination Address of the outbound packets",
                   AddressValue (),
                   MakeAddressAccessor (&PoissonUdpClient::m_peerAddress),
                   MakeAddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&PoissonUdpClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketSize",
                   "Size of packets generated. The minimum packet size is 12 bytes which is the size of the header carrying the sequence number and the time stamp.",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&PoissonUdpClient::m_size),
                   MakeUintegerChecker<uint32_t> (12,65507))
    .AddAttribute ("PacketBurst",
                   "Number of Packets been transmitted at the same time",
                   UintegerValue (1),
                   MakeUintegerAccessor (&PoissonUdpClient::m_burst),
                   MakeUintegerChecker<uint32_t> (1,1000))
    .AddAttribute ("PoissonArrival",
                   "Use Poisson arrival for packet generation",
                   BooleanValue (true),
                   MakeBooleanAccessor (&PoissonUdpClient::m_enablePoissonArrival),
                   MakeBooleanChecker ())
    .AddAttribute ("RateTraceFile",
                   "Use a trace file to configure sending rate",
                   StringValue (""),
                   MakeStringAccessor (&PoissonUdpClient::m_rateTraceFileName),
                   MakeStringChecker ())
    .AddAttribute ("TraceUpdateInterval",
                   "The time to wait between update the trace based rate", TimeValue (Seconds (5.0)),
                   MakeTimeAccessor (&PoissonUdpClient::m_traceUpdateInterval),
                   MakeTimeChecker ())
    .AddAttribute ("TraceUserNum",
                   "Number of Users used to scale trace file",
                   UintegerValue (2),
                   MakeUintegerAccessor (&PoissonUdpClient::m_userNum),
                   MakeUintegerChecker<uint32_t> (1,1000))
  ;
  return tid;
}

PoissonUdpClient::PoissonUdpClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_totalTx = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_urv = CreateObject<UniformRandomVariable> ();
  m_urv->SetAttribute ("Min", DoubleValue (0.0));
  m_urv->SetAttribute ("Max", DoubleValue (1.0));
}

PoissonUdpClient::~PoissonUdpClient ()
{
  NS_LOG_FUNCTION (this);
}

void
PoissonUdpClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
PoissonUdpClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
PoissonUdpClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
PoissonUdpClient::ReadTraceFile(void)
{
    // Create an input filestream
    std::ifstream myFile(m_rateTraceFileName);

    // Make sure the file is open
    if(myFile.is_open())
    {
      // Helper vars
      std::string line, colname;
      double val;

      // Read the column names
      if(myFile.good())
      {
          // Extract the first line in the file
          std::getline(myFile, line);

          // Create a stringstream from line
          std::stringstream ss(line);

          // Extract each column name
          while(std::getline(ss, colname, ',')){
              
              // Initialize and add <colname, int vector> pairs to result
              //std::cout << "colname: " << colname << std::endl;
          }
      }

      // Read data, line by line
      while(std::getline(myFile, line))
      {
          // Create a stringstream of the current line
          std::stringstream ss(line);
          
          // Keep track of the current column index
          int colIdx = 0;
          
          // Extract each integer
          while(ss >> val){
              
              // Add the current integer to the 'colIdx' column's values vector
              if (colIdx == 6)
              {
                m_trueRateList.push_back(val);
              }
              else if (colIdx == 7)
              {
                m_automlRateList.push_back(val);
              }
              else if (colIdx == 8)
              {
                m_naiveRateList.push_back(val);
              }
              // If the next token is a comma, ignore it and move on
              if(ss.peek() == ':') ss.ignore();
              if(ss.peek() == ',') ss.ignore();
              if(ss.peek() == '-') ss.ignore();

              // Increment the column index
              colIdx++;
          }
      }

      // Close file
      myFile.close();

      if(m_trueRateList.size() != m_automlRateList.size() || m_trueRateList.size() != m_naiveRateList.size())
      {
        NS_FATAL_ERROR("[ERROR] rate trace file:" << m_rateTraceFileName << " size:" << m_trueRateList.size() << "," << m_automlRateList.size() << "," << m_naiveRateList.size());
      }

      //for (uint32_t i = 0; i < m_trueRateList.size(); i++)
      //{
      //  std::cout << "[" << i << "]" << "true:" << m_trueRateList.at (i) << " automl:" << m_automlRateList.at(i) << " naive:" << m_naiveRateList.at(i) << std::endl;
      //}

      UpdateInterval();
    }
}

void
PoissonUdpClient::UpdateInterval (void)
{
  uint64_t indexCount = Now().GetMilliSeconds()/m_traceUpdateInterval.GetMilliSeconds();
  m_interval = Time::FromDouble (
            (m_size * 8) *m_burst/ static_cast<double> (1e6*m_trueRateList.at (indexCount)/m_userNum), Time::S);

  std::cout << Now().GetSeconds() << "indexCout:" << indexCount 
    << " true:" << m_trueRateList.at (indexCount)/m_userNum 
    << " automl:" << m_automlRateList.at(indexCount)/m_userNum 
    << " naive:" << m_naiveRateList.at(indexCount)/m_userNum 
    << " interval:" << m_interval.GetSeconds() << std::endl;
  Simulator::Schedule(m_traceUpdateInterval, &PoissonUdpClient::UpdateInterval, this);
}

void
PoissonUdpClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  ReadTraceFile();
  if (!m_socket)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (InetSocketAddress (Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType(m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (Inet6SocketAddress (Ipv6Address::ConvertFrom(m_peerAddress), m_peerPort));
        }
      else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }

  m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  m_socket->SetAllowBroadcast (true);
  m_sendEvent = Simulator::Schedule (Seconds (0.0), &PoissonUdpClient::Send, this);
}

void
PoissonUdpClient::StopApplication (void)
{
  NS_LOG_FUNCTION (this);
  Simulator::Cancel (m_sendEvent);
}

void
PoissonUdpClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  SeqTsHeader seqTs;
  seqTs.SetSeq (m_sent);
  Ptr<Packet> p = Create<Packet> (m_size-(8+4)); // 8+4 : the size of the seqTs header
  p->AddHeader (seqTs);

  std::stringstream peerAddressStringStream;
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv4Address::ConvertFrom (m_peerAddress);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      peerAddressStringStream << Ipv6Address::ConvertFrom (m_peerAddress);
    }

  if ((m_socket->Send (p)) >= 0)
    {
      ++m_sent;
      m_totalTx += p->GetSize ();
      NS_LOG_INFO ("TraceDelay TX " << m_size << " bytes to "
                                    << peerAddressStringStream.str () << " Uid: "
                                    << p->GetUid () << " Time: "
                                    << (Simulator::Now ()).As (Time::S));

    }
  else
    {
      NS_LOG_INFO ("Error while sending " << m_size << " bytes to "
                                          << peerAddressStringStream.str ());
    }

  if (m_sent < m_count)
    {
      m_burstCounter++;
      if(m_enablePoissonArrival)
      {
        //next time = -1*ln(U)/lamda, where U is the random variable from (0, 1], 1/lamda is arrival interval.
        //we use U = (1 - uniform random variable) since U cannot be 0...

        if (m_burstCounter >= m_burst)
        {
          m_burstCounter = 0;
          double poissonIntervalUs = -1*std::log(1.0 -  m_urv->GetValue ()) * m_interval.GetMicroSeconds();
          //std::cout << "m_interval:" << m_interval.GetMicroSeconds() << "us; poisson interval:" << poissonIntervalUs << " us\n";
          m_sendEvent = Simulator::Schedule (MicroSeconds(poissonIntervalUs), &PoissonUdpClient::Send, this);
        }
        else
        {
          Send();
        }

      }
      else
      {
        if (m_burstCounter >= m_burst)
        {
          m_burstCounter = 0;
          //std::cout << "fix m_interval:" << m_interval.GetMicroSeconds() << "us\n";
          m_sendEvent = Simulator::Schedule (m_interval, &PoissonUdpClient::Send, this);
        }
        else
        {
          Send();
        }
      }
    }
}


uint64_t
PoissonUdpClient::GetTotalTx () const
{
  return m_totalTx;
}


} // Namespace ns3