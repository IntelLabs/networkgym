/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "gma-data-processor.h"
#include <sys/time.h>
#include<unistd.h> 
using json = nlohmann::json;

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GmaDataProcessor");

NS_OBJECT_ENSURE_REGISTERED (GmaDataProcessor);

TypeId
GmaDataProcessor::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GmaDataProcessor")
    .SetParent<Object> ()
    .SetGroupName("networkgym")
    .AddConstructor<GmaDataProcessor> ()
  ;
  return tid;
}

GmaDataProcessor::GmaDataProcessor ()
{
  NS_LOG_FUNCTION (this);
}

GmaDataProcessor::~GmaDataProcessor ()
{
	NS_LOG_FUNCTION (this);
}


void GmaDataProcessor::SaveDlQosMeasurement (uint32_t clientId, double rate, double priority, int cid)
{
  //std::cout << Now().GetSeconds() << " GMA DL | client: " << clientId << " CID: " << cid <<  " rate: "<< rate << " priority:"<< priority << "\n";
  if (m_dlQosMeasure.find(cid) == m_dlQosMeasure.end())
  {
    Ptr<QosMeasure> qosMeasure = Create<QosMeasure>();
    m_dlQosMeasure[cid] = qosMeasure;
  }
  m_dlQosMeasure[cid]->m_clientId.push_back(clientId);
  m_dlQosMeasure[cid]->m_rate.push_back(rate);
  m_dlQosMeasure[cid]->m_qosMarking.push_back(priority);
}

void GmaDataProcessor::SaveUlQosMeasurement (uint32_t clientId, double rate, double priority, int cid)
{
  //std::cout << Now().GetSeconds() << " GMA UL | client: " << clientId << " CID: " << cid <<  " rate: "<< rate << " priority:"<< priority << "\n";
  if (m_ulQosMeasure.find(cid) == m_ulQosMeasure.end())
  {
    Ptr<QosMeasure> qosMeasure = Create<QosMeasure>();
    m_ulQosMeasure[cid] = qosMeasure;
  }
  m_ulQosMeasure[cid]->m_clientId.push_back(clientId);
  m_ulQosMeasure[cid]->m_rate.push_back(rate);
  m_ulQosMeasure[cid]->m_qosMarking.push_back(priority);
}

void
GmaDataProcessor::UpdateCellId(uint32_t clientId, double cellId, std::string cid)
{
  auto cidInt = LinkState::ConvertCidFormat(cid);
  m_clientIdToCellIdMap[std::make_pair(clientId, cidInt)] = cellId;
}

int
GmaDataProcessor::GetCellId(uint32_t clientId, int cid)
{
  if (cid == NETWORK_CID)
  {
    return 1;//gma network. gma only have 1 network.
  }

  if (m_clientIdToCellIdMap.find(std::make_pair(clientId, cid)) == m_clientIdToCellIdMap.end())
  {
    NS_FATAL_ERROR("cannot find cell ID for this client:" << clientId);
  }
  else
  {
    return m_clientIdToCellIdMap[std::make_pair(clientId, cid)];
  }
}

void
GmaDataProcessor::UpdateSliceId(uint32_t clientId, double sliceId)
{
  //std::cout << Now().GetSeconds() << " LTE | client: " << imsi - 1<<" Save Measurements "<< str << ": "<< val << "\n";
  m_clientIdToSliceIdMap[clientId] = int (sliceId);
}

int
GmaDataProcessor::GetSliceId(uint32_t clientId)
{
  int sliceId = -1;
  if (m_clientIdToSliceIdMap.find(clientId) == m_clientIdToSliceIdMap.end())
  {
    //not network slicing, same slice id for all users.
    //NS_FATAL_ERROR("cannot find slice ID for this client:" << clientId);
  }
  else
  {
    sliceId = m_clientIdToSliceIdMap[clientId];
  }
  return sliceId;
}

