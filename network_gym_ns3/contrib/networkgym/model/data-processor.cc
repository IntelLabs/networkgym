/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "data-processor.h"
#include <sys/time.h>
#include <unistd.h>
#include <chrono>
using json = nlohmann::json;

namespace ns3 {


NetworkStats::NetworkStats (std::string source, uint64_t id, uint64_t ts)
{
  m_source = source;
  m_id = id;
  m_ts = ts;
}
NetworkStats::~NetworkStats ()
{

}

json
NetworkStats::GetJson()
{
  return m_data;
}

void
NetworkStats::Append(std::string name, double value)
{
    json measurement;
    measurement["source"] = m_source;
    measurement["id"].push_back(m_id);
    measurement["ts"] = m_ts;
    measurement["name"] = name;
    measurement["value"].push_back(value);
    m_data.push_back(measurement);
}

void
NetworkStats::Append(std::string name, json& value)
{
    json measurement;
    measurement["source"] = m_source;
    measurement["id"].push_back(m_id);
    measurement["ts"] = m_ts;
    measurement["name"] = name;
    measurement["value"].push_back(value);
    m_data.push_back(measurement);
}

void
NetworkStats::Append(std::string name, std::string indexName, std::vector<int> indexList, std::vector<double> list)
{
    if(indexList.size() != list.size())
    {
      NS_FATAL_ERROR("The size of the indexName and list is not the same!!!");
    }
    json measurement;
    measurement["source"] = m_source;
    measurement["id"].push_back(m_id);
    measurement["ts"] = m_ts;
    measurement["name"] = name;
    json item;
    item[indexName] = indexList;
    item["value"] = list;

    measurement["value"].push_back(item);

    m_data.push_back(measurement);
}

NS_LOG_COMPONENT_DEFINE ("DataProcessor");

NS_OBJECT_ENSURE_REGISTERED (DataProcessor);

TypeId
DataProcessor::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DataProcessor")
    .SetParent<Object> ()
    .SetGroupName("networkgym")
    .AddConstructor<DataProcessor> ()
  ;
  return tid;
}

DataProcessor::DataProcessor ()
{
  NS_LOG_FUNCTION (this);
  m_southbound = CreateObject<SouthboundInterface>();
  m_waitCounter = 0;
  m_waitSysTimeMs = 0;
  m_startSysTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  //std::cout << "ns3 starts at :"<< m_startSysTimeMs << " milliseconds since the Epoch\n";

  std::ifstream jsonStreamEnv("env-configure.json");
  json jsonConfigEnv;
  jsonStreamEnv >> jsonConfigEnv;
  m_totalSteps = jsonConfigEnv["steps_per_episode"].get<uint64_t>() * jsonConfigEnv["episodes_per_session"].get<uint64_t>();
  uint32_t mSize = jsonConfigEnv["subscribed_network_stats"].size();
  for (uint32_t i = 0; i < mSize; i++)
  {
    //std::cout << jsonConfigEnv["subscribed_network_stats"].at(i) << std::endl;
    m_subscribedMeasurement.push_back(jsonConfigEnv["subscribed_network_stats"].at(i));
  }
}

DataProcessor::~DataProcessor ()
{
	NS_LOG_FUNCTION (this);
}

void
DataProcessor::DoDispose()
{
  uint64_t endSysTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  //std::cout<<"ns3 end at :"<< endSysTime << " milliseconds since the Epoch\n";
  uint64_t timeLapse = endSysTime - m_startSysTimeMs;
  std::cout<<"Time Lapse :"<< timeLapse << " milliseconds\n";
  if (timeLapse > 0)
  {
    uint64_t simTime = timeLapse - m_waitSysTimeMs;
    std::cout<<"ns3 Sim time :"<<  simTime << " milliseconds. (" << simTime*100/timeLapse <<"%)\n";
  }
  m_southbound->Dispose();
}

void
DataProcessor::AppendMeasurement(Ptr<NetworkStats> measurement)
{
  if(!m_measurementStarted)
  {
    return;
  }
  
  Time maxWaitTime = NanoSeconds(1);
  //std::cout << measurement->GetJson() << std::endl;
  //only keep the measurement in the subscribed list
  json measurementJson = measurement->GetJson();
  json subJson;
  for(auto it = measurementJson.begin(); it != measurementJson.end(); ++it)
  {
      auto sourceAndName = (*it)["source"].get<std::string>() + "::"+ (*it)["name"].get<std::string>();
      //std::cout<< sourceAndName << std::endl;
      if (std::find(m_subscribedMeasurement.begin(), m_subscribedMeasurement.end(),sourceAndName)!=m_subscribedMeasurement.end())
      {
        //std::cout << " FIND IT !" << std::endl;
        subJson.push_back(*it);
      }
  }
  m_measurementBatch.push_back(subJson);

  //TODO: for multi-agent case, we should not use the delayed schedule event. we send the measurement right away.
  if (m_exchangeMeasurementAndActionEvent.IsExpired())
  {
    m_exchangeMeasurementAndActionEvent = Simulator::Schedule(maxWaitTime, &DataProcessor::ExchangeMeasurementAndAction, this);
  }
}

