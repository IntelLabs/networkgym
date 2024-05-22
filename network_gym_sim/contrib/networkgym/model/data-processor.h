/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef DATA_HANDLER_H
#define DATA_HANDLER_H

#include "ns3/core-module.h"
#include "json.hpp"
#include "ns3/southbound-interface.h"
using json = nlohmann::json;
namespace ns3 {
class NetworkStats : public Object
{
public:
  NetworkStats (std::string source, uint64_t id, uint64_t ts);
  virtual ~NetworkStats ();

  void Append(std::string name, double value);//append a double measurement.
  void Append(std::string name, json& value);//append a json measurement.
  void Append(std::string name, std::string indexName, std::vector<int> indexList, std::vector<double> list);//append a list of double measurement

  json GetJson();
private:
  std::string m_source;
  uint64_t m_id;
  uint64_t m_ts;
  json m_data;
};

class DataProcessor : public Object
{
public:
  DataProcessor ();
	/*
  Connect the ns-3 to networkgym server.
  */
  virtual ~DataProcessor ();
  virtual void DoDispose ();
  static TypeId GetTypeId (void);
  void StartMeasurement ();
  bool IsMeasurementStarted ();
  void AppendMeasurement(Ptr<NetworkStats> measurement);//the measurements appended from multiple sources at the same time will be aggregated and sent after 1 nanosecond.
  typedef Callback<void, const json& > NetworkGymActionCallback;
  void SetNetworkGymActionCallback(std::string name, uint64_t id, NetworkGymActionCallback cb);
  void SetMaxPollTime (int timeMs);
protected:
  Ptr<SouthboundInterface> m_southbound;
  bool m_measurementStarted = false;
  std::vector<json> m_measurementBatch;
  std::vector<std::string> m_subscribedMeasurement; //store the measurement list.

private:
  void ExchangeMeasurementAndAction(); //send measurement and get action.
  virtual void AddMoreMeasurement();
  virtual void GetNoneAiAction(json& action);
  EventId m_exchangeMeasurementAndActionEvent;
  std::map< std::pair< std::string, uint64_t>, NetworkGymActionCallback> m_networkgymActionCallbackMap; //callback that send action to the connected modules. Multiple modules may connects to it. key is the action name

  uint64_t m_waitCounter;
  uint64_t m_startSysTimeMs;
  uint64_t m_waitSysTimeMs;

  uint64_t m_totalSteps;
  double m_measurementSentTsMs;
};

}

#endif /* DATA_HANDLER_H */