void
GmaDataProcessor::CalcQosMarkingAction (Ptr<QosMeasure> qosMeasure, int cid, bool dl)
{
  if(qosMeasure->m_rate.size() != qosMeasure->m_qosMarking.size())
  {
    NS_FATAL_ERROR("the size of rate and marking is different.");
  }
  if(qosMeasure->m_rate.size() != qosMeasure->m_clientId.size())
  {
    NS_FATAL_ERROR("the size of rate and client id is different.");
  }


  std::map < std::pair<uint32_t, uint32_t>, Ptr<QosMeasure> > qosMeasurePerSlicePerCell;//separate the measurements according to slices and cells.
  for (uint32_t index = 0; index < qosMeasure->m_clientId.size(); index++)
  {
    uint32_t clientId = qosMeasure->m_clientId.at(index);
    uint32_t sliceId = 0;
    uint32_t cellId = 0;

    if (m_clientIdToSliceIdMap.find(clientId) == m_clientIdToSliceIdMap.end())
    {
      //not network slicing, same slice id for all users.
      //NS_FATAL_ERROR("cannot find slice ID for this client:" << clientId);
    }
    else
    {
      sliceId = m_clientIdToSliceIdMap[clientId];
    }

    if (m_clientIdToCellIdMap.find(std::make_pair(clientId, cid)) == m_clientIdToCellIdMap.end())
    {
      NS_FATAL_ERROR("cannot find cell ID for this client:" << clientId);
    }
    else
    {
      cellId = m_clientIdToCellIdMap[std::make_pair(clientId, cid)];
    }
    //std::cout<< "clientId:" <<clientId << " sliceId:" <<sliceId << " cellId:" << cellId<< " size:" << qosMeasure->m_rate.size() << std::endl;

    auto keyItem = std::make_pair(sliceId, cellId);
    if(qosMeasurePerSlicePerCell.find(keyItem) == qosMeasurePerSlicePerCell.end())
    {
      //create a qos measure for this slice.
      Ptr<QosMeasure> newQosMeasure = Create<QosMeasure>();
      qosMeasurePerSlicePerCell[keyItem] = newQosMeasure;
    }
    
    //copy the measurement to the per slice measurement.
    qosMeasurePerSlicePerCell[keyItem]->m_rate.push_back(qosMeasure->m_rate.at(index));
    qosMeasurePerSlicePerCell[keyItem]->m_qosMarking.push_back(qosMeasure->m_qosMarking.at(index));
    qosMeasurePerSlicePerCell[keyItem]->m_clientId.push_back(qosMeasure->m_clientId.at(index));

  }

  auto iter = qosMeasurePerSlicePerCell.begin();
  while (iter != qosMeasurePerSlicePerCell.end())
  {
    //compute the qos marking within each slice.
    //std::cout<< "slice ID:" << iter->first << " size:" <<iter->second->m_rate.size() << std::endl;
    //for (uint32_t i = 0; i<iter->second->m_clientId.size(); i++)
    //{
    //  std::cout << iter->second->m_clientId.at(i) << " ";
    //}
    //std::cout << std::endl;
    CalcQosMarkingActionPerSlice(iter->second, cid, dl);
    iter++;
  }

}

