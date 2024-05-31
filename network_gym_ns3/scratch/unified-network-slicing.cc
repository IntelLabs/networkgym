/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*  File : unified-network-slicing.cc
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/virtual-net-device.h"
#include "ns3/wifi-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include <ns3/spectrum-module.h>
#include "ns3/gma-module.h"
#include "ns3/nr-helper.h"
#include "ns3/nr-mac-scheduler-tdma-rr.h"
#include "ns3/nr-module.h"
#include "ns3/nr-point-to-point-epc-helper.h"
#include <ns3/antenna-module.h>

using json = nlohmann::json;

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("GmaSimMlPlayground");

static void RcvFrom (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet, const Address &from)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize()<< std::endl;
}

static void Rcv (Ptr<OutputStreamWrapper> stream, Ptr<const Packet> packet)
{
  SeqTsHeader seqTs;
  packet->PeekHeader (seqTs);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << packet->GetSize() << "\t" <<
   Simulator::Now ().GetMilliSeconds() - seqTs.GetTs ().GetMilliSeconds()<< std::endl;
}


static void
CwndChange (Ptr<OutputStreamWrapper> stream, uint32_t oldCwnd, uint32_t newCwnd)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldCwnd << "\t" << newCwnd << std::endl;
}

static void
RttChange (Ptr<OutputStreamWrapper> stream, Time oldRtt, Time newRtt)
{
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << oldRtt.GetSeconds () << "\t" << newRtt.GetSeconds () << std::endl;
}


static void
DonwlinkTraces(uint16_t nodeNum)
{
  AsciiTraceHelper asciiTraceHelper;

  std::ostringstream pathCW;
  pathCW<<"/NodeList/0/$ns3::TcpL4Protocol/SocketList/"<<nodeNum<<"/CongestionWindow";

  std::ostringstream fileCW;
  fileCW<<"cwnd-"<<nodeNum<<".txt";

  std::ostringstream pathRTT;
  pathRTT<<"/NodeList/0/$ns3::TcpL4Protocol/SocketList/"<<nodeNum<<"/RTT";

  std::ostringstream fileRTT;
  fileRTT<<"rtt-"<<nodeNum<<".txt";

  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileCW.str ().c_str ());
  Config::ConnectWithoutContextFailSafe (pathCW.str ().c_str (), MakeBoundCallback(&CwndChange, stream1));

  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileRTT.str ().c_str ());
  Config::ConnectWithoutContextFailSafe (pathRTT.str ().c_str (), MakeBoundCallback(&RttChange, stream2));

}

static void
UplinkTraces(uint16_t nodeNum)
{
  AsciiTraceHelper asciiTraceHelper;

  std::ostringstream pathCW;
  pathCW<<"/NodeList/"<<nodeNum+1<<"/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow";

  std::ostringstream fileCW;
  fileCW<<"cwnd-"<<nodeNum<<".txt";

  std::ostringstream pathRTT;
  pathRTT<<"/NodeList/"<<nodeNum+1<<"/$ns3::TcpL4Protocol/SocketList/0/RTT";

  std::ostringstream fileRTT;
  fileRTT<<"rtt-"<<nodeNum<<".txt";

  Ptr<OutputStreamWrapper> stream1 = asciiTraceHelper.CreateFileStream (fileCW.str ().c_str ());
  Config::ConnectWithoutContextFailSafe (pathCW.str ().c_str (), MakeBoundCallback(&CwndChange, stream1));

  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper.CreateFileStream (fileRTT.str ().c_str ());
  Config::ConnectWithoutContextFailSafe (pathRTT.str ().c_str (), MakeBoundCallback(&RttChange, stream2));

}

class GmaSimWorker : public Object
{
public:
  /**
    * Register this type.
    * \return The TypeId.
    */
  static TypeId GetTypeId();
  GmaSimWorker();
  ~GmaSimWorker();
  int GetClosestWifiAp (Ptr<Node> userNode);
  void UpdateStatus ();
  void LogLocations ();
  void WifiRateCallback (std::string path, DataRate rate, Mac48Address dest);
  void LteEnbMeasurementCallback (std::string path, std::vector<int> sliceId, std::vector<double> rate, std::vector<double> rbUsage, bool dl);
  void LteUeMeasurementCallback (std::string path, int sliceId, double rate, double rbUsage, uint64_t imsi, bool dl);
  void NotifyConnectionEstablished (std::string context, uint64_t imsi, uint16_t cellId, uint16_t rnti);
  void ParseJsonConfig ();
  void SaveConfigFile ();
  void InstallNetworks();
  void InstallGmaInterface ();
  void InstallApplications ();
  void ConnectTraceCallbacks ();
  void StoreMacAddress ();
  void Run ();
  void TrafficSplittingDeployment ();
  void QoSTrafficSteeringDeployment ();
private:

  std::vector<Ipv4InterfaceContainer> m_iCList;
  std::vector<Ipv4InterfaceContainer> m_iRiAPList;
  Ipv4InterfaceContainer m_iRiNr;

  Ptr<GmaProtocol> m_routerGma;

  NodeContainer m_clientNodes;
  NodeContainer m_eNodeBs;
  NodeContainer m_nrEnbNodes;
  NodeContainer m_apNodes;
  std::vector<NetDeviceContainer> m_stationDeviceList;//first demension is the AP id, the seconds demension is the user station id.
  NetDeviceContainer m_apDeviceList;
  std::map<Mac48Address, uint64_t> m_macAddrToUserMap;
  std::map<Mac48Address, uint64_t> m_macAddrToCellMap;
  Time m_stopTime = Seconds(10.0);
  int m_numOfUsers = 1;

  Time m_wifiMeasurementInterval = Seconds(1.0);
  Time m_lteUeMeasurementInterval = Seconds(1.0);

  int m_radioType = -1;
  int m_distance = 50;
  bool m_enableTCPtrace = false;

  std::string m_wifiBackhaulDelay = "0ms";
  uint32_t m_nrBackhaulDelayMs = 0;
  uint32_t m_lteBackhaulDelayMs = 0;
  double m_userSpeedMin = 1.0; //m/s
  double m_userSpeedMax = 1.0; //m/s
  double m_userDirectionMin = 0.0; //gradiants
  double m_userDirectionMax = 6.283184; //gradiants
  double m_userWalkDistance = 3; //m

  bool m_downlink = true;
  double m_wifiLowPowerThresh = -80.0;
  double m_wifiHighPowerThresh = -77.0;

  double m_wifiApSwitchPowerThresh = -79;

  bool m_enableRtsCts = true;

  int m_wifiDownTime = 0; //ms

  double m_gmaDelaySplitThresh = 10;
  bool m_fixWifiRate = false;
  bool m_enableRxTrace = false;
  // setup topology

  std::string m_apManager = "ns3::GmaMinstrelHtWifiManager";
  int m_measurement_start_time_ms = 0;
  int m_action_wait_ms = -1;

  int m_numOfAps = 2;
  int m_numOfLteEnbs = 1; //Must be one, cannot be changed....
  int m_numOfNrEnbs = 2;

  //steer choose the minimal link
  // auto: tcp ack use steer
  bool m_dfp = false; //dynamic flow prioritization.
  std::string m_splittingAlgorithm = "gma2";
  uint32_t m_splittingBurst = 32;
  Ptr<GmaDataProcessor> m_gmaDataProcessor;
  Ipv4InterfaceContainer m_iSiR;
  NetDeviceContainer m_dSdR;
  std::vector<Ipv4Address> m_clientVirtualIpList;
  Ipv4InterfaceContainer m_ueIpIface;
  Ipv4InterfaceContainer m_nrUeIpIface;
  Ptr<Node> m_router;
  Ptr<Node> m_nrRouter;
  Ptr<PointToPointEpcHelper> m_epcHelper;
  Ptr<NrPointToPointEpcHelper> m_nrEpcHelper;
  Ptr<NrHelper> m_nrHelper;
  Ptr<IdealBeamformingHelper> m_idealBeamformingHelper;
  Ptr<Node> m_server;
  std::vector< Ptr<GmaProtocol> > m_clientGmaList; //this cannot be local variable, will cause error....
  Vector m_enbLocations;
  std::vector <Vector> m_apLocationVector;
  std::vector <Vector> m_nrEnbLocationVector;
  std::vector <Vector> m_ueLocationVector;

  double m_user_min_x = 0;
  double m_user_max_x = 0;
  double m_user_min_y = 1;
  double m_user_max_y = 1;

  uint32_t m_lteRb = 25;
  bool m_wifiShareBand = false;
  bool m_wifiHandover = false;

  Ptr<UniformRandomVariable> m_uniformRv;
  struct SliceInfo : public SimpleRefCount<SliceInfo>
  {
  uint64_t m_numUsers = 0;
  uint64_t m_dedicatedRbg = 0;
  uint64_t m_prioritizedRbg = 0;
  uint64_t m_sharedRbg = 0;
  uint64_t m_minRbg = 0; //=m_dedicatedRbg + m_prioritizedRbg
  uint64_t m_maxRbg = 0; //=m_dedicatedRbg + m_prioritizedRbg+m_sharedRbg
  };

  std::vector< Ptr<SliceInfo> > m_sliceList;
  uint64_t m_sliceNum = 0;

  std::map<std::pair<int, uint64_t>, uint16_t> m_imsiToCellIdMap; //the key is the node id of the enb and imsi of user. Node id helps identify which bs, 4g, or 5g...

  int m_wifi_cell_id_offset = 1;
  int m_nr_bwp_num = 2;

  struct PerSliceConfig : public SimpleRefCount<PerSliceConfig>
  {
    int m_packetSize = 1400;
    std::string m_transport_protocol = "";
    bool m_tcpData = true;
    bool m_poissonArrival = false;
    double m_minUdpRateMbps = 20.0;
    double m_maxUdpRateMbps = 30.0;
    int m_qosTestDurationMs = 500;
    double m_qosLossTarget = 1.0; //the max allowed packet loss for a qos flow, if this target is not met, qos rate = 0.
    double m_qosDelayViolationTarget = 0.01; //the max allowed delay violation for a qos flow, if this target is not met, qos rate = 0.
    int m_qosDelayRequirementMs = 5; //if a packet's delay is more than m_qosDelayRequirementMs, it is marked as delay violation.
    int m_delaythresh1 = 200; //for testing measurement.
    int m_delaythresh2 = 400; //for testing measurement.
    std::string m_dlGmaMode = "auto"; // 3 modes: "auto"; "split"; "steer"; "wifi" or "lte";
    std::string m_ulGmaMode = "auto"; // 3 modes: "auto"; "split"; "steer"; "wifi" or "lte";
  };

  std::vector< Ptr<PerSliceConfig> > m_perSliceConfigList;
  std::vector<int> m_sliceIdList;
  uint8_t m_beTos = 0x70 & 0xe0; //AC_BE with 3 MSB mask
};

GmaSimWorker::GmaSimWorker()
{
  m_uniformRv = CreateObject<UniformRandomVariable>();
  m_gmaDataProcessor = CreateObject<GmaDataProcessor>();

}
GmaSimWorker::~GmaSimWorker()
{

}

TypeId 
GmaSimWorker::GetTypeId()
{
    static TypeId tid = TypeId("GmaSimWorker")
                            .SetParent<Object>()
                            .SetGroupName("Scratch")
                            .AddConstructor<GmaSimWorker>()
    ;
    return tid;
}

int
GmaSimWorker::GetClosestWifiAp (Ptr<Node> userNode)
{
  NS_ASSERT_MSG (m_apNodes.GetN () > 0, "empty wifi node container");
  Vector uepos = userNode->GetObject<MobilityModel> ()->GetPosition ();

  double minDistance = std::numeric_limits<double>::infinity ();
  Ptr<Node> apNode;
  int minDisApId = -1;
  for (uint32_t apInd = 0; apInd < m_apNodes.GetN(); apInd++)
  {
      apNode = m_apNodes.Get(apInd);
      Vector enbpos = apNode->GetObject<MobilityModel> ()->GetPosition ();
      double m_distance = CalculateDistance (uepos, enbpos);
      if (m_distance < minDistance)
        {
          minDistance = m_distance;
          minDisApId = apInd; 
        }
  }

  NS_ASSERT (minDisApId != -1);
  return minDisApId;
}

