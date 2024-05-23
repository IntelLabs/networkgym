/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GMA_DATA_PROCESSOR_H
#define GMA_DATA_PROCESSOR_H

#include <zmq.hpp>
#include "ns3/core-module.h"
#include "ns3/networkgym-module.h"
#include "link-state.h"
using json = nlohmann::json;
namespace ns3 {

class GmaDataProcessor : public DataProcessor
{
public:
  GmaDataProcessor ();
	/*
  This function setup a tunnel between server and client. The server with physical address serverAddr, client with two phsical address clientAddr and clientAddrSec.
  The function will encapulate the packets forwarding to the virtual address 7.0.1.2, and route these packets over either clientAddr or clientAddrSec.
  TODO: split the sender and receiver functionality into 2 sub-classes.
  */
  virtual ~GmaDataProcessor ();
  static TypeId GetTypeId (void);
  void SaveDlQosMeasurement (uint32_t clientId, double rate, double priority, int cid);
  void SaveUlQosMeasurement (uint32_t clientId, double rate, double priority, int cid);
  void UpdateCellId(uint32_t clientId, double cellId, std::string cid);
  int GetCellId(uint32_t clientId, int cid);
  void UpdateSliceId(uint32_t clientId, double sliceId);
  int GetSliceId(uint32_t clientId);
  void AppendSliceMeasurement(Ptr<NetworkStats> measurement, int cid = NETWORK_CID, bool average = false); //cid = -1 stands for all inks in the network
  void AddClientId (uint32_t clientId);
private:
  virtual void GetNoneAiAction(json& action);
  virtual void AddMoreMeasurement();
  struct QosMeasure : public SimpleRefCount<QosMeasure>
  {
    std::vector<double> m_rate;
    std::vector<double> m_qosMarking;
    std::vector<int> m_clientId;
  };
  
  void CalcQosMarkingAction (Ptr<QosMeasure> qosMeasure, int cid, bool dl); //this function should be moved to somewhere...
  void CalcQosMarkingActionPerSlice (Ptr<QosMeasure> qosMeasure, int cid, bool dl); //this function should be moved to somewhere...

  std::map<std::pair<uint32_t, int>, int> m_idToDlQosMap;
  std::map<std::pair<uint32_t, int>, int> m_idToUlQosMap;


  std::map< int, Ptr<QosMeasure> > m_dlQosMeasure; //key is the cid of the link
  std::map< int, Ptr<QosMeasure> > m_ulQosMeasure; //key is the cid of the link

  std::map<uint32_t, uint32_t> m_clientIdToSliceIdMap;
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> m_clientIdToCellIdMap; //key is the client and cid

  std::vector<json> m_moreMeasurement;
  json m_idList;
};

}

#endif /* GMA_DATA_PROCESSOR_H */