void
GmaDataProcessor::CalcQosMarkingActionPerSlice (const Ptr<QosMeasure> qosMeasure, int cid, bool dl)
{
  double qosRateHighTarget = 0.9;
  double qosRateLowTarget = 0.7;
  double qosRateMinRatio = qosRateLowTarget; //the minimal qos rate throughput ratio after the qos marking update
  if (qosMeasure->m_rate.size() != qosMeasure->m_qosMarking.size())
  {
    NS_FATAL_ERROR("the size of the qos rate and qos marking is not the same!!!");
  }

  double qosRateSum = 0;
  double nqosRateSum = 0;
  uint32_t listSize = qosMeasure->m_rate.size();
  
  for (uint32_t ind = 0; ind < listSize; ind++)
  {
    if(qosMeasure->m_qosMarking.at (ind) > 0)//qos marking
    {
      qosRateSum += qosMeasure->m_rate.at (ind);
    }
    else
    {
      nqosRateSum += qosMeasure->m_rate.at (ind);
    }

  }

  double qosRateMin = qosRateSum * qosRateMinRatio;

  //std::cout << "CalcQosMarkingAction | CID:" << cid << " DL flag:"<< dl 
  //    << " qos rate sum:" << qosRateSum << " nqos rate sum:" << nqosRateSum 
  //    << " qos High Target:" << qosRateHighTarget<< " qos Low Target:" << qosRateLowTarget<< std::endl;

  //if the ratio of qos rate (qos/(qos+nqos)) is lower than qosRateLowTarget, increase the number of qos users.
  //if the ratio of qos rate (qos/(qos+nqos)) is higher than qosRateHighTarget, decrease the number of qos users.

  //check if we need to change users from non-qos to qos.
  uint32_t forwardId = 0;
  while (qosRateSum < qosRateLowTarget*(qosRateSum + nqosRateSum) && forwardId < listSize) //current qos rate is lower than the low target
  {
    //current rate is lower than the qos rate low target, move some users from non-qos to qos
    if(qosMeasure->m_qosMarking.at (forwardId) < 1)//non-qos
    {
      //change to qos marking.
      //qosRateSum += qosMeasure->m_rate.at (forwardId);
      //nqosRateSum -= qosMeasure->m_rate.at (forwardId);
      qosMeasure->m_qosMarking.at (forwardId) = 1.0;
      break; //now let us only change one user from nQoS to Qos per time.
    }

    forwardId += 1;
  }

  //check if we need to change user from qos to non-qos
  int reverseInd = listSize-1;
  while (qosRateSum > qosRateHighTarget*(qosRateSum + nqosRateSum) && qosRateSum > qosRateMin && reverseInd >= 0)
  {
    //current qos rate is greater than the High target and current qos rate is greater than the min qos rate (after qos marking update) in this interval
    //move some users from qos to nqos

    if(qosMeasure->m_qosMarking.at (reverseInd) > 0)//qos marking
    {
      //change to nqos marking.
      qosRateSum -= qosMeasure->m_rate.at (reverseInd);
      //nqosRateSum += qosMeasure->m_rate.at (reverseInd);
      qosMeasure->m_qosMarking.at (reverseInd) = 0;
    }

    reverseInd -= 1;
  }

  
  //else no action
  //std::cout << "New Action | CID:" << cid << " DL flag:"<< dl << " qos rate sum:" << qosRateSum << " nqos rate sum:" << nqosRateSum << " qos High Target:" << qosRateHighTarget<< " qos Low Target:" << qosRateLowTarget<< std::endl;

  //save the new qos marking to the action map..

  if(dl)
  {
    for (uint32_t ind = 0; ind < listSize; ind++)
    {
      m_idToDlQosMap[std::make_pair(qosMeasure->m_clientId.at(ind), cid)] = qosMeasure->m_qosMarking.at(ind);
    }

    /*std::cout << "DL New Marking [clientID, CID, qosMarking]:";
    auto iter = m_idToDlQosMap.begin();
    while(iter != m_idToDlQosMap.end())
    {
      std::cout<< "["<< iter->first.first << ", " << iter->first.second << ", " << iter->second << "]";
      iter++;
    }
    std::cout << std::endl;*/
  }
  else
  {
    for (uint32_t ind = 0; ind < listSize; ind++)
    {
      m_idToUlQosMap[std::make_pair(qosMeasure->m_clientId.at(ind), cid)] = qosMeasure->m_qosMarking.at(ind);
    }

    /*std::cout << "UL New Marking [clientID, CID, qosMarking]:";
    auto iter = m_idToUlQosMap.begin();
    while(iter != m_idToUlQosMap.end())
    {
      std::cout<< "["<< iter->first.first << ", " << iter->first.second << ", " << iter->second << "]";
      iter++;
    }
    std::cout << std::endl;*/
  }

}