void
GmaSimWorker::UpdateStatus(){
          //log file
  std::ifstream ifTraceFile;
  ifTraceFile.open ("stop.txt", std::ifstream::in);
  if (ifTraceFile.good ())
  {
    //stop the simulatio right now!!
    std::ostringstream fileName;
    fileName <<"status.txt";
    std::ofstream myfile;
    myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
    //time_t seconds;

    //seconds = time (NULL);
    //myfile << (int)seconds << "/"<< m_stopTime.GetSeconds() << "/" <<m_stopTime.GetSeconds() <<std::endl;
    myfile << m_stopTime.GetSeconds() << "/" <<m_stopTime.GetSeconds() <<std::endl;

    myfile.close();

    NS_FATAL_ERROR("find the stop file");

  }
  else
  {
    std::ostringstream fileName;
    fileName <<"status.txt";
    std::ofstream myfile;
    myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
    //time_t seconds;

    //seconds = time (NULL);

    //myfile << (int)seconds << "/"<< Simulator::Now ().GetSeconds () << "/" <<m_stopTime.GetSeconds() <<std::endl;
    myfile << Simulator::Now ().GetSeconds () << "/" <<m_stopTime.GetSeconds() <<std::endl;
    myfile.close();
  }

    
}


void
GmaSimWorker::LogLocations ()
{
      //log file
    std::ostringstream fileName;
    fileName <<"config.txt";
    std::ofstream myfile;
    myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
    if(Simulator::Now ().GetSeconds () < 0.1)
    {
        myfile << "4G LTE EnB locations: ";
        for (uint32_t apInd = 0; apInd < m_eNodeBs.GetN(); apInd++)
        {
            Vector enbpos = m_eNodeBs.Get(apInd)->GetObject<MobilityModel> ()->GetPosition ();
            myfile <<"["<< +enbpos.x << "," << +enbpos.y << "," << +enbpos.z << "] ";
        }
        myfile << std::endl;
        myfile << "5G NR EnB locations: ";
        for (uint32_t apInd = 0; apInd < m_nrEnbNodes.GetN(); apInd++)
        {
            Vector enbpos = m_nrEnbNodes.Get(apInd)->GetObject<MobilityModel> ()->GetPosition ();
            myfile <<"["<< +enbpos.x << "," << +enbpos.y << "," << +enbpos.z << "] ";
        }
        myfile << std::endl;
        myfile << "Wi-Fi AP locations: ";
        for (uint32_t apInd = 0; apInd < m_apNodes.GetN(); apInd++)
        {
            Vector wifipos = m_apNodes.Get(apInd)->GetObject<MobilityModel> ()->GetPosition ();
            myfile << "[" << +wifipos.x << "," << +wifipos.y << "," << +wifipos.z << "] ";
        }
        myfile << std::endl;
         for (uint32_t apInd = 0; apInd < m_clientNodes.GetN(); apInd++)
        {
            Vector uepos = m_clientNodes.Get(apInd)->GetObject<MobilityModel> ()->GetPosition ();
            myfile << "UE "<< apInd+1 << " location: [" << +uepos.x << "," << +uepos.y << "," << +uepos.z << "]" << std::endl;
            // std::cout << "UE "<< apInd+1 << " location: [" << +uepos.x << "," << +uepos.y << "," << +uepos.z << std::endl;

        }
    }
    //myfile << "Time: "<< Simulator::Now ().GetSeconds ()<< " Second" << std::endl;

    /*for (uint32_t apInd = 0; apInd < m_clientNodes.GetN(); apInd++)
    {
        Vector uepos = m_clientNodes.Get(apInd)->GetObject<MobilityModel> ()->GetPosition ();
        //myfile << "UE "<< apInd+1 << " location: " << +uepos.x << "," << +uepos.y << "," << +uepos.z << std::endl;
        //std::cout << "UE "<< apInd+1 << " location: [" << +uepos.x << "," << +uepos.y << "," << +uepos.z << "]" << std::endl;

        if(m_userSpeed > 0)
        {
          if(uepos.x >= m_user_max_x)
          {
            m_clientNodes.Get(apInd)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (m_userSpeed*(-1), 0, 0));
          }
          else if (uepos.x <= m_user_min_x)
          {
            m_clientNodes.Get(apInd)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (m_userSpeed, 0, 0));
          }

        }

    }*/

    myfile.close();
    UpdateStatus();
    Simulator::Schedule (Seconds(1.0), &GmaSimWorker::LogLocations, this);

}

void
GmaSimWorker::WifiRateCallback(std::string path, DataRate rate, Mac48Address dest)
{
    //std::cout << Simulator::Now().GetSeconds() << " "<< path << " addr:" << dest << " Rate:" << rate.GetBitRate()/1e6 << std::endl;

    //we need to store all ap -> client mapping and make sure collecting the data of client that belongs to the designated AP!!!! using cellid.
    std::map<Mac48Address, uint64_t>::iterator iter = m_macAddrToUserMap.find(dest);

    int nodeId = -1;

    std::string s = path;
    std::string delimiter = "/";
    size_t pos = 0;
    std::string token;
    std::string last_token = "";
    while ((pos = s.find(delimiter)) != std::string::npos)
    {
      token = s.substr(0, pos);
      //std::cout << token << std::endl;
      if (last_token.compare("NodeList") == 0)
      {
        nodeId = std::stoi(token);
      }
      else if (last_token.compare("DeviceList") == 0)
      {
      }
      s.erase(0, pos + delimiter.length());
      last_token = token;
    }

    if(iter!= m_macAddrToUserMap.end())
    {
      int cellId = m_macAddrToCellMap[dest];
      uint64_t userId = iter->second;
      int AssignedCellId = m_gmaDataProcessor->GetCellId(userId, WIFI_CID);
      //std::cout << Simulator::Now().GetSeconds() << " " << path << " userID:" << userId << " cellId:" <<  cellId << " assginedCellId:" << AssignedCellId << " addr:" << dest << " Rate:" << rate.GetBitRate()/1e6 << std::endl;
      if (AssignedCellId == cellId)//for handover, only update the measurement from the asigned AP (by GMA algorithm).
      {
        Time nowTime = Now();
        ns3::Ptr<ns3::NetworkStats> element = ns3::CreateObject<ns3::NetworkStats>("wifi", userId, nowTime.GetMilliSeconds());
        element->Append("dl::max_rate", (double)rate.GetBitRate()/1e6);
        m_gmaDataProcessor->AppendMeasurement(element);
      }
    }
    else
    {
      int imsi = -1;
      NetDeviceContainer deviceContainer = m_stationDeviceList.at(0);
      for (uint64_t dev = 0; dev < deviceContainer.GetN (); dev++)
      {
        Ptr<WifiNetDevice> wifiStaDevice = DynamicCast<WifiNetDevice> (deviceContainer.Get(dev));
        int devNodeId = wifiStaDevice->GetNode()->GetId();
        if (devNodeId == nodeId)
        {
          imsi = dev + 1;
        }
      }
      

      if(imsi == -1)
      {
        NS_FATAL_ERROR("cannot find wifi imsi for this user!");
      }

      int cellId = -1;
      for (uint64_t dev = 0; dev < m_apDeviceList.GetN (); dev++)
      {
        Ptr<WifiNetDevice> wifiApDev = DynamicCast<WifiNetDevice> (m_apDeviceList.Get(dev));
        Mac48Address addr = wifiApDev->GetMac ()->GetAddress ();
        if (dest == addr)
        {
          cellId = dev;
        }
      }
      if(cellId == -1)
      {
        NS_FATAL_ERROR("cannot find wifi cell id for this user!");
      }

      //std::cout << " nodeId: " << nodeId << " imsi: " << imsi  << " cellId: " << cellId <<  std::endl;

      auto AssignedCellId = m_gmaDataProcessor->GetCellId(imsi, WIFI_CID);
      if (AssignedCellId == cellId)//for handover, only update the measurement from the asigned AP (by GMA algorithm).
      {
        Time nowTime = Now();
        ns3::Ptr<ns3::NetworkStats> element = ns3::CreateObject<ns3::NetworkStats>("wifi", imsi, nowTime.GetMilliSeconds());
        //element->Append("max_rate::ul", "slice", std::vector<double>{(double)rate.GetBitRate()/1e6, 123});
        element->Append("ul::max_rate", (double)rate.GetBitRate()/1e6);
        m_gmaDataProcessor->AppendMeasurement(element);
      }
    }
}

void
GmaSimWorker::LteEnbMeasurementCallback(std::string path, std::vector<int> sliceId, std::vector<double> rate,  std::vector<double> rbUsage, bool dl)
{
  //std::cout << Simulator::Now().GetSeconds() << " "<< path << " rate:" << rate.at(0) << " sliceId:" << sliceId.at(0) << " rbUsage:" << rbUsage.at(0) << " dl:" << dl<< std::endl;
  //we will overwrite the cell id here...
  int nodeId = -1;
  int deviceId = -1;

  std::string s = path;
  std::string delimiter = "/";
  size_t pos = 0;
  std::string token;
  std::string last_token = "";
  while ((pos = s.find(delimiter)) != std::string::npos)
  {
    token = s.substr(0, pos);
    //std::cout << token << std::endl;
    if (last_token.compare("NodeList") == 0)
    {
      nodeId = std::stoi(token);
    }
    else if (last_token.compare("DeviceList") == 0)
    {
      deviceId = std::stoi(token);
    }
    s.erase(0, pos + delimiter.length());
    last_token = token;
  }

  int cellId = -1;
  for (uint32_t i = 0; i < m_eNodeBs.GetN(); i++)
  {
    if (nodeId == (int)m_eNodeBs.Get(i)->GetId())
    {
      cellId = m_eNodeBs.Get(i)->GetDevice(0)->GetObject<LteEnbNetDevice>()->GetCellId();
    }
  }

  //std::cout << " nodeId:" << nodeId << " deviceId:" << deviceId << " cellId:" << cellId << std::endl;

  if(nodeId < 0 || deviceId < 0 || cellId < 0)
  {
    NS_FATAL_ERROR("Cannot find the nodeId or deviceId or cellId");
  }
  Time nowTime = Now();
  ns3::Ptr<ns3::NetworkStats> element = ns3::CreateObject<ns3::NetworkStats>("lte", cellId, nowTime.GetMilliSeconds());

  if (dl)
  {
    element->Append("dl::cell::max_rate", "slice", sliceId, rate);
    element->Append("dl::cell::rb_usage", "slice", sliceId, rbUsage);
  }
  else
  {
    //uplink not implemented yet.
  }
  m_gmaDataProcessor->AppendMeasurement(element);
}

void
GmaSimWorker::LteUeMeasurementCallback(std::string path, int sliceId, double rate, double rbUsage, uint64_t imsi, bool dl)
{
  //std::cout << Simulator::Now().GetSeconds() << " "<< path << " rate:" << rate << " sliceId:" << sliceId << " rbUsage:" << rbUsage << " imsi:" << imsi << " dl:" << dl<< std::endl;
  Time nowTime = Now();
  ns3::Ptr<ns3::NetworkStats> element = ns3::CreateObject<ns3::NetworkStats>("lte", imsi, nowTime.GetMilliSeconds());

  int nodeId = -1;

  std::string s = path;
  std::string delimiter = "/";
  size_t pos = 0;
  std::string token;
  std::string last_token = "";
  while ((pos = s.find(delimiter)) != std::string::npos)
  {
    token = s.substr(0, pos);
    //std::cout << token << std::endl;
    if (last_token.compare("NodeList") == 0)
    {
      nodeId = std::stoi(token);
    }
    s.erase(0, pos + delimiter.length());
    last_token = token;
  }

  uint16_t cellId = 255;
  auto keyT = std::make_pair(nodeId, imsi);
  if(m_imsiToCellIdMap.find(keyT) != m_imsiToCellIdMap.end())
  {
    cellId = m_imsiToCellIdMap[keyT];
  }
  
  if (dl)
  {
    element->Append("dl::max_rate", rate);
    element->Append("cell_id", cellId);
    m_gmaDataProcessor->UpdateCellId(imsi, cellId, "lte");
    element->Append("slice_id", sliceId);
    m_gmaDataProcessor->UpdateSliceId(imsi, sliceId);
    element->Append("dl::rb_usage", rbUsage);

  }
  else
  {
    //uplink not implemented yet.
  }
  m_gmaDataProcessor->AppendMeasurement(element);

  if(m_nrEnbNodes.GetN() > 0)
  {
    //create a measurement for NR ue here. move the NR measurement in the future.
    //NR not support handover yet.
    ns3::Ptr<ns3::NetworkStats> elementNr = ns3::CreateObject<ns3::NetworkStats>("nr", imsi, nowTime.GetMilliSeconds());
    elementNr->Append("cell_id", m_gmaDataProcessor->GetCellId(imsi, CELLULAR_NR_CID)/m_nr_bwp_num); //divide the number of bandwith part...
    m_gmaDataProcessor->AppendMeasurement(elementNr);
  }

}

