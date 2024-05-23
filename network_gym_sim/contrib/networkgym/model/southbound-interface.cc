/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "southbound-interface.h"
#include <sys/time.h>
#include <unistd.h>
#include <chrono>
using json = nlohmann::json;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SouthboundInterface");

NS_OBJECT_ENSURE_REGISTERED (SouthboundInterface);

TypeId
SouthboundInterface::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SouthboundInterface")
    .SetParent<Object> ()
    .SetGroupName("networkgym")
    .AddConstructor<SouthboundInterface> ()
    .AddAttribute ("MaxActionWaitTime",
                "Max Waiting time (ms) for an action, -1 stands for forever",
                IntegerValue (600000),
                MakeIntegerAccessor (&SouthboundInterface::m_maxActionWaitTime),
                MakeIntegerChecker<int> ())
  ;
  return tid;
}

SouthboundInterface::SouthboundInterface ()
{
  NS_LOG_FUNCTION (this);
  Connect();
}

SouthboundInterface::~SouthboundInterface ()
{
	NS_LOG_FUNCTION (this);
}

void
SouthboundInterface::DoDispose (void)
{
  zmq_close (m_zmq_socket);
  zmq_ctx_destroy (m_zmq_context);
  std::cout  << m_workerName << ": ns3 disconnected from NetworkGym." << std::endl;
}


void
SouthboundInterface::Connect()
{
  std::ifstream jsonStream("gym-configure.json");
  json jsonConfig;
  jsonStream >> jsonConfig;

  m_workerName = jsonConfig["env_identity"].get<std::string>();

  m_clientIdentity = jsonConfig["client_identity"].get<std::string>();

  std::string plain_username = jsonConfig["session_name"].get<std::string>();
  std::string plain_password = jsonConfig["session_key"].get<std::string>();
  int portN = jsonConfig["env_port"].get<int>();
  std::cout << m_workerName << ": ns3 connecting to NetworkGym." << std::endl;
  m_zmq_context = zmq_ctx_new ();
  m_zmq_socket = zmq_socket (m_zmq_context, ZMQ_DEALER);
  zmq_setsockopt (m_zmq_socket, ZMQ_PLAIN_USERNAME, plain_username.c_str(), plain_username.size());
  zmq_setsockopt (m_zmq_socket, ZMQ_PLAIN_PASSWORD, plain_password.c_str(), plain_password.size());
  zmq_setsockopt (m_zmq_socket, ZMQ_IDENTITY, m_workerName.c_str(), m_workerName.size());
  int64_t linger = 10000;
  zmq_setsockopt (m_zmq_socket, ZMQ_LINGER, &linger, sizeof linger);
  std::string addrAndPort = "tcp://localhost:"+std::to_string(portN);
  zmq_connect (m_zmq_socket, addrAndPort.c_str());

}

void
SouthboundInterface::SendMeasurementJson(json& networkStats, json& workloadStats)
{
  json measurementReport = {};
  measurementReport["type"] = "env-measurement";

  measurementReport["network_stats"] = networkStats;
  measurementReport["workload_stats"] = workloadStats;
  std::string j_str =measurementReport.dump();
  zmq_send (m_zmq_socket, m_clientIdentity.c_str(), m_clientIdentity.size(), ZMQ_SNDMORE);
  zmq_send (m_zmq_socket, j_str.c_str(), j_str.size(), 0);
}

void
SouthboundInterface::SendMeasurementJson(json& networkStats)
{
  json measurementReport = {};
  measurementReport["type"] = "env-measurement";

  measurementReport["network_stats"] = networkStats;
  std::string j_str =measurementReport.dump();
  zmq_send (m_zmq_socket, m_clientIdentity.c_str(), m_clientIdentity.size(), ZMQ_SNDMORE);
  zmq_send (m_zmq_socket, j_str.c_str(), j_str.size(), 0);
}

void
SouthboundInterface::GetAction(json& action, bool raiseError)
{

  /* Poll for events for m_maxActionWaitTime */
  zmq_pollitem_t items [] = {
      { m_zmq_socket,   0, ZMQ_POLLIN, 0 },
  };
  //pull timeout = m_maxActionWaitTime, 0 measn return rightway, -1 means wait forever...
  int rc = zmq_poll (items, 1, m_maxActionWaitTime);

  if (rc == 0 && raiseError)
  {
    //For new, we exit ns3 after action timeout. We can comment this out when we support realtime action.
    NS_FATAL_ERROR("Action Waiting Timeout. Exit!");
  }

  assert (rc >= 0); /* Returned events will be stored in items[].revents */
  json j_msg;
  while(rc > 0)//while there is a msg in the socket, we get the last one!
  {
    //the server sends two msgs: (1) algorithm client indentiy and followed by the (2) msg.

    //(1) RX identity
    char identity [1024];
    int size = zmq_recv (m_zmq_socket, identity, 1023, 0);
    if (size == -1 || size > 1023)
    {
      NS_FATAL_ERROR("Receive ERROR");
    }
    identity [size] = '\0';
    if (m_clientIdentity != identity)
    {
      NS_FATAL_ERROR("client identity changed! from " << m_clientIdentity << " to " << identity);
    }
    //std::cout << "Received Identity: "<< m_clientIdentity << std::endl;

    //(2) RX action msg
    char buffer [10001];
    size = zmq_recv (m_zmq_socket, buffer, 10000, 0);
    if (size == -1 || size > 10000)
    {
      NS_FATAL_ERROR("Receive ERROR");
    }
    buffer [size] = '\0';
    //std::cout << "Received: "<< buffer << std::endl;

    action = json::parse(buffer);
    if(action["type"].get<std::string>().compare("env-action") != 0 )
    {
      NS_FATAL_ERROR("Unkown MSG, the client should only receive env-action, but received :" << action["type"].get<std::string>());
    }

    //this is the action we are expecting...
    std::cout << Now().GetSeconds() << " NetworkGym Southbound RX [env-action]" << std::endl;
    zmq_pollitem_t items [] = {
          { m_zmq_socket,   0, ZMQ_POLLIN, 0 },
      };
    rc = zmq_poll (items, 1, 0);
    assert (rc >= 0); /* Returned events will be stored in items[].revents */
  }

}


}