void
GmaDataProcessor::GetNoneAiAction(json& action)
{
  //std::cout << "Action received. Do something here before firing the action callbacks." << std::endl;
  //std::cout <<"Send Measurement end_ts:" << m_gmaLastEndTs << std::endl;

  //std::cout << "DL qos rate measurement" << std::endl;
  auto iter = m_dlQosMeasure.begin();

  //we perform dynamic flow prioritization here...
  //remove all existing mapping
  m_idToDlQosMap.clear();
  m_idToUlQosMap.clear();
  //we will recompute them in the CalcQosMarkingAction function.
  
  while (iter!= m_dlQosMeasure.end())
  {
    /*std::cout << "CID: "<<iter->first << " Rate: ";
    for (auto i: iter->second->m_rate)
      std::cout << i << ' ';
    std::cout<<std::endl;

    std::cout << "CID: "<<iter->first << " Marking: ";
    for (auto i: iter->second->m_qosMarking)
      std::cout << i << ' ';
    std::cout<<std::endl;

    std::cout << "CID: "<<iter->first << " Client ID: ";
    for (auto i: iter->second->m_clientId)
      std::cout << i << ' ';
    std::cout<<std::endl;*/

    CalcQosMarkingAction(iter->second, iter->first, true);

    iter++;
  }

  //std::cout << "UL qos rate measurement" << std::endl;
  iter = m_ulQosMeasure.begin();
  while (iter!= m_ulQosMeasure.end())
  {
    /*std::cout << "Rate CID: "<<iter->first << " | ";
    for (auto i: iter->second->m_rate)
      std::cout << i << ' ';
    std::cout<<std::endl;

    std::cout << "Marking CID: "<<iter->first << " | ";
    for (auto i: iter->second->m_qosMarking)
      std::cout << i << ' ';
    std::cout<<std::endl;

    std::cout << "CID: "<<iter->first << " Client ID: ";
    for (auto i: iter->second->m_clientId)
      std::cout << i << ' ';
    std::cout<<std::endl;*/

    CalcQosMarkingAction(iter->second, iter->first, false);

    iter++;
  }


  //compute the qos rate and non qos rate. We could directly compute the number for user to be transfered to qos/nqos, such that the qos/total_rate target will be meet.
  //no need to use the AIMD algorithm !!!!

  if(action["action_list"].is_array())
  {
    auto iterQos = m_dlQosMeasure.begin();
    while (iterQos != m_dlQosMeasure.end())
    {
      //std::cout << +iterQos->first << " " << LinkState::ConvertCidFormat(iterQos->first) << std::endl;
      auto newAction = action["action_list"][0];
      newAction["source"] = "gma";
      newAction["name"] = LinkState::ConvertCidFormat(iterQos->first) + "::dl::priority";
      newAction["id"] = m_idList;
      newAction["value"].clear();
      for (uint32_t it = 0; it < newAction["id"].size(); it++)
      {
        auto iter = m_idToDlQosMap.find(std::make_pair(newAction["id"][it], iterQos->first));
        if(iter == m_idToDlQosMap.end())
        {
          NS_FATAL_ERROR("No traffic found for this link!");
        }
        else
        {
          newAction["value"][it] = iter->second;
        }
      }
      action["action_list"].push_back(newAction);

      iterQos++;
    }
  }
  else
  {
    //not an array. This is one action list.
    json elementList = {};
    elementList.push_back(action["action_list"]); //store the old action

    auto iterQos = m_dlQosMeasure.begin();
    while (iterQos != m_dlQosMeasure.end())
    {
      //std::cout << +iterQos->first << " " << LinkState::ConvertCidFormat(iterQos->first) << std::endl;
      auto newAction = action["action_list"];
      newAction["source"] = "gma";
      newAction["name"] = LinkState::ConvertCidFormat(iterQos->first) + "::dl::priority";
      newAction["id"] = m_idList;
      newAction["value"].clear();
      for (uint32_t it = 0; it < newAction["id"].size(); it++)
      {
        auto iter = m_idToDlQosMap.find(std::make_pair(newAction["id"][it], iterQos->first));
        if(iter == m_idToDlQosMap.end())
        {
          NS_FATAL_ERROR("No traffic found for this link!");
        }
        else
        {
          newAction["value"][it] = iter->second;
        }
      }
      elementList.push_back(newAction);
      iterQos++;
    }
    //std::cout << "elementList:" << elementList << std::endl;
    action["action_list"] = elementList;
    
  }
  //std::cout << "with none ai action:" << action << std::endl;
  m_dlQosMeasure.clear();
  m_ulQosMeasure.clear();
}