void
GmaSimWorker::NotifyConnectionEstablished (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << "MlMacScheduler " << context
        << " eNB CellId " << cellid
        << ": successful connection of UE with IMSI " << imsi
        << " RNTI " << rnti
        << std::endl;
  
  int nodeId = -1;

  std::string s = context;
  std::string delimiter = "/";
  size_t pos = 0;
  std::string token;
  std::string last_token = "";
  while ((pos = s.find(delimiter)) != std::string::npos)
  {
    token = s.substr(0, pos);
    //std::cout << token << std::endl;
    if (last_token.compare("NodeList") == 0)
    {
      nodeId = std::stoi(token);
    }
    s.erase(0, pos + delimiter.length());
    last_token = token;
  }
  m_imsiToCellIdMap[std::make_pair(nodeId, imsi)] = cellid;

  //TODO: move it to the nr measurement in the future.
  if (nodeId != 1)
  {
    //not lte ENB
    m_gmaDataProcessor->UpdateCellId(imsi, cellid, "nr");
  }
}

void
GmaSimWorker::ParseJsonConfig ()
{
  // read a JSON file
  std::ifstream jsonStream("env-configure.json");
  json jsonConfig;
  try
  {
    jsonStream >> jsonConfig;
  }
  catch (const json::type_error& e)
  {
      // output exception information
      std::cout << "message: " << e.what() << '\n'
                << "exception id: " << e.id << std::endl;
  }

  m_stopTime = MilliSeconds(jsonConfig["env_end_time_ms"].get<int>());
  int random_seed = jsonConfig["random_seed"].get<int>();
  RngSeedManager::SetSeed(1);
  RngSeedManager::SetRun(random_seed);

  m_downlink = jsonConfig["downlink_traffic"].get<bool>();
  /*if (!m_downlink)
  {
    m_enableRtsCts = true; //for uplink, I prefer to turn it on.
  }*/

  m_action_wait_ms = jsonConfig["max_wait_time_for_action_ms"].get<int>();

  //limit the max waiting time to 600 seconds
  if(m_action_wait_ms < 0 || m_action_wait_ms > 600000)
  {
    m_action_wait_ms = 600000;
  }

  m_enbLocations = Vector(jsonConfig["lte_enb_locations"]["x"].get<double>(), 
    jsonConfig["lte_enb_locations"]["y"].get<double>(),
    jsonConfig["lte_enb_locations"]["z"].get<double>());

  m_numOfNrEnbs = jsonConfig["nr_gnb_locations"].size();
  for(int ind = 0; ind < m_numOfNrEnbs; ind++)
  {
    m_nrEnbLocationVector.push_back( Vector(jsonConfig["nr_gnb_locations"][ind]["x"].get<double>(),
     jsonConfig["nr_gnb_locations"][ind]["y"].get<double>(),
     jsonConfig["nr_gnb_locations"][ind]["z"].get<double>()));
  }

  m_numOfAps = jsonConfig["wifi_ap_locations"].size();
  for(int ind = 0; ind < m_numOfAps; ind++)
  {
    m_apLocationVector.push_back( Vector(jsonConfig["wifi_ap_locations"][ind]["x"].get<double>(),
     jsonConfig["wifi_ap_locations"][ind]["y"].get<double>(),
     jsonConfig["wifi_ap_locations"][ind]["z"].get<double>()));
  }

  if (m_numOfAps + m_numOfNrEnbs == 0)
  {
    //TODO: also check the number of nr base stations.
    m_radioType = CELLULAR_LTE_CID; //no WiFi AP
  }

  m_user_min_x = jsonConfig["user_location_range"]["min_x"].get<double>();
  m_user_max_x = jsonConfig["user_location_range"]["max_x"].get<double>();

  m_user_min_y = jsonConfig["user_location_range"]["min_y"].get<double>();
  m_user_max_y = jsonConfig["user_location_range"]["max_y"].get<double>();

  double z_loc = jsonConfig["user_location_range"]["z"].get<double>();

  m_numOfUsers = jsonConfig["num_users"].get<int>();

  if(m_numOfUsers > 50)
  {
    NS_FATAL_ERROR("We only support max of 50 users...");
  }


  m_sliceNum = jsonConfig["per_slice_config"]["num_users"].size();
  std::cout << "num of slices: " << m_sliceNum << std::endl;

  if (m_sliceNum > 8 || m_sliceNum == 0)
  {
    NS_FATAL_ERROR("We only support max of 8 slices using 3 bits in dscp field. slice number also cannot be zero!");
  }

  for(uint64_t ind = 0; ind < m_sliceNum; ind++)
  {
    Ptr<SliceInfo> sliceInfo = Create<SliceInfo>();
    sliceInfo->m_numUsers = jsonConfig["per_slice_config"]["num_users"][ind].get<int>();
    sliceInfo->m_dedicatedRbg = jsonConfig["per_slice_config"]["dedicated_rbg"][ind].get<int>();
    sliceInfo->m_prioritizedRbg = jsonConfig["per_slice_config"]["prioritized_rbg"][ind].get<int>();
    sliceInfo->m_sharedRbg = jsonConfig["per_slice_config"]["shared_rbg"][ind].get<int>();
    m_sliceList.push_back(sliceInfo);

    auto perSliceConfig = Create<PerSliceConfig>();

    perSliceConfig->m_packetSize = jsonConfig["per_slice_config"]["packet_size_bytes"][ind].get<int>();

    perSliceConfig->m_transport_protocol = jsonConfig["per_slice_config"]["transport_protocol"][ind].get<std::string>();

    if(perSliceConfig->m_transport_protocol.compare(0, 3, "tcp") == 0 || perSliceConfig->m_transport_protocol.compare(0, 3, "TCP") == 0 || perSliceConfig->m_transport_protocol.compare(0, 3, "Tcp") == 0)
    {
      if(perSliceConfig->m_transport_protocol.compare("tcp") == 0 || perSliceConfig->m_transport_protocol.compare("TCP") == 0 || perSliceConfig->m_transport_protocol.compare("Tcp") == 0)
      {
        perSliceConfig->m_transport_protocol = "TcpBbr"; //use BBR as default if not specified.
      }
      perSliceConfig->m_transport_protocol = std::string("ns3::") + perSliceConfig->m_transport_protocol;

      perSliceConfig->m_tcpData = true;
    }
    else if(perSliceConfig->m_transport_protocol.compare("udp") == 0 || perSliceConfig->m_transport_protocol.compare("UDP") == 0 || perSliceConfig->m_transport_protocol.compare("Udp") == 0)
    {
      perSliceConfig->m_tcpData = false;
    }
    else
    {
      NS_FATAL_ERROR("unkown Protocol....only support tcp or udp");
    }

    perSliceConfig->m_poissonArrival = jsonConfig["per_slice_config"]["udp_poisson_arrival"][ind].get<bool>();
    perSliceConfig->m_minUdpRateMbps = jsonConfig["per_slice_config"]["min_udp_rate_per_user_mbps"][ind].get<int>();
    perSliceConfig->m_maxUdpRateMbps = jsonConfig["per_slice_config"]["max_udp_rate_per_user_mbps"][ind].get<int>();
    perSliceConfig->m_qosTestDurationMs =  jsonConfig["per_slice_config"]["qos_test_duration_ms"][ind].get<int>();
    if(perSliceConfig->m_qosTestDurationMs < 100 || perSliceConfig->m_qosTestDurationMs > 25000)
    {
      NS_FATAL_ERROR("qos_test_duration_ms only support range from [100, 25000].");
    }

    perSliceConfig->m_qosLossTarget = jsonConfig["per_slice_config"]["qos_loss_target"][ind].get<double>();
    perSliceConfig->m_qosDelayViolationTarget = jsonConfig["per_slice_config"]["qos_delay_violation_target"][ind].get<double>();
    perSliceConfig->m_qosDelayRequirementMs = jsonConfig["per_slice_config"]["qos_delay_requirement_ms"][ind].get<int>();
    perSliceConfig->m_delaythresh1 = jsonConfig["per_slice_config"]["delay_test_1_thresh_ms"][ind].get<int>();
    perSliceConfig->m_delaythresh2 = jsonConfig["per_slice_config"]["delay_test_2_thresh_ms"][ind].get<int>();

    sliceInfo->m_numUsers = jsonConfig["per_slice_config"]["num_users"][ind].get<int>();
    sliceInfo->m_dedicatedRbg = jsonConfig["per_slice_config"]["dedicated_rbg"][ind].get<int>();
    sliceInfo->m_prioritizedRbg = jsonConfig["per_slice_config"]["prioritized_rbg"][ind].get<int>();
    sliceInfo->m_sharedRbg = jsonConfig["per_slice_config"]["shared_rbg"][ind].get<int>();
    perSliceConfig->m_dlGmaMode = jsonConfig["per_slice_config"]["downlink_mode"][ind].get<std::string>();
    perSliceConfig->m_ulGmaMode = jsonConfig["per_slice_config"]["uplink_mode"][ind].get<std::string>();

    if(perSliceConfig->m_dlGmaMode.compare("auto") !=0 && perSliceConfig->m_dlGmaMode.compare("split") !=0 && perSliceConfig->m_dlGmaMode.compare("steer") !=0)
    {
      NS_FATAL_ERROR("only support 3 modes: auto, split, and steer. input m_dlGmaMode:" << perSliceConfig->m_dlGmaMode);
    }

    if(perSliceConfig->m_ulGmaMode.compare("auto") !=0 && perSliceConfig->m_ulGmaMode.compare("split") !=0 && perSliceConfig->m_ulGmaMode.compare("steer") !=0)
    {
      NS_FATAL_ERROR("only support 3 modes: auto, split, and steer. input m_ulGmaMode:" << perSliceConfig->m_ulGmaMode);
    }

    std::cout <<"slice: "<< +ind << " num_users = " << sliceInfo->m_numUsers << std::endl
              <<"slice: "<< +ind << " dedicated_rbg = " << sliceInfo->m_dedicatedRbg << std::endl
              <<"slice: "<< +ind << " shared_rbg = " <<  sliceInfo->m_prioritizedRbg << std::endl
              <<"slice: "<< +ind << " packet_size_bytes = " << perSliceConfig->m_packetSize << std::endl
              <<"slice: "<< +ind << " transport_protocol = " << perSliceConfig->m_transport_protocol << std::endl
              <<"slice: "<< +ind << " udp_poisson_arrival = " << perSliceConfig->m_poissonArrival << std::endl
              <<"slice: "<< +ind << " min_udp_rate_per_user_mbps = " << perSliceConfig->m_minUdpRateMbps << std::endl
              <<"slice: "<< +ind << " max_udp_rate_per_user_mbps = " << perSliceConfig->m_maxUdpRateMbps << std::endl
              <<"slice: "<< +ind << " qos_loss_target = " << perSliceConfig->m_qosLossTarget <<  std::endl
              <<"slice: "<< +ind << " qos_delay_violation_target = " << perSliceConfig->m_qosDelayViolationTarget <<  std::endl
              <<"slice: "<< +ind << " qos_delay_requirement_ms = " << perSliceConfig->m_qosDelayRequirementMs <<  std::endl
              <<"slice: "<< +ind << " delay_test_1_thresh_ms = " << perSliceConfig->m_delaythresh1 <<  std::endl
              <<"slice: "<< +ind << " delay_test_2_thresh_ms = " << perSliceConfig->m_delaythresh2 <<  std::endl
              <<"slice: "<< +ind << " downlink_mode = " << perSliceConfig->m_dlGmaMode <<  std::endl
              <<"slice: "<< +ind << " uplink_mode = " << perSliceConfig->m_ulGmaMode <<  std::endl;

    m_perSliceConfigList.push_back(perSliceConfig);

  }

  for (uint32_t j = 0; j < m_sliceList.size(); j++)
  {
    for (uint32_t k = 0; k < m_sliceList.at(j)->m_numUsers; k++)
    {
      m_sliceIdList.push_back(j);
    }
  }

  //for (uint32_t i = 0; i < m_sliceIdList.size(); i++)
  //{
  //  std::cout << m_sliceIdList.at(i) << " ";
  //}
  //std::cout << std::endl;

  m_uniformRv->SetStream(random_seed);
  for(int ind = 0; ind < m_numOfUsers; ind++)
  {
    double x_loc = m_uniformRv->GetInteger(m_user_min_x, m_user_max_x);
    double y_loc = m_uniformRv->GetInteger(m_user_min_y, m_user_max_y);
    m_ueLocationVector.push_back(Vector(x_loc, y_loc, z_loc));
    //std::cout<< " x_loc:" << x_loc << " y_loc:" << y_loc << " z_loc:" << z_loc << std::endl;
  }
  
  m_userSpeedMin = jsonConfig["user_random_walk"]["min_speed_m/s"].get<double>();
  m_userSpeedMax = jsonConfig["user_random_walk"]["max_speed_m/s"].get<double>();

  if (m_userSpeedMax < m_userSpeedMin)
  {
    NS_FATAL_ERROR("user_random_walk: min_speed_m/s must be smaller or equal to max_speed_m/s!");
  }

  m_userDirectionMin = jsonConfig["user_random_walk"]["min_direction_gradients"].get<double>();
  m_userDirectionMax = std::min(m_userDirectionMax, jsonConfig["user_random_walk"]["max_direction_gradients"].get<double>());

  if (m_userDirectionMax < m_userDirectionMin)
  {
    NS_FATAL_ERROR("user_random_walk: min_direction_gradients must be smaller or equal to max_direction_gradients!");
  }

  m_userWalkDistance = jsonConfig["user_random_walk"]["distance_m"].get<double>();

  m_measurement_start_time_ms = jsonConfig["measurement_start_time_ms"].get<int>();

  m_dfp = jsonConfig["gma"]["enable_dynamic_flow_prioritization"].get<bool>();
  m_splittingAlgorithm = jsonConfig["gma"]["mx_algorithm"].get<std::string>();

  Time gma_interval = MilliSeconds(jsonConfig["gma"]["measurement_interval_ms"].get<int>());
  Time gma_guard = MilliSeconds(jsonConfig["gma"]["measurement_guard_interval_ms"].get<int>());

  int wifiBackhaulDelayMs = jsonConfig["wifi"]["backhaul_delay_ms"].get<int>();
  m_wifiBackhaulDelay = std::to_string(wifiBackhaulDelayMs) + "ms";
  m_wifiShareBand = jsonConfig["wifi"]["ap_share_same_band"].get<bool>();
  m_wifiHandover = jsonConfig["wifi"]["enable_rx_signal_based_handover"].get<bool>();
  m_wifiMeasurementInterval = MilliSeconds(jsonConfig["wifi"]["measurement_interval_ms"].get<int>());
  Time wifi_guard = MilliSeconds(jsonConfig["wifi"]["measurement_guard_interval_ms"].get<int>());

  m_nrBackhaulDelayMs = jsonConfig["nr"]["backhaul_delay_ms"].get<int>();
  m_lteBackhaulDelayMs = jsonConfig["lte"]["backhaul_delay_ms"].get<int>();
  m_lteRb = jsonConfig["lte"]["resource_block_num"].get<int>();
  m_lteUeMeasurementInterval = MilliSeconds(jsonConfig["lte"]["measurement_interval_ms"].get<int>());
  Time lte_guard = MilliSeconds(jsonConfig["lte"]["measurement_guard_interval_ms"].get<int>());

  std::cout << "Config loaded from 'env-configure.json':"<< std::endl
            << "simulation_time_s = " << m_stopTime.GetSeconds() <<  std::endl
            << "random_seed = " << random_seed <<  std::endl
            << "downlink = " << m_downlink <<  std::endl
            << "EnableRtsCts = " << m_enableRtsCts <<  std::endl
            << "max_wait_time_for_action_ms = " << m_action_wait_ms <<  std::endl
            << "num_users = " << m_numOfUsers <<  std::endl;

  std::cout << "user_random_walk_speed = [" << m_userSpeedMin << "|" << m_userSpeedMax<< "]" << std::endl
            << "user_random_walk_direction = [" << m_userDirectionMin << "|" << m_userDirectionMax<< "]" << std::endl
            << "user_random_walk_distance = " << m_userWalkDistance << std::endl
            << "measurement_start_time_ms = " << m_measurement_start_time_ms <<  std::endl
            << "[GMA] enable_dynamic_flow_prioritization = " << m_dfp << std::endl
            << "[GMA] mx_algorithm = " << m_splittingAlgorithm << ""

            << "[GMA] measurement_interval_ms = " << gma_interval.GetMilliSeconds() <<  std::endl
            << "[GMA] measurement_guard_interval_ms = " << gma_guard.GetMilliSeconds() <<  std::endl

            << "[Wi-Fi] measurement_interval_ms = " << m_wifiMeasurementInterval.GetMilliSeconds() <<  std::endl
            << "[Wi-Fi] measurement_guard_interval_ms = " <<  wifi_guard.GetMilliSeconds() << std::endl
            
            << "[LTE] resource_block_num = " << m_lteRb <<  std::endl
            << "[LTE] measurement_interval_ms = " << m_lteUeMeasurementInterval.GetMilliSeconds() <<  std::endl
            << "[LTE] measurement_guard_interval_ms = " <<  lte_guard.GetMicroSeconds() << std::endl;
  
  Config::SetDefault ("ns3::GmaRxControl::EnableQosFlowPrioritization", BooleanValue (m_dfp));



  Config::SetDefault ("ns3::GmaVirtualInterface::MeasurementInterval", TimeValue (gma_interval));
  Config::SetDefault ("ns3::GmaVirtualInterface::MeasurementGuardInterval", TimeValue (gma_guard));

  Config::SetDefault (m_apManager+"::MeasurementInterval", TimeValue (m_wifiMeasurementInterval));
  Config::SetDefault (m_apManager+"::MeasurementGuardInterval", TimeValue (wifi_guard));

  //Config::SetDefault ("ns3::NsPfFfMacScheduler::MeasurementInterval", TimeValue (m_lteUeMeasurementInterval));
  //Config::SetDefault ("ns3::NsPfFfMacScheduler::MeasurementGuardInterval", TimeValue (lte_guard));
}

