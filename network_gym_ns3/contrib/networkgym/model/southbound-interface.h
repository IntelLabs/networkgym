/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef SOUTHBOUND_INTERFACE_H
#define SOUTHBOUND_INTERFACE_H

#include <zmq.hpp>
#include "ns3/core-module.h"
#include "json.hpp"

using json = nlohmann::json;
namespace ns3 {

class SouthboundInterface : public Object
{
public:
  SouthboundInterface ();
	/*
  Connect the ns-3 to networkgym server.
  */
  virtual ~SouthboundInterface ();
  virtual void DoDispose (void);

  static TypeId GetTypeId (void);
  void SendMeasurementJson (json& networkStats, json& workloadStats); //network stats and workload stats measurement
  void SendMeasurementJson (json& networkStats); //network stats measurement
  void GetAction (json& action, bool raiseError); //if raiseError = true, the program exits with error when the action is not received after poll timeout.

private:
  void Connect();
  int m_maxActionWaitTime; //unit ms

  void *m_zmq_context;
  void *m_zmq_socket;
  std::string m_workerName;
  std::string m_clientIdentity;

};

}

#endif /* SOUTHBOUND_INTERFACE_H */