void
GmaDataProcessor::AppendSliceMeasurement(Ptr<NetworkStats> measurement, int cid, bool average)
{
  if(!m_measurementStarted)
  {
    return;
  }
  
  //std::cout << measurement->GetJson() << std::endl;
  json measurementJson = measurement->GetJson();
  for(auto it = measurementJson.begin(); it != measurementJson.end(); ++it)
  {
    auto sourceAndName = (*it)["source"].get<std::string>() + "::"+ (*it)["name"].get<std::string>();
    if (std::find(m_subscribedMeasurement.begin(), m_subscribedMeasurement.end(),sourceAndName)==m_subscribedMeasurement.end())
    {
      //not in suscribe list; continue to the next measurement
      continue;
    }

    if(average)
    {
      //if average is enabled, the measurement is divided by the number of clients in the same cell and same slice.
      int clientId = (*it)["id"].at(0).get<int>();
      auto cellId = GetCellId(clientId, cid);
      auto sliceId = GetSliceId(clientId);

      auto iter = m_clientIdToSliceIdMap.begin();
      int clientCounter = 0;
      while(iter != m_clientIdToSliceIdMap.end())
      {
        if ((int)(*iter).second == sliceId)
        {
          //also check if they are in the same cell!
          if (GetCellId((*iter).first, cid) == cellId)
          {
            clientCounter += 1;
          }
        }
        iter++;
      }

      if(clientCounter == 0)
      {
        NS_FATAL_ERROR("a slice cannot have 0 client!!!");
      }

      //std::cout << " client: " << clientId << " cell: " << cellId << " slice: " << sliceId << " num: " << clientCounter << std::endl;
      (*it)["value"].at(0) = (*it)["value"].at(0).get<double>()/clientCounter; //divide the value by the number of user per slice.
    }

    if (m_moreMeasurement.size() == 0)
    {
      //this measurement does not exit in the more measurement, add it to the more measurement.
      int clientId = (*it)["id"].at(0).get<int>();
      auto cellId = GetCellId(clientId, cid);
      auto sliceId = GetSliceId(clientId);
      //std::cout << "id:" << clientId << " cellId:" << cellId << " sliceId:" << sliceId << std::endl;

      json newMeasurement;
      newMeasurement["source"] = (*it)["source"];
      newMeasurement["id"].push_back(cellId);
      newMeasurement["ts"] = (*it)["ts"];
      newMeasurement["name"] = (*it)["name"];
      json item;
      item["slice"] = std::vector<int>{sliceId};
      item["value"] = (*it)["value"];

      newMeasurement["value"].push_back(item);
      m_moreMeasurement.push_back(newMeasurement);
    }
    else
    {
      bool findName = false;
      for (uint32_t ind = 0; ind < m_moreMeasurement.size(); ind++)
      {
        if ((*it)["source"] == m_moreMeasurement.at(ind)["source"] && (*it)["name"] == m_moreMeasurement.at(ind)["name"])
        {
          //find the measurement in moreMeasurement vector.
          findName = true;
          if ((*it)["ts"] != m_moreMeasurement.at(ind)["ts"])
          {
            NS_FATAL_ERROR("timestamp not the same!!!");
          }
          //this measurement exist in the more measurement object.
          int clientId = (*it)["id"].at(0).get<int>();
          auto cellId = GetCellId(clientId, cid);
          auto sliceId = GetSliceId(clientId);
          if (cellId == m_moreMeasurement.at(ind)["id"].at(0))
          {
            //same cellid
            bool findSlice = false;
            for (uint32_t sId = 0; sId < m_moreMeasurement.at(ind)["value"].at(0)["slice"].size(); sId++)
            {
              if (m_moreMeasurement.at(ind)["value"].at(0)["slice"].at(sId) == sliceId)
              {
                //same cellid and same slice id
                //std::cout << "same cell id: " << cellId << " same slice id: " << sliceId << " -> add to value." << std::endl;
                m_moreMeasurement.at(ind)["value"].at(0)["value"].at(sId) = m_moreMeasurement.at(ind)["value"].at(0)["value"].at(sId).get<double>() + (*it)["value"].at(0).get<double>();
                findSlice = true;
              }
            }

            if (findSlice == false)
            {
              //same cellid but different slice id
              //std::cout << "same cell id: " << cellId << " no slice id -> create a new slice." << std::endl;
              m_moreMeasurement.at(ind)["value"].at(0)["slice"].push_back(sliceId);
              m_moreMeasurement.at(ind)["value"].at(0)["value"].push_back((*it)["value"].at(0).get<double>());
            }
          }
          else
          {
            //not the same cell id...create a new entry
            m_moreMeasurement.at(ind)["id"].push_back(cellId);
            m_moreMeasurement.at(ind)["value"].push_back((*it)["value"].at(0));
          }

        }
        
      }
      if (findName == false)
      {
        //this measurement does not exit in the more measurement, add it to the more measurement.
        int clientId = (*it)["id"].at(0).get<int>();
        auto cellId = GetCellId(clientId, cid);
        auto sliceId = GetSliceId(clientId);

        json newMeasurement;
        newMeasurement["source"] = (*it)["source"];
        newMeasurement["id"].push_back(cellId);
        newMeasurement["ts"] = (*it)["ts"];
        newMeasurement["name"] = (*it)["name"];
        json item;
        item["slice"] = std::vector<int>{sliceId};
        item["value"] = (*it)["value"];

        newMeasurement["value"].push_back(item);
        m_moreMeasurement.push_back(newMeasurement);
      } 
    }
  
  }

}

void
GmaDataProcessor::AddMoreMeasurement()
{
  //std::cout << " Add more measurement here" << std::endl;
  json subJson;
  for (uint32_t ind = 0; ind < m_moreMeasurement.size(); ind++)
  {
    subJson.push_back(m_moreMeasurement.at(ind));
  }

  m_measurementBatch.push_back(subJson);
  m_moreMeasurement.clear();
}

void
GmaDataProcessor::AddClientId (uint32_t clientId)
{
  m_idList.push_back(clientId);
}

}