void
GmaSimWorker::SaveConfigFile()
{
  std::ostringstream fileName;
  fileName <<"config.txt";
  std::ofstream myfile;
  myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);

  if(m_radioType == WIFI_CID)
  {
    Config::SetDefault ("ns3::GmaTxControl::SplittingAlgorithm", EnumValue (GmaTxControl::FixedHighPriority));//Tx use default link
    Config::SetDefault ("ns3::GmaRxControl::SplittingAlgorithm", EnumValue (GmaRxControl::FixedHighPriority));//Rx use default link, no tsu update
    Config::SetDefault ("ns3::LinkState::HighPriorityLinkCid", UintegerValue (WIFI_CID));//default over wifi
    Config::SetDefault ("ns3::LinkState::FixDefaultLink", BooleanValue (true));//never update default link

    myfile << "Radio: wifi only" << std::endl;
    for (uint32_t i = 0; i < m_perSliceConfigList.size(); i++)
    {
      m_perSliceConfigList.at(i)->m_dlGmaMode = "wifi";
      m_perSliceConfigList.at(i)->m_ulGmaMode = "wifi";
    }

  }
  else if(m_radioType == CELLULAR_LTE_CID)
  {
    Config::SetDefault ("ns3::GmaTxControl::SplittingAlgorithm", EnumValue (GmaTxControl::FixedHighPriority));//Tx use default link
    Config::SetDefault ("ns3::GmaRxControl::SplittingAlgorithm", EnumValue (GmaRxControl::FixedHighPriority));//Rx use default link, no tsu update
    Config::SetDefault ("ns3::LinkState::HighPriorityLinkCid", UintegerValue (CELLULAR_LTE_CID));//default over LTE
    Config::SetDefault ("ns3::LinkState::FixDefaultLink", BooleanValue (true));//never update default link

    myfile << "Radio: lte Only" << std::endl;
    for (uint32_t i = 0; i < m_perSliceConfigList.size(); i++)
    {
      m_perSliceConfigList.at(i)->m_dlGmaMode = "lte";
      m_perSliceConfigList.at(i)->m_ulGmaMode = "lte";
    }

  }
  else if(m_radioType == CELLULAR_NR_CID)
  {
    Config::SetDefault ("ns3::GmaTxControl::SplittingAlgorithm", EnumValue (GmaTxControl::FixedHighPriority));//Tx use default link
    Config::SetDefault ("ns3::GmaRxControl::SplittingAlgorithm", EnumValue (GmaRxControl::FixedHighPriority));//Rx use default link, no tsu update
    Config::SetDefault ("ns3::LinkState::HighPriorityLinkCid", UintegerValue (CELLULAR_NR_CID));//default over NR
    Config::SetDefault ("ns3::LinkState::FixDefaultLink", BooleanValue (true));//never update default link

    myfile << "Radio: nr Only" << std::endl;
    for (uint32_t i = 0; i < m_perSliceConfigList.size(); i++)
    {
      m_perSliceConfigList.at(i)->m_dlGmaMode = "nr";
      m_perSliceConfigList.at(i)->m_ulGmaMode = "nr";
    }

  }
  else
  {
    Config::SetDefault ("ns3::LinkState::HighPriorityLinkCid", UintegerValue (CELLULAR_LTE_CID));//default link over lte
    Config::SetDefault ("ns3::LinkState::LowPriorityLinkCid", UintegerValue (WIFI_CID));//low priority link over wifi
    Config::SetDefault ("ns3::LinkState::FixDefaultLink", BooleanValue (false));//never update default link
    Config::SetDefault ("ns3::LinkState::PreferLargeCidValue", BooleanValue(true));//larger cid -> high priority
  
    myfile << "Radio: all network" << std::endl;
    for (uint32_t i = 0; i < m_perSliceConfigList.size(); i++)
    {
      if(m_perSliceConfigList.at(i)->m_dlGmaMode == "auto")
      {
        //change dl mode for auto mode
        if(m_perSliceConfigList.at(i)->m_tcpData && m_downlink)
        {
          m_perSliceConfigList.at(i)->m_dlGmaMode = "split";
        }
        else
        {
          m_perSliceConfigList.at(i)->m_dlGmaMode = "steer";
        }
        myfile <<" Slice: " << +i << " Downlink GMA mode: auto(config as " << m_perSliceConfigList.at(i)->m_dlGmaMode << ")" << std::endl;

      }
      else
      {
        myfile <<" Slice: " << +i << " Downlink GMA mode:" << m_perSliceConfigList.at(i)->m_dlGmaMode << std::endl;
      }

      if(m_perSliceConfigList.at(i)->m_ulGmaMode == "auto")
      {
        //change ul mode for auto mode
        if(m_perSliceConfigList.at(i)->m_tcpData && !m_downlink)
        {
          m_perSliceConfigList.at(i)->m_ulGmaMode = "split";
        }
        else
        {
          m_perSliceConfigList.at(i)->m_ulGmaMode = "steer";
        }
        myfile <<" Slice: " << +i << " Uplink GMA mode: auto(config as " << m_perSliceConfigList.at(i)->m_ulGmaMode << ")" << std::endl;
      }
      else
      {
        myfile <<" Slice: " << +i << " Uplink GMA mode:" << m_perSliceConfigList.at(i)->m_ulGmaMode << std::endl;
      }
    }

  }


  if(m_downlink)
  {
    myfile <<"Downlink Traffic" << std::endl;
  }
  else
  {
    myfile << "Uplink Traffic" << std::endl;
  }

  for (uint32_t i = 0; i < m_perSliceConfigList.size(); i++)
  {
    if(m_perSliceConfigList.at(i)->m_tcpData)
    {
      myfile <<" Slice: " << +i << " Traffic: high throughput (TCP) - " << m_perSliceConfigList.at(i)->m_transport_protocol << std::endl;

    }
    else
    {
      myfile <<" Slice: " << +i << " Traffic: low-latency (UDP)" << std::endl;
      myfile <<" Slice: " << +i << " Min Rate per user: " << m_perSliceConfigList.at(i)->m_minUdpRateMbps << " mbps" << std::endl;
      myfile <<" Slice: " << +i << " Max Rate per user: " << m_perSliceConfigList.at(i)->m_maxUdpRateMbps << " mbps" << std::endl;
    }
    myfile <<" Slice: " << +i << " Packet size: " << m_perSliceConfigList.at(i)->m_packetSize << " bytes" << std::endl;
  }

  if(m_splittingAlgorithm.compare("gma") == 0)
  {
    m_splittingBurst = 32;
  }
  else if (m_splittingAlgorithm.compare("gma2") == 0)
  {
    m_splittingBurst = 0; //use 0 to enable adaptive splitting burst.
  }
  else
  {
    NS_FATAL_ERROR("GMA mx_algorithm only support gma or gma2.");
  }

  if(m_enableRtsCts)
  {
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
    myfile << "Wi-Fi RTS/CTS enabled!" << std::endl;
  }
  Config::SetDefault ("ns3::PhyAccessControl::SimulateLinkDownTime", TimeValue(MilliSeconds(m_wifiDownTime)));
  myfile << "Wi-Fi down time after AP switch "<< m_wifiDownTime <<" ms!" << std::endl;

  Config::SetDefault ("ns3::TcpSocket::RcvBufSize", UintegerValue (1 << 24));
  Config::SetDefault ("ns3::TcpSocket::SndBufSize", UintegerValue (1 << 24));
  //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName ("ns3::TcpCubic")));
  int wifiDelay = 1000;
  Config::SetDefault ("ns3::WifiMacQueue::MaxDelay", TimeValue (MilliSeconds (wifiDelay)));
  int wifiQueueSize = 500;
  Config::SetDefault ("ns3::WifiMacQueue::MaxSize", QueueSizeValue (QueueSize (std::to_string(wifiQueueSize) + "p")));
  //Config::SetDefault ("ns3::WifiPhy::CcaEdThreshold", DoubleValue (-100.0));
  //Config::SetDefault ("ns3::WifiMacQueue::DropPolicy", EnumValue(WifiMacQueue::DROP_OLDEST));


  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
  Config::SetDefault ("ns3::GmaVirtualInterface::WifiLowPowerThresh", DoubleValue (m_wifiLowPowerThresh));
  Config::SetDefault ("ns3::GmaVirtualInterface::WifiHighPowerThresh", DoubleValue (m_wifiHighPowerThresh));
  Config::SetDefault ("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue (1024 * 1024));
  Config::SetDefault ("ns3::LteRlcUm::ReorderingTimer", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::LteRlcAm::MaxTxBufferSize", UintegerValue (1024 * 1024));

  Config::SetDefault ("ns3::LteAmc::AmcModel", EnumValue (LteAmc::PiroEW2010));
  Config::SetDefault ("ns3::StaWifiMac::MaxMissedBeacons", UintegerValue (100));
  
  Config::SetDefault ("ns3::ArpCache::WaitReplyTimeout", TimeValue (MilliSeconds (10)));
  Config::SetDefault ("ns3::ArpCache::MaxRetries", UintegerValue (100));
  Config::SetDefault ("ns3::ArpCache::AliveTimeout", TimeValue (Seconds (1000)));
  Config::SetDefault ("ns3::ArpCache::DeadTimeout", TimeValue (Seconds (1)));

  Simulator::Schedule(MilliSeconds(m_measurement_start_time_ms+1), &GmaDataProcessor::StartMeasurement, m_gmaDataProcessor);
  m_gmaDataProcessor->SetMaxPollTime(m_action_wait_ms);

  //Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (30.0));
  //Config::SetDefault ("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue (false));

  //2 layers mimo
  //Config::SetDefault ("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue (2)); // MIMO Spatial Multiplexity (2 layers) ONLY for Downlink...

  //configure he guard interval
  Config::SetDefault ("ns3::HeConfiguration::GuardInterval", TimeValue (NanoSeconds(1600))); //1600 or 800?
  Config::SetDefault ("ns3::GmaRxControl::DelayThresh", DoubleValue (m_gmaDelaySplitThresh));
  Config::SetDefault ("ns3::GmaRxControl::EnableStableAlgorithm", BooleanValue (true));


  Config::SetDefault ("ns3::PhyAccessControl::RoamingLowRssi", DoubleValue(m_wifiApSwitchPowerThresh));

  myfile << "Wi-Fi Power AP switch:" << m_wifiApSwitchPowerThresh <<"dBm; Low Thresh:" << m_wifiLowPowerThresh << "dBm; High thresh:" << m_wifiHighPowerThresh <<"dBm" << std::endl;
  myfile << "Num of users: " << m_numOfUsers << std::endl;
  myfile << "User speed min: " << m_userSpeedMin << " m/s" << std::endl;
  myfile << "User speed max: " << m_userSpeedMax << " m/s" << std::endl;
  myfile << "User direction min: " << m_userDirectionMin << " m/s" << std::endl;
  myfile << "User direction max: " << m_userDirectionMax << " m/s" << std::endl;
  myfile << "User walk distance: " << m_userWalkDistance << "m" << std::endl;
  myfile << "Simulation time: " << m_stopTime.GetSeconds() << " seconds" << std::endl;
  myfile << "D (Wi-Fi AP Grid size): " << m_distance << " meters" << std::endl;
  //myfile << "Acceptable edge owd: " << m_qosDelayRequirementMs << " ms" << std::endl;
  myfile << "Wi-Fi backhaul delay: " << m_wifiBackhaulDelay << std::endl;
  myfile << "GMA split algorithm delay thresh: " << m_gmaDelaySplitThresh << " ms" << std::endl;

  myfile << "Wi-Fi Queue MaxDelay:" << wifiDelay << " ms" << ", Queue MaxSize:" << wifiQueueSize << " packets" << std::endl;
  myfile << "LTE UM mode, buffer size: 1MB, t-reordering: 10 ms" << std::endl;

  myfile.close();

}

void
GmaSimWorker::InstallNetworks()
{
  for (int clientInd = 0; clientInd < m_numOfUsers; clientInd++)
  {
    //store the virtual IPs for clients.
    std::ostringstream virtualIp;
    virtualIp << "50.1.1." << 101+clientInd;//start from 50.1.1.101
    m_clientVirtualIpList.push_back(Ipv4Address(virtualIp.str ().c_str()));
  }

  //1. Create all Nodes.
  m_server = CreateObject<Node> ();

  //NodeContainer m_eNodeBs;
  m_eNodeBs.Create(m_numOfLteEnbs);
  m_nrEnbNodes.Create(m_numOfNrEnbs);

  //NodeContainer m_apNodes;
  m_apNodes.Create(m_numOfAps);

  //NodeContainer m_clientNodes;
  m_clientNodes.Create(m_numOfUsers);

  //set backhaul delay for lte
  Config::SetDefault ("ns3::NoBackhaulEpcHelper::S5LinkDelay", TimeValue (MilliSeconds(m_lteBackhaulDelayMs)));

  m_epcHelper = CreateObject<PointToPointEpcHelper> ();
  m_router = m_epcHelper->GetPgwNode ();
  //set backhaul delay for nr
  Config::SetDefault ("ns3::NoBackhaulEpcHelper::S5LinkDelay", TimeValue (MilliSeconds(m_nrBackhaulDelayMs)));

  m_nrEpcHelper = CreateObject<NrPointToPointEpcHelper> ();
  m_nrRouter = m_nrEpcHelper->GetPgwNode();

  //2. Add mobility
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (-10.0, 0.0, 0.0)); //m_server
  positionAlloc->Add (Vector (0.0, 0.0, 0.0)); //lte gateway
  positionAlloc->Add (Vector (0.0, 0.0, 0.0)); //nr gateway

  //LTE EnB locations
  positionAlloc->Add (m_enbLocations);

  //NR EnB locations
  for (int apInd = 0; apInd < m_numOfNrEnbs; apInd++)
  {
    positionAlloc->Add (m_nrEnbLocationVector.at(apInd));
  }
  //AP locations
  for (int apInd = 0; apInd < m_numOfAps; apInd++)
  {
    positionAlloc->Add (m_apLocationVector.at(apInd));
  }

  for (int clientInd = 0; clientInd < m_numOfUsers; clientInd++)
  {
    positionAlloc->Add (m_ueLocationVector.at(clientInd));
  }

  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (m_server);
  mobility.Install (m_router);
  mobility.Install (m_nrRouter);
  mobility.Install (m_eNodeBs);
  mobility.Install (m_nrEnbNodes);

  mobility.Install (m_apNodes);
  if(m_userSpeedMax > 0)
  {
    std::string speedString = "ns3::UniformRandomVariable[Min="+std::to_string(m_userSpeedMin)+"|Max="+std::to_string(m_userSpeedMax)+"]";
    std::string directionString = "ns3::UniformRandomVariable[Min="+std::to_string(m_userDirectionMin)+"|Max="+std::to_string(m_userDirectionMax)+"]";
    mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Mode",
        StringValue("Distance"),
        "Distance",
        DoubleValue(m_userWalkDistance),
        "Speed",
        StringValue(speedString),
        "Direction",
        StringValue(directionString),
        "Bounds",
        RectangleValue(Rectangle(m_user_min_x, std::max(m_user_max_x, m_user_min_x+1),
         m_user_min_y, std::max(m_user_min_y+1, m_user_max_y))));
    mobility.Install (m_clientNodes);

    /*MobilityHelper ueMobility;
    ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install (m_clientNodes);
    for (int i = 0; i < m_numOfUsers; i++)
      {
        Vector vec = m_ueLocationVector.at(i);
        m_clientNodes.Get (i)->GetObject<MobilityModel> ()->SetPosition (Vector (vec.x, vec.y, vec.z));
        m_clientNodes.Get (i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (Vector (m_userSpeed*(-1), 0, 0));

        //Simulator::Schedule (Seconds (40), &ChangeSpeed, m_clientNodes.Get (i));
    }*/
  }
  else
  { 
    mobility.Install (m_clientNodes);
  }
  

  //3. install internet.
  InternetStackHelper internet;
  internet.Install (m_server);
  internet.Install (m_nrEnbNodes);
  internet.Install (m_apNodes);
  internet.Install (m_clientNodes);
  
  //4. Install WiFi network
  // Point-to-point links
  // We create the channels first without any IP addressing information
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("10Gb/s"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));

  // Wifi network
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy;
  phy.SetChannel (channel.Create ());

  //4 wifi antenas
  //phy.Set ("Antennas", UintegerValue (2));
  //phy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (2));
  //phy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (2));

  WifiHelper wifi;

  wifi.SetStandard (WIFI_STANDARD_80211ax);
  if (m_fixWifiRate)
  {
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", 
                                  "DataMode", StringValue ("HtMcs7"), 
                                  "ControlMode", StringValue ("HtMcs0"));
  }
  else
  {
    //wifi.SetRemoteStationManager ("ns3::MinstrelHtWifiManager");
    wifi.SetRemoteStationManager (m_apManager);
    
  }
  WifiMacHelper mac;

  NodeContainer nSnR = NodeContainer (m_server, m_router);
  m_dSdR = p2p.Install (nSnR);

    // Later, we add IP addresses.
  Ipv4AddressHelper ipv4Server;
  ipv4Server.SetBase ("50.1.1.0", "255.255.255.0");
  m_iSiR = ipv4Server.Assign (m_dSdR);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;

  //TODO: check if we still need the route at server......
  //Create static routes from m_server to client
  //Ptr<Ipv4StaticRouting> staticRoutingS = ipv4RoutingHelper.GetStaticRouting (m_server->GetObject<Ipv4> ());
  // add the client network to the m_server routing table
  //staticRoutingS->AddNetworkRouteTo (Ipv4Address ("50.0.0.0"), Ipv4Mask ("255.0.0.0"), m_server->GetNDevices()-1);
  //staticRoutingS->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), m_server->GetNDevices()-1);
  //staticRoutingS->AddNetworkRouteTo (Ipv4Address ("17.0.0.0"), Ipv4Mask ("255.0.0.0"), m_server->GetNDevices()-1);

  std::vector<Ipv4InterfaceContainer> iAPList;

  // Create static routes from m_router to AP
  Ptr<Ipv4StaticRouting> staticRoutingR = ipv4RoutingHelper.GetStaticRouting (m_router->GetObject<Ipv4> ());

  // Create static routes from client to m_router
  Ipv4AddressHelper ipv4;
  NetDeviceContainer staDevices, apDevice;

  std::ostringstream fileName;
  fileName <<"config.txt";
  std::ofstream myfile;
  myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
  myfile << "AP Band Num: ";

  for (int apInd = 0; apInd < m_numOfAps; apInd++)
  {

    p2p.SetChannelAttribute ("Delay", StringValue (m_wifiBackhaulDelay));
 
    NodeContainer nRnAP = NodeContainer (m_router, m_apNodes.Get (apInd));
    NetDeviceContainer dRdAP = p2p.Install (nRnAP);

    // Later, we add IP addresses.
    std::ostringstream subnet;
    subnet << "50.2." << apInd+1 << ".0";
    ipv4.SetBase (subnet.str ().c_str (), "255.255.255.0");
    m_iRiAPList.push_back (ipv4.Assign (dRdAP));

    Ssid ssid = Ssid ("network-"+std::to_string(apInd+1));
    phy.SetChannel (channel.Create ());
    //phy.Set ("ChannelNumber", UintegerValue (36+8*(apInd%4)));
    //phy.Set ("ChannelWidth", UintegerValue (20));

    int bandNum = 36;

    if(!m_wifiShareBand)
    {
      bandNum = 36+4*apInd;
    }

    std::string channelStr = "{"+std::to_string(bandNum)+", 20, BAND_5GHZ, 0}";
    myfile << channelStr << " ";
    phy.Set ("ChannelSettings", StringValue (channelStr));
    mac.SetType ("ns3::StaWifiMac", "QosSupported", BooleanValue(true),
                 "Ssid", SsidValue (ssid));
    staDevices = wifi.Install (phy, mac, m_clientNodes);
    m_stationDeviceList.push_back(staDevices);

    mac.SetType ("ns3::ApWifiMac", "QosSupported", BooleanValue(true),
                    "Ssid", SsidValue (ssid));
    //              "Ssid", SsidValue (ssid),
    //              "BeaconInterval", TimeValue (Seconds (0.512)));
    apDevice = wifi.Install (phy, mac, m_apNodes.Get (apInd));
    // Later, we add IP addresses.
    std::ostringstream subnetWifi;
    subnetWifi << "50.3." << apInd+1 << ".0";
    ipv4.SetBase (subnetWifi.str ().c_str (), "255.255.255.0");

    iAPList.push_back (ipv4.Assign (apDevice));
    m_iCList.push_back (ipv4.Assign (staDevices));
    m_apDeviceList.Add(apDevice);

    //add default route for wifi client IP
    staticRoutingR->AddNetworkRouteTo (Ipv4Address (subnetWifi.str ().c_str ()), Ipv4Mask ("255.255.255.0"), m_router->GetNDevices() - 1);

    //add default route for m_router IP
    std::ostringstream routerIp;
    routerIp << "50.2." << apInd+1 << ".1";
    std::ostringstream apIp;
    apIp << "50.3." << apInd+1 << ".1";
    for (int clientInd = 0; clientInd < m_numOfUsers; clientInd++)
    {
      Ptr<Ipv4StaticRouting> staticRoutingC = ipv4RoutingHelper.GetStaticRouting (m_clientNodes.Get (clientInd)->GetObject<Ipv4> ());
      staticRoutingC->AddHostRouteTo (Ipv4Address (routerIp.str ().c_str()), Ipv4Address (apIp.str ().c_str()), m_clientNodes.Get (clientInd)->GetNDevices()-1);
    }
  }
  myfile<<std::endl;
  myfile.close();


  //5. LTE network
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  //lteHelper->SetSchedulerAttribute ("UlCqiFilter", EnumValue (FfMacScheduler::PUSCH_UL_CQI));

  //lteHelper->SetHandoverAlgorithmType ("ns3::A2A4RsrqHandoverAlgorithm");
  //lteHelper->SetHandoverAlgorithmAttribute ("ServingCellThreshold",
  //                                          UintegerValue (30));
  //lteHelper->SetHandoverAlgorithmAttribute ("NeighbourCellOffset",
  //                                          UintegerValue (1));

  lteHelper->SetEpcHelper (m_epcHelper);

  //lteHelper->SetPathlossModelType(TypeId::LookupByName("ns3::LogDistancePropagationLossModel"));
  //lteHelper->SetPathlossModelAttribute("Exponent", DoubleValue(3.9));
  //lteHelper->SetPathlossModelAttribute("ReferenceLoss",
  //                                      DoubleValue(38.57)); // ref. loss in dB at 1m for 2.025GHz
  //lteHelper->SetPathlossModelAttribute("ReferenceDistance", DoubleValue(1));

  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (m_lteRb));
  lteHelper->SetEnbDeviceAttribute ("UlBandwidth", UintegerValue (m_lteRb));
  lteHelper->SetSchedulerAttribute ("HarqEnabled",  BooleanValue (true));

  //lteHelper->SetSchedulerType ("ns3::NsPfFfMacScheduler");  

  lteHelper->SetEnbDeviceAttribute ("UlEarfcn", UintegerValue (23850));//815MHz
  lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (5850));//860MHz

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (m_eNodeBs);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (m_clientNodes);
  // Install the IP stack on the UEs
  m_ueIpIface = m_epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

  for (uint32_t u = 0; u < m_clientNodes.GetN (); ++u)
  {
    Ptr<Node> ueNode = m_clientNodes.Get (u);
    // Set the default gateway for the UE
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
    ueStaticRouting->AddHostRouteTo (m_epcHelper->GetUeDefaultGatewayAddress (), m_epcHelper->GetUeDefaultGatewayAddress (), ueNode->GetNDevices()-1);
  }

    // Attach one UE per eNodeB
  //for (uint16_t i = 0; i < m_numOfUsers; i++)
  //{
  //    lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(0));
      // side effect: the default EPS bearer will be activated
  //}
   lteHelper->AttachToClosestEnb (ueLteDevs, enbLteDevs);

  //lteHelper->EnableTraces();
  lteHelper->AddX2Interface (m_eNodeBs);
  //PopulateARPcache();

  //add action callbacks for network slicing scheduler.
  /*for (uint64_t d = 0; d < enbLteDevs.GetN (); d++)
  {
    auto ccMap = DynamicCast<LteEnbNetDevice> (enbLteDevs.Get(d))->GetCcMap();
    for (uint32_t i = 0; i < ccMap.size(); i++)
    {
      auto scheduler = DynamicCast<ComponentCarrierEnb>(ccMap.at(i))->GetFfMacScheduler();
      auto nsScheduler = DynamicCast<NsPfFfMacScheduler>(scheduler);
      //std::cout << i << " ccMap.at(i): " << ccMap.at(i) << " scheduler:" << scheduler << " nsScheduler:"<< nsScheduler << std::endl;
      //nsScheduler->SetGmaDataProcessor(m_gmaDataProcessor);
      auto cellId = enbLteDevs.Get(d)->GetObject<LteEnbNetDevice>()->GetCellId();
      m_gmaDataProcessor->SetNetworkGymActionCallback("lte::drb_allocation", cellId, MakeCallback (&ns3::NsPfFfMacScheduler::ReceiveDrbAllocation, nsScheduler));//dedicated rbs
      m_gmaDataProcessor->SetNetworkGymActionCallback("lte::prb_allocation", cellId, MakeCallback (&ns3::NsPfFfMacScheduler::ReceivePrbAllocation, nsScheduler));//prioritized rbs
      m_gmaDataProcessor->SetNetworkGymActionCallback("lte::srb_allocation", cellId, MakeCallback (&ns3::NsPfFfMacScheduler::ReceiveSrbAllocation, nsScheduler));//shared rbs

    }
  }*/

  //add two radio bearers, one for NON-GBR (best effort DSCP marking) and one of GBR (Voice DSCP marking)
  //need to enable QoS aware scheduler to identify different radio bearers. (Unfortunately, the CQA scheduler only surrpot DL)

  for (int u = 0; u < m_numOfUsers; u++)
  {

    int numOfBears = 2;

    for (int b=0; b<numOfBears; b++)
    {
      Ptr<EpcTft> tft = Create<EpcTft>();
      EpcTft::PacketFilter pktFilter;
      GbrQosInformation qos;
      qos.mbrDl = UINT32_MAX; //indicate network slice ID active
      qos.gbrDl = m_sliceIdList.at(u) 
                      + (m_sliceList.at(m_sliceIdList.at(u))->m_dedicatedRbg << 8)
                      + (m_sliceList.at(m_sliceIdList.at(u))->m_prioritizedRbg << 16)
                      + (m_sliceList.at(m_sliceIdList.at(u))->m_sharedRbg << 24)
                      + ((uint64_t)(u+1) << 32); //imsi
      if(b == 0)
      {
        pktFilter.typeOfService = m_beTos;
        pktFilter.typeOfServiceMask = 0xe0; //3 MSB
        tft->Add(pktFilter);
        EpsBearer bearer(EpsBearer::NGBR_VIDEO_TCP_DEFAULT, qos);
        lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(u), bearer, tft);
      }
      else if (b == 1)
      {
        pktFilter.typeOfService = 0xb8; //AC_VI
        pktFilter.typeOfServiceMask = 0xe0; //3 MSB
        tft->Add(pktFilter);
        EpsBearer bearer(EpsBearer::GBR_CONV_VIDEO, qos);
        lteHelper->ActivateDedicatedEpsBearer(ueLteDevs.Get(u), bearer, tft);
      }
      else
      {
        NS_FATAL_ERROR("only add 2 EpsBearers");
      }
    }
  }
  

  //6. Install NR network.

  if(m_nrEnbNodes.GetN() > 0)
  {
    //connect lte pgw to nr pgw
    p2p.SetChannelAttribute ("Delay", StringValue ("0ms"));
  
    NodeContainer nRnNr= NodeContainer (m_router, m_nrRouter);
    NetDeviceContainer dRdNr = p2p.Install (nRnNr);

    // Later, we add IP addresses. Make  sure to use the same subnet of the nr packet gate way. e.g., *17.0.0.0*, such that we do not need to setup the ip route within NR.
    ipv4.SetBase ("17.17.0.0", "255.255.255.0");
    m_iRiNr = ipv4.Assign (dRdNr);

    staticRoutingR->AddNetworkRouteTo (Ipv4Address ("17.0.0.0"), Ipv4Mask ("255.0.0.0"), m_router->GetNDevices() - 1);

    //Ptr<Ipv4StaticRouting> staticRoutingNr = ipv4RoutingHelper.GetStaticRouting (m_nrRouter->GetObject<Ipv4> ());
    //staticRoutingNr->AddNetworkRouteTo (Ipv4Address ("50.0.0.0"), Ipv4Mask ("255.0.0.0"), m_nrRouter->GetNDevices() - 1);


    double frequency = 3.5e9;      // central frequency
    double bandwidth = 20e6;     // bandwidth
    double txPower = 40; // txPower
    /*
    * Create NR simulation helpers
    */
    //m_idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    m_nrHelper = CreateObject<NrHelper>();
    //m_nrHelper->SetBeamformingHelper(m_idealBeamformingHelper);
    m_nrHelper->SetEpcHelper(m_nrEpcHelper);

    /*
    * Spectrum configuration. We create a single operational band and configure the scenario.
    */
    BandwidthPartInfoPtrVector allBwps, bwps0, bwps1;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1; // in this example we have a single band, and that band is
                                    // composed of a single component carrier

    CcBwpCreator::SimpleOperationBandConf bandConf0(frequency,
                                                    bandwidth,
                                                    numCcPerBand,
                                                    BandwidthPartInfo::UMa);

    CcBwpCreator::SimpleOperationBandConf bandConf1(frequency+bandwidth,
                                                    bandwidth,
                                                    numCcPerBand,
                                                    BandwidthPartInfo::UMa);
    OperationBandInfo band0 = ccBwpCreator.CreateOperationBandContiguousCc(bandConf0);
    OperationBandInfo band1 = ccBwpCreator.CreateOperationBandContiguousCc(bandConf1);

    // Initialize channel and pathloss, plus other things inside band.
    m_nrHelper->InitializeOperationBand(&band0);
    m_nrHelper->InitializeOperationBand(&band1);

    allBwps = CcBwpCreator::GetAllBwps({band0, band1});
    bwps0 = CcBwpCreator::GetAllBwps({band0});
    bwps1 = CcBwpCreator::GetAllBwps({band1});

    // Configure ideal beamforming method
    //m_idealBeamformingHelper->SetAttribute("BeamformingMethod",
    //                                      TypeIdValue(DirectPathBeamforming::GetTypeId()));

    // Configure scheduler
    m_nrHelper->SetSchedulerTypeId(NrMacSchedulerTdmaRR::GetTypeId());

    // Antennas for the UEs
    m_nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(1));
    m_nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(1));
    m_nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    // Antennas for the gNbs
    m_nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(1));
    m_nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(1));
    m_nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                      PointerValue(CreateObject<IsotropicAntennaModel>()));

    //m_nrHelper->SetGnbPhyAttribute("N2Delay", UintegerValue(0));
    //m_nrHelper->SetGnbPhyAttribute("N1Delay", UintegerValue(0));

    // install nr net devices
    if(m_nrEnbNodes.GetN() != 2)
    {
      NS_FATAL_ERROR("This environment is for two 5G NR base stations.");
    }
    NetDeviceContainer nrEnbNetDev = m_nrHelper->InstallGnbDevice(m_nrEnbNodes.Get(0), bwps0);
    nrEnbNetDev.Add(m_nrHelper->InstallGnbDevice(m_nrEnbNodes.Get(1), bwps1));
    NetDeviceContainer nrUeNetDev;

    for (auto it = m_clientNodes.Begin(); it != m_clientNodes.End(); ++it)
    {
      Vector uepos = (*it)->GetObject<MobilityModel>()->GetPosition();
      double minDistance = std::numeric_limits<double>::infinity();
      int closestEnbDevice = -1;
      int bandId = 0;
      for (NetDeviceContainer::Iterator i = nrEnbNetDev.Begin(); i != nrEnbNetDev.End(); ++i)
      {
          Vector enbpos = (*i)->GetNode()->GetObject<MobilityModel>()->GetPosition();
          double distance = CalculateDistance(uepos, enbpos);
          if (distance < minDistance)
          {
              minDistance = distance;
              closestEnbDevice = bandId;
          }
          bandId++;
      }
      if (closestEnbDevice == 0)
      {
        nrUeNetDev.Add(m_nrHelper->InstallUeDevice((*it), bwps0));
      }
      else if (closestEnbDevice == 1)
      {
        nrUeNetDev.Add(m_nrHelper->InstallUeDevice((*it), bwps1));
      }
      else
      {
        NS_FATAL_ERROR("unkown band id");
      }

      
    }


    int64_t randomStream = 1;
    randomStream += m_nrHelper->AssignStreams(nrEnbNetDev, randomStream);
    randomStream += m_nrHelper->AssignStreams(nrUeNetDev.Get(0), randomStream);
    randomStream += m_nrHelper->AssignStreams(nrUeNetDev.Get(1), randomStream);

    for (uint32_t i = 0; i < nrEnbNetDev.GetN(); i++)
    {
      m_nrHelper->GetGnbPhy(nrEnbNetDev.Get(i), 0)->SetTxPower(txPower);
    }
    // When all the configuration is done, explicitly call UpdateConfig ()
    for (auto it = nrEnbNetDev.Begin(); it != nrEnbNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }

    for (auto it = nrUeNetDev.Begin(); it != nrUeNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }


    //NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (m_eNodeBs);
    //NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (m_clientNodes);
    // Install the IP stack on the UEs
    m_nrUeIpIface = m_nrEpcHelper->AssignUeIpv4Address (NetDeviceContainer (nrUeNetDev));

    for (uint32_t u = 0; u < m_clientNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = m_clientNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->AddHostRouteTo (m_nrEpcHelper->GetUeDefaultGatewayAddress (), m_nrEpcHelper->GetUeDefaultGatewayAddress (), ueNode->GetNDevices()-1);
      //ueStaticRouting->AddHostRouteTo (m_epcHelper->GetUeDefaultGatewayAddress (), m_nrEpcHelper->GetUeDefaultGatewayAddress (), ueNode->GetNDevices()-1);

    }

    // attach UEs to the closest eNB
    m_nrHelper->AttachToClosestEnb(nrUeNetDev, nrEnbNetDev);
    //m_nrHelper->EnableTraces();
  }

}