void
DataProcessor::ExchangeMeasurementAndAction()
{
  if(!m_measurementStarted)
  {
    return;
  }

  if (m_measurementBatch.size() == 0)
  {
    return;
  }

  AddMoreMeasurement();
  json networkStats = m_measurementBatch.at(0); //networkStats is the json based measurement
  std::vector<std::string> nameList; //namelist stores the source::name list of the measurement
  for(auto it = networkStats.begin(); it != networkStats.end(); ++it)
  {
    auto sourceAndName = (*it)["source"].get<std::string>() + "::"+ (*it)["name"].get<std::string>();
    if (nameList.empty())
    {
      nameList.push_back(sourceAndName);
    }
    else
    {
      if (find(nameList.begin(), nameList.end(), sourceAndName) == nameList.end())
      {
        nameList.push_back(sourceAndName);
      }
    }
  }

  //build nested json
  for (uint32_t ind = 1; ind < m_measurementBatch.size(); ind++)
  {
    //loop over all measurement in this batch.
    //std::cout << networkStats << std::endl;
    //std::cout << nameList.size() << std::endl;

    for(auto j = m_measurementBatch.at(ind).begin(); j != m_measurementBatch.at(ind).end(); ++j)
    {
      //for each measurement entry, it may contain multiple metrics, loop them all.
      auto it = networkStats.begin();
      while(it != networkStats.end())
      {
        //for each network stats, loop all metrics
        //for measurement from different group...
        //we need to append measurement at the outer loop...
        //std::cout << "out : "<< (*j) << std::endl;
        //auto sourceAndName = (*it)["source"].get<std::string>() + "::"+ (*it)["name"].get<std::string>();
        //std::cout << "in : "<< (*it) << std::endl;

        if ( (*it)["source"] == (*j)["source"] && (*it)["name"] == (*j)["name"])
        {
          //same source and name --> same measurement... append to the id and value list
          if ((*it)["ts"] != (*j)["ts"])
          {
            NS_FATAL_ERROR("the timestamp of two measurements are different!");
          }

          //save the the list in a sorted order.
          uint32_t index = 0;
          while (index <= (*it)["id"].size())
          {
            //the new element is greater than all existing element, push it into the back.
            if (index == (*it)["id"].size())
            {
              (*it)["id"].push_back((*j)["id"].at(0));
              (*it)["value"].push_back((*j)["value"].at(0));
              break;
            }

            //put value in a sorted way...
            if ((*it)["id"].at(index) > (*j)["id"].at(0))
            {
              (*it)["id"].insert((*it)["id"].begin()+index, (*j)["id"].at(0));
              (*it)["value"].insert((*it)["value"].begin()+index, (*j)["value"].at(0));
              break;
            }
            index++;
          }

          break;
        }
        else
        {
          //check if we need to append to network stats.
          auto sourceAndNameTemp = (*j)["source"].get<std::string>() + "::"+ (*j)["name"].get<std::string>();
          if (nameList.empty())
          {
            nameList.push_back(sourceAndNameTemp);
            networkStats.push_back((*j));
            break;
          }
          else
          {
            if (find(nameList.begin(), nameList.end(), sourceAndNameTemp) == nameList.end())
            {
              nameList.push_back(sourceAndNameTemp);
              networkStats.push_back(*j);
              break;

            }
          }
        }
        it++;
      }
    }

  }
  m_measurementSentTsMs = Now().GetMilliSeconds();
  std::cout << Now().GetSeconds() << " NetworkGym Southbound Send Measurement"<< std::endl;
  //std::cout << networkStats << std::endl;
  uint64_t currentSysTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  uint64_t timeLapse = currentSysTime - m_startSysTimeMs;
  uint64_t simTimeLaps = timeLapse - m_waitSysTimeMs;

  //add workload measurement.
  json element;
  element["total_ms"] = timeLapse;
  element["sim_ms"] = simTimeLaps;
  if(timeLapse > 0)
  {
    element["sim_time_%"] = 100*simTimeLaps/timeLapse;
  }

  element["pause_ms"] = m_waitSysTimeMs;
  if(m_waitCounter > 0)
  {
    element["pause_ms_per_step"] = m_waitSysTimeMs/m_waitCounter;
  }
 
  json workloadStats;
  workloadStats["time_lapse"].push_back(element);

  m_southbound->SendMeasurementJson(networkStats, workloadStats);
  m_measurementBatch.clear();

  if (m_waitCounter+1 >= m_totalSteps)
  {
    //the first step is the reset function which does not need an action. therefore we +1.
    m_measurementStarted = false; //simulated the max number of steps. stop sending measurement and receive actions.
    return;
  }

  //std::cout << m_waitCounter << " total: " << m_totalSteps << std::endl;
  uint64_t beforePollMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

  json action;
  m_southbound->GetAction(action, true);
  GetNoneAiAction(action);
  uint64_t afterPollMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
  //compute the time ns3 waits for action.
  m_waitSysTimeMs += afterPollMs - beforePollMs;
  m_waitCounter += 1;

  //send the action to subscribed module.
  std::cout << action["action_list"] << " is_array:" << action["action_list"].is_array()<< std::endl;
  //send action to the connected callback. The key is the measurement <source::name, id>.
  if(action["action_list"].is_array())
  {
    for (uint32_t i = 0; i < action["action_list"].size(); i++)
    {
      auto action_name = action["action_list"][i]["source"].get<std::string>() +"::"+ action["action_list"][i]["name"].get<std::string>();
      
      if (action["action_list"][i]["value"].is_array())
      {
        for (uint32_t it = 0; it < action["action_list"][i]["id"].size(); it++)
        {
          auto key = std::make_pair(action_name, action["action_list"][i]["id"][it]);
          //std::cout << action["action_list"][i]["value"] << std::endl;
          if(m_networkgymActionCallbackMap.find(key) == m_networkgymActionCallbackMap.end())
          {
            NS_FATAL_ERROR("callback does not exits for the action_name: "<< key.first << " and id:" << key.second);
          }
          if (m_measurementSentTsMs != action["action_list"][i]["ts"])
          {
            NS_FATAL_ERROR("the action ts:"<< action["action_list"][i]["ts"] <<" does not equal the measurement ts:" << m_measurementSentTsMs);
          }
          m_networkgymActionCallbackMap[key](action["action_list"][i]["value"][it]);
        }
      }
      else
      {
        if (m_measurementSentTsMs != action["action_list"][i]["ts"])
        {
          NS_FATAL_ERROR("the action ts:"<< action["action_list"][i]["ts"] <<" does not equal the measurement ts:" << m_measurementSentTsMs);
        }
        auto key = std::make_pair(action_name, action["action_list"][i]["id"]);
        m_networkgymActionCallbackMap[key](action["action_list"][i]["value"]);
      }

    }
  }
  else
  {
    //not an array. This is one action list.
    auto action_name = action["action_list"]["source"].get<std::string>() +"::"+action["action_list"]["name"].get<std::string>();
    if (action["action_list"]["value"].is_array())
    {
      for (uint32_t it = 0; it < action["action_list"]["id"].size(); it++)
      {
        if (m_measurementSentTsMs != action["action_list"]["ts"])
        {
          NS_FATAL_ERROR("the action ts:"<< action["action_list"]["ts"] <<" does not equal the measurement ts:" << m_measurementSentTsMs);
        }
        auto key = std::make_pair(action_name, action["action_list"]["id"][it]);
        if(m_networkgymActionCallbackMap.find(key) == m_networkgymActionCallbackMap.end())
        {
          NS_FATAL_ERROR("callback does not exits for the action_name: "<< key.first << " and id:" << key.second);
        }
        m_networkgymActionCallbackMap[key](action["action_list"]["value"][it]);
      }
    }
    else
    {
      if (m_measurementSentTsMs != action["action_list"]["ts"])
      {
        NS_FATAL_ERROR("the action ts:"<< action["action_list"]["ts"] <<" does not equal the measurement ts:" << m_measurementSentTsMs);
      }
      auto key = std::make_pair(action_name, action["action_list"]["id"]);
      m_networkgymActionCallbackMap[key](action["action_list"]["value"]);
    }

  }
}

void
DataProcessor::SetNetworkGymActionCallback(std::string name, uint64_t id, NetworkGymActionCallback cb)
{
  auto key = std::make_pair(name, id);
  if(m_networkgymActionCallbackMap.find(key) != m_networkgymActionCallbackMap.end())
  {
    NS_FATAL_ERROR("The callback with the same name and id already exists!");
  }
  m_networkgymActionCallbackMap[key] = cb;
}

void
DataProcessor::StartMeasurement ()
{
  m_measurementStarted = true;
}

bool
DataProcessor::IsMeasurementStarted ()
{
  return m_measurementStarted;
}

void
DataProcessor::AddMoreMeasurement()
{
  //overwrite by others.
}


void
DataProcessor::GetNoneAiAction(json& action)
{
  //overwrite by others.
}

void
DataProcessor::SetMaxPollTime (int timeMs)
{
  m_southbound->SetAttribute("MaxActionWaitTime", IntegerValue (timeMs));
}


}