void
GmaSimWorker::InstallGmaInterface()
{
  // Assign IP address to UEs, and install applications
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  int startTimeCount = 0;
  int startTimeDiff = 5;
  //install GMA to the m_router
  m_routerGma = CreateObject<GmaProtocol>(m_router);
  //add the virtual m_server IP to the m_router GMA
  m_routerGma->AddLocalVirtualInterface (m_iSiR.GetAddress (0), m_dSdR.Get(1)); //this can add all virtual IP of the same subnet

  for (int clientInd = 0; clientInd < m_numOfUsers; clientInd++)
  {
    startTimeCount += startTimeDiff;
    Ptr<GmaVirtualInterface> routerInterface;

    if(m_radioType == WIFI_CID || m_radioType == CELLULAR_LTE_CID || m_radioType == CELLULAR_NR_CID)
    {
      routerInterface = m_routerGma->CreateGmaTxVirtualInterface(MilliSeconds(500+startTimeCount));
    }
    else
    {
      //create an virtual interface per client
      routerInterface = m_routerGma->CreateGmaVirtualInterface(MilliSeconds(500+startTimeCount));
    }
    routerInterface->EnableServerRole(clientInd+1); //use imsi as client id
    m_gmaDataProcessor->AddClientId(clientInd+1); //use imsi as client id
    //routerInterface->AggregateObject(m_gmaDataProcessor);
    routerInterface->SetGmaDataProcessor(m_gmaDataProcessor);

    //get the slice id for a users.
    auto sliceId = m_sliceIdList.at(clientInd);

    if (sliceId >= (int)m_perSliceConfigList.size())
    {
      NS_FATAL_ERROR("slice ID: " << sliceId << " not find in m_perSliceConfigList, size: " << m_perSliceConfigList.size() );
    }
    
    if (m_perSliceConfigList.at(sliceId)->m_dlGmaMode == "wifi")
    {
      routerInterface->SetFixedTxCid(WIFI_CID);
    }
    else if (m_perSliceConfigList.at(sliceId)->m_dlGmaMode == "lte")
    {
      routerInterface->SetFixedTxCid(CELLULAR_LTE_CID);
    }
    else if (m_perSliceConfigList.at(sliceId)->m_dlGmaMode == "nr")
    {
      routerInterface->SetFixedTxCid(CELLULAR_NR_CID);
    }
    else if (m_perSliceConfigList.at(sliceId)->m_dlGmaMode == "duplicate")
    {
      routerInterface->SetDuplicateMode(true);
    }
    //other mode nothing to config

    if(m_perSliceConfigList.at(sliceId)->m_dlGmaMode == "split")
    {
      routerInterface->ConfigureTxAlgorithm(m_splittingAlgorithm, m_splittingBurst);//this will send TSU to allow the other side to split traffic
    }
    else if (m_perSliceConfigList.at(sliceId)->m_dlGmaMode == "steer")
    {
      routerInterface->ConfigureTxAlgorithm(m_splittingAlgorithm, 1);//this will send TSU to allow the other side to split traffic
    }

    if(m_perSliceConfigList.at(sliceId)->m_ulGmaMode == "split")
    {
      routerInterface->ConfigureRxAlgorithm(m_splittingAlgorithm, m_splittingBurst);//this will send TSU to allow the other side to split traffic
    }
    else if (m_perSliceConfigList.at(sliceId)->m_ulGmaMode == "steer")
    {
      routerInterface->ConfigureRxAlgorithm(m_splittingAlgorithm, 1);//this will send TSU to allow the other side to split traffic
    }
    //the other mode, no thing to config...
    routerInterface->SetQosRequirement(sliceId, m_perSliceConfigList.at(sliceId)->m_qosTestDurationMs, m_perSliceConfigList.at(sliceId)->m_qosDelayRequirementMs, 
          m_perSliceConfigList.at(sliceId)->m_delaythresh1, 
          m_perSliceConfigList.at(sliceId)->m_delaythresh2, 
          m_perSliceConfigList.at(sliceId)->m_qosDelayViolationTarget,
          m_perSliceConfigList.at(sliceId)->m_qosLossTarget);


    //set the virtual IP of the clients to the m_router GMA
    m_routerGma->AddRemoteVirIp (routerInterface, m_clientVirtualIpList.at(clientInd), m_clientNodes.Get(clientInd)->GetId());
    /*for (int apInd = 0; apInd < m_numOfAps; apInd++)
    {
      //add available PHY link IPs to the virtual interface
      m_routerGma->AddRemotePhyIpCandidate (m_clientVirtualIpList.at(clientInd), m_iCList.at(apInd).GetAddress(clientInd), 0, apInd);// add Wi-Fi link, cid 0
    }*/
    /*for (int apInd = 0; apInd < m_numOfAps; apInd++)
    {
      //add available PHY link IPs to the virtual interface
      m_routerGma->AddRemotePhyIp (routerInterface, m_iCList.at(apInd).GetAddress(clientInd), apInd); //add Wi-Fi links, cid 0
    }*/
    
    // routerInterface->GetMeasurementManager()->SetMeasurementCallback(MakeCallback (&GmaMeasurementServerReport));

    int apInd = -1;
    if(m_numOfAps>0)
    {
      apInd = GetClosestWifiAp(m_clientNodes.Get(clientInd));

      //set the initial client ip, for intra-rat handover, the ip will be updated by probes from client...
      m_routerGma->AddRemotePhyIp (m_clientVirtualIpList.at(clientInd), m_iCList.at(apInd).GetAddress(clientInd), WIFI_CID);// add Wi-Fi link
    }

    if(m_nrEnbNodes.GetN() > 0)
    {
      //add nr IP
      m_routerGma->AddRemotePhyIp (m_clientVirtualIpList.at(clientInd), m_nrUeIpIface.GetAddress(clientInd),  CELLULAR_NR_CID); // add NR linke
    }
    //add lte IP
    m_routerGma->AddRemotePhyIp (m_clientVirtualIpList.at(clientInd), m_ueIpIface.GetAddress(clientInd), CELLULAR_LTE_CID); // add LTE link

    //add a GMA to each client
    Ptr<GmaProtocol> clientGma = CreateObject<GmaProtocol> (m_clientNodes.Get(clientInd));

    //add the client virtual IP to the client GMA
    clientGma->AddLocalVirtualInterface (m_clientVirtualIpList.at(clientInd));
    Ptr<GmaVirtualInterface> clientInterface;

    if(m_radioType == WIFI_CID || m_radioType == CELLULAR_LTE_CID || m_radioType == CELLULAR_NR_CID)
    {
      clientInterface = clientGma->CreateGmaTxVirtualInterface(MilliSeconds(100+startTimeCount));
    }
    else
    {
      //create an virtual interface
      clientInterface = clientGma->CreateGmaVirtualInterface(MilliSeconds(100+startTimeCount));
    }
    clientInterface->EnableClientRole (clientInd+1); //use imsi as client id
    clientInterface->SetGmaDataProcessor(m_gmaDataProcessor);

    if (m_perSliceConfigList.at(sliceId)->m_ulGmaMode == "wifi")
    {
      clientInterface->SetFixedTxCid(WIFI_CID);
    }
    else if (m_perSliceConfigList.at(sliceId)->m_ulGmaMode == "lte")
    {
      clientInterface->SetFixedTxCid(CELLULAR_LTE_CID);
    }
    else if (m_perSliceConfigList.at(sliceId)->m_ulGmaMode == "nr")
    {
      clientInterface->SetFixedTxCid(CELLULAR_NR_CID);
    }
    else if (m_perSliceConfigList.at(sliceId)->m_ulGmaMode == "duplicate")
    {
      clientInterface->SetDuplicateMode(true);//reordering will be enabled at the receiver since this is flow ID 3
    }
    //other mode nothing to config

    if(m_perSliceConfigList.at(sliceId)->m_ulGmaMode == "split")
    {
      clientInterface->ConfigureTxAlgorithm(m_splittingAlgorithm, m_splittingBurst);//this will send TSU to allow the other side to split traffic
    }
    else if (m_perSliceConfigList.at(sliceId)->m_ulGmaMode == "steer")
    {
      clientInterface->ConfigureTxAlgorithm(m_splittingAlgorithm, 1);//this will send TSU to allow the other side to split traffic
    }

    if(m_perSliceConfigList.at(sliceId)->m_dlGmaMode == "split")
    {
      clientInterface->ConfigureRxAlgorithm(m_splittingAlgorithm, m_splittingBurst);//this will send TSU to allow the other side to split traffic
    }
    else if (m_perSliceConfigList.at(sliceId)->m_dlGmaMode == "steer")
    {
      clientInterface->ConfigureRxAlgorithm(m_splittingAlgorithm, 1);//this will send TSU to allow the other side to split traffic
    }
    //the other mode, no thing to config...

    clientInterface->SetQosRequirement(sliceId, m_perSliceConfigList.at(sliceId)->m_qosTestDurationMs, m_perSliceConfigList.at(sliceId)->m_qosDelayRequirementMs,
          m_perSliceConfigList.at(sliceId)->m_delaythresh1, 
          m_perSliceConfigList.at(sliceId)->m_delaythresh2, 
          m_perSliceConfigList.at(sliceId)->m_qosDelayViolationTarget,
          m_perSliceConfigList.at(sliceId)->m_qosLossTarget);

    //add the virtual IP of the m_server to client GMA
    clientGma->AddRemoteVirIp(clientInterface, m_iSiR.GetAddress (0), m_router->GetId());

    /*for (int apInd = 0; apInd < m_numOfAps; apInd++)
    {
      //add available PHY link IPs to the virtual interface
      clientGma->AddRemotePhyIp (clientInterface, m_iRiAPList.at(apInd).GetAddress (0), apInd);// add Wi-Fi link, cid 0
    }*/
    //apInd = GetClosestWifiAp(m_clientNodes.Get(clientInd), m_apNodes);


    if (m_numOfAps > 0)
    {
      clientGma->AddRemotePhyIp (m_iSiR.GetAddress (0), m_iRiAPList.at(apInd).GetAddress (0), WIFI_CID, apInd+m_wifi_cell_id_offset);// add Wi-Fi link, cid 0
    }

    for (int intraId = 0; intraId < m_numOfAps; intraId++)
    {
      //add available PHY link IPs to the virtual interface
      Ptr<WifiNetDevice> wifiApDev = DynamicCast<WifiNetDevice> (m_apDeviceList.Get(intraId));
      Mac48Address macAddr = wifiApDev->GetMac ()->GetAddress ();
      clientGma->AddRemotePhyIpCandidate (m_iSiR.GetAddress (0), m_iRiAPList.at(intraId).GetAddress (0), macAddr, WIFI_CID, intraId+m_wifi_cell_id_offset);// add Wi-Fi link

      //Ptr<WifiNetDevice> wifiDev = DynamicCast<WifiNetDevice>(m_stationDeviceList.at(intraId).Get(clientInd));
      //comment for now...
      //wifiDev->GetPhy()->SetPeriodicPowerCallback (MakeCallback (&GmaVirtualInterface::WifiPeriodicPowerTrace, clientInterface), 0, intraId);

    }

    if (m_wifiHandover)
    {
      auto nodeId = m_clientNodes.Get(clientInd)->GetId();
      Config::ConnectFailSafe("/NodeList/"+std::to_string(nodeId)+"/DeviceList/*/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/MonitorSnifferRx",
                                      MakeCallback(&GmaVirtualInterface::MonitorSniffRx, clientInterface));
    }

    if(m_nrEnbNodes.GetN() > 0)
    {
      clientGma->AddRemotePhyIp (m_iSiR.GetAddress (0), m_iRiNr.GetAddress (0), CELLULAR_NR_CID);// add NR link
    }
    clientGma->AddRemotePhyIp (m_iSiR.GetAddress (0), m_epcHelper->GetUeDefaultGatewayAddress (), CELLULAR_LTE_CID); // add LTE link

    m_clientGmaList.push_back(clientGma);

    //Simulator::Schedule (Seconds(1.0), &UpdateWifiLinkIp, apInd, clientInd, m_clientVirtualIpList.at(clientInd), clientGma, m_iSiR.GetAddress (0), clientInterface);
  }
}

void
GmaSimWorker::InstallApplications()
{
  // Create the OnOff application to send UDP datagrams of size
  // 210 bytes at a rate of 448 Kb/s

  std::ostringstream fileName;
  fileName <<"config.txt";
  std::ofstream myfile;
  myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);

  ApplicationContainer sendApps;
  ApplicationContainer sinkApps;

  for (int clientInd = 0; clientInd < m_numOfUsers; clientInd++)
  {
    uint16_t port = clientInd + 9;   // Discard port (RFC 863)

    Ipv4Address receiverAddress;

    if(m_downlink)
    {
      receiverAddress = m_clientVirtualIpList.at(clientInd);
    }
    else
    {
      receiverAddress = m_iSiR.GetAddress (0);
    }

    auto sliceId = m_sliceIdList.at(clientInd);

    if (sliceId >= (int)m_perSliceConfigList.size())
    {
      NS_FATAL_ERROR("slice ID: " << sliceId << " not find in m_perSliceConfigList, size: " << m_perSliceConfigList.size() );
    }

    if(m_perSliceConfigList.at(sliceId)->m_tcpData)
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(m_perSliceConfigList.at(sliceId)->m_transport_protocol, &tcpTid),
                          "TypeId " << m_perSliceConfigList.at(sliceId)->m_transport_protocol << " not found");
      Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                        TypeIdValue(TypeId::LookupByName(m_perSliceConfigList.at(sliceId)->m_transport_protocol)));

      InetSocketAddress socketAddr = InetSocketAddress (receiverAddress, port);
      socketAddr.SetTos(m_beTos + (sliceId<<2)); //3MSB: tos, 3bits: sliceID, 2LSB: ECN
      AddressValue remoteAddress (socketAddr);
      Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (m_perSliceConfigList.at(sliceId)->m_packetSize));
      BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
      ftp.SetAttribute ("Remote", remoteAddress);
      ftp.SetAttribute ("SendSize", UintegerValue (m_perSliceConfigList.at(sliceId)->m_packetSize));
      if (m_downlink)
      {
        sendApps.Add(ftp.Install (m_server));
      }
      else
      {
        sendApps.Add(ftp.Install (m_clientNodes.Get(clientInd)));
      }
    }
    else
    {
      // onoff ("ns3::UdpSocketFactory", 
      //                   Address (InetSocketAddress (receiverAddress, port)));
      //onoff.SetConstantRate (DataRate (udpRate));
      //onoff.SetAttribute("PacketSize", UintegerValue (m_packetSize));
      //UdpClientHelper onoff (receiverAddress, port);

      InetSocketAddress socketAddr = InetSocketAddress (receiverAddress, port);
      socketAddr.SetTos(m_beTos + (sliceId<<2)); //3MSB: tos, 3bits: sliceID, 2LSB: ECN
      PoissonUdpClientHelper onoff (socketAddr);
      onoff.SetAttribute ("MaxPackets", UintegerValue (UINT32_MAX));
      double udpRateMbps = m_uniformRv->GetInteger(m_perSliceConfigList.at(sliceId)->m_minUdpRateMbps, m_perSliceConfigList.at(sliceId)->m_maxUdpRateMbps);
      myfile << "UE " << clientInd << " rate: "<< udpRateMbps << " mbps"<< std::endl;

      double interval = (double)m_perSliceConfigList.at(sliceId)->m_packetSize*8*1e-6/udpRateMbps;
      onoff.SetAttribute ("Interval", TimeValue (Seconds(interval)));
      onoff.SetAttribute ("PacketSize", UintegerValue (m_perSliceConfigList.at(sliceId)->m_packetSize));
      onoff.SetAttribute ("PoissonArrival", BooleanValue (m_perSliceConfigList.at(sliceId)->m_poissonArrival));

      if (m_downlink)
      {
        sendApps.Add(onoff.Install (m_server));
      }
      else
      {
        sendApps.Add(onoff.Install (m_clientNodes.Get(clientInd)));
      }
    }

    if(m_perSliceConfigList.at(sliceId)->m_tcpData)
    {
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(m_perSliceConfigList.at(sliceId)->m_transport_protocol, &tcpTid),
                          "TypeId " << m_perSliceConfigList.at(sliceId)->m_transport_protocol << " not found");
      Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                        TypeIdValue(TypeId::LookupByName(m_perSliceConfigList.at(sliceId)->m_transport_protocol)));
      InetSocketAddress socketAddr = InetSocketAddress (Ipv4Address::GetAny (), port);
      socketAddr.SetTos(m_beTos + (sliceId<<2)); //3MSB: tos, 3bits: sliceID, 2LSB: ECN
      PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
                     Address (socketAddr));
      sinkHelper.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
      if (m_downlink)
      {
        sinkApps.Add(sinkHelper.Install (m_clientNodes.Get(clientInd)));
      }
      else
      {
        sinkApps.Add(sinkHelper.Install (m_server));
      }

    }
    else
    {
      // Create a packet sink to receive these packets
      //PacketSinkHelper sink ("ns3::UdpSocketFactory",
      //                       Address (InetSocketAddress (Ipv4Address::GetAny (), port)));

      UdpServerHelper sink (port);

      if (m_downlink)
      {
        sinkApps.Add(sink.Install (m_clientNodes.Get(clientInd)));
      }
      else
      {
        sinkApps.Add(sink.Install (m_server));
      }
    }

    if(m_enableRxTrace)
    {

      std::ostringstream fileName;
      fileName<<"data-"<<clientInd<<".txt";

      AsciiTraceHelper asciiTraceHelper;
      Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream (fileName.str ().c_str ());
      if (m_perSliceConfigList.at(sliceId)->m_tcpData)
      {
        sinkApps.Get(clientInd)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&RcvFrom, stream));
      }
      else
      {
        sinkApps.Get(clientInd)->TraceConnectWithoutContext("Rx",MakeBoundCallback (&Rcv, stream));
      }
    }

    //std::ostringstream fileNameA;
    //fileNameA<<"data"<<clientInd+1<<".txt";

    //AsciiTraceHelper asciiTraceHelperA;
    //Ptr<OutputStreamWrapper> streamA = asciiTraceHelperA.CreateFileStream (fileNameA.str ().c_str ());
    //sinkApps.Get(clientInd+1)->TraceConnectWithoutContextFailSafe("Rx",MakeBoundCallback (&RxFrom, streamA));
    if(m_perSliceConfigList.at(sliceId)->m_tcpData && m_enableTCPtrace)
    {
      if(m_downlink)
      {
        Simulator::Schedule (Seconds (1.001), &DonwlinkTraces, clientInd);
        //Simulator::Schedule (Seconds (30*clientInd+1.001), &DonwlinkTraces, clientInd);
      }
      else
      {
        Simulator::Schedule (Seconds (1.001), &UplinkTraces, clientInd);
        //Simulator::Schedule (Seconds (30*clientInd+1.001), &UplinkTraces, clientInd);
      }
    }

    //sendApps.Get(clientInd)->SetStartTime (Seconds (30*clientInd+1));
    double startTimeGap = 0.001;
    sendApps.Get(clientInd)->SetStartTime (MilliSeconds (m_measurement_start_time_ms) + Seconds(startTimeGap*clientInd));
    //double stopTimeGap = 0.1;
    //sendApps.Get(clientInd)->SetStopTime (m_stopTime - Seconds(1) - Seconds(stopTimeGap*(m_numOfUsers-clientInd-1)));
  }

  //sendApps.Start (Seconds (1.0));
  sendApps.Stop (m_stopTime);
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (m_stopTime);
  myfile.close();
}

void
GmaSimWorker::ConnectTraceCallbacks()
{
  if(m_numOfAps > 0)
  {
    Config::ConnectFailSafe("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/RemoteStationManager/$" +
                                  m_apManager + "/RateMeasurement",
                              MakeCallback(&GmaSimWorker::WifiRateCallback, this));
  }

    //Config::ConnectFailSafe("/NodeList/*/DeviceList/*/ComponentCarrierMap/*/FfMacScheduler/$ns3::NsPfFfMacScheduler/LteEnbMeasurement",
    //                          MakeCallback(&GmaSimWorker::LteEnbMeasurementCallback, this));
    //Config::ConnectFailSafe("/NodeList/*/DeviceList/*/ComponentCarrierMap/*/FfMacScheduler/$ns3::NsPfFfMacScheduler/LteUeMeasurement",
    //                          MakeCallback(&GmaSimWorker::LteUeMeasurementCallback, this));
    Config::Connect("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                  MakeCallback (&GmaSimWorker::NotifyConnectionEstablished, this));
}

void
GmaSimWorker::StoreMacAddress()
{
  for (uint64_t i = 0; i < m_stationDeviceList.size(); i++)
  {
    NetDeviceContainer deviceContainer = m_stationDeviceList.at(i);
    {
      for (uint64_t dev = 0; dev < deviceContainer.GetN (); dev++)
      {
        Ptr<WifiNetDevice> wifiStaDevice = DynamicCast<WifiNetDevice> (deviceContainer.Get(dev));
        Mac48Address addr = wifiStaDevice->GetMac ()->GetAddress ();
        //if((int)i == GetClosestWifiAp(m_clientNodes.Get(dev)))
        //{
          m_macAddrToUserMap[addr] = dev + 1;//use imsi
          m_macAddrToCellMap[addr] = i + m_wifi_cell_id_offset;
          std::cout << "AP: " << i << " <--> User: " << dev <<" MAC Address:" << addr << " (handover supported)"<< std::endl;
        //}
      }
    }
  }
}

void
GmaSimWorker::Run()
{
  //AsciiTraceHelper ascii;;
  //p2p.EnableAsciiAll (ascii.CreateFileStream ("gma.tr"));
  //p2p.EnablePcapAll ("gma");
  //phy.EnableAsciiAll (ascii.CreateFileStream ("gma-wifi.tr"));
  //phy.EnablePcapAll ("gma-wifi");

  Simulator::Stop (m_stopTime+MicroSeconds(2)); //for sending the last measurement.
  Simulator::Run ();
  Simulator::Destroy ();
  std::cout << "Simulation end at " << m_stopTime.GetSeconds() << "s" << std::endl;
}

void
GmaSimWorker::TrafficSplittingDeployment ()
{

  ParseJsonConfig();

  SaveConfigFile();

  InstallNetworks();

  InstallGmaInterface();

  InstallApplications();

  LogLocations ();

  ConnectTraceCallbacks();

  StoreMacAddress();
  
  Run();

}

void
GmaSimWorker::QoSTrafficSteeringDeployment()
{
  
  ParseJsonConfig();

  SaveConfigFile();

  InstallNetworks();

  InstallGmaInterface();

  InstallApplications();

  LogLocations ();

  ConnectTraceCallbacks();

  StoreMacAddress();
  
  Run();
}

int 
main (int argc, char *argv[])
{

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Ptr<GmaSimWorker> worker = CreateObject<GmaSimWorker>();
  try
  {
    std::ifstream jsonStream("env-configure.json");
    json jsonConfig;
    jsonStream >> jsonConfig;
    std::string env_name = jsonConfig["env"].get<std::string>();
    std::cout << "env = " << env_name <<  std::endl;

    if(env_name.compare("nqos_split") == 0)
    {
      worker->TrafficSplittingDeployment();
    }
    else
    {
      NS_FATAL_ERROR("["+env_name+"] use case not implemented in ns3 workers");
    }
  }
  catch (const json::type_error& e)
  {
      // output exception information
      std::cout << "message: " << e.what() << '\n'
                << "exception id: " << e.id << std::endl;
  }

  return 0;
}