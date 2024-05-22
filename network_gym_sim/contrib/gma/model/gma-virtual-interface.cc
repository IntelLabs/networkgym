/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "gma-virtual-interface.h"
#include "ns3/mobility-module.h"
#include <iomanip>
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GmaVirtualInterface");

NS_OBJECT_ENSURE_REGISTERED (GmaVirtualInterface);


GmaVirtualInterface::GmaVirtualInterface (Time startTime)
{
	NS_LOG_FUNCTION (this);
	m_linkState = CreateObject<LinkState>();
	m_gmaRxControl = CreateObject<GmaRxControl> ();
	m_gmaRxControl->SetLinkState(m_linkState);
	m_forwardPacketCallback = MakeNullCallback<void, Ptr<Packet> > ();
	m_reorderingTimeoutEvent.Cancel();
	m_stopReorderEvent.Cancel();
	m_periodicProbeEvent.Cancel();
	m_ctrRto = INITIAL_CONTROL_RTO;

	m_gmaTxControl = CreateObject<GmaTxControl> ();
	m_gmaTxControl->SetLinkState(m_linkState);

    m_periodicProbeEvent = Simulator::Schedule(startTime, &GmaVirtualInterface::PeriodicProbeMsg, this);//start probe at different time for each instance, otherwise may cause problem for wifi...

    m_txMode = true;
    m_rxMode = true;
    m_flowParam = Create<MeasureParam>();
    m_enableReordering = false;
    m_receiveDuplicateFlow = false;

}

void
GmaVirtualInterface::SetGmaDataProcessor(Ptr<GmaDataProcessor> database)
{
	m_gmaDataProcessor = database;
	if (m_serverRoleEnabled)
	{

		m_gmaDataProcessor->SetNetworkGymActionCallback("gma::wifi::dl::priority", m_clientId, MakeCallback (&ns3::GmaVirtualInterface::ReceiveWifiDlDfpAction, this));
		m_gmaDataProcessor->SetNetworkGymActionCallback("gma::lte::dl::priority", m_clientId, MakeCallback (&ns3::GmaVirtualInterface::ReceiveLteDlDfpAction, this));
		m_gmaDataProcessor->SetNetworkGymActionCallback("gma::nr::dl::priority", m_clientId, MakeCallback (&ns3::GmaVirtualInterface::ReceiveNrDlDfpAction, this));

		//uplink not implemented yet.
	}
	else if (m_clientRoleEnabled)
	{
		m_gmaDataProcessor->SetNetworkGymActionCallback("gma::dl::split_weight", m_clientId, MakeCallback (&ns3::GmaVirtualInterface::ReceiveDlSplitWeightAction, this));

	}
	else
	{
		NS_FATAL_ERROR("GmaDataProcessor should be set after setting client or server role!");
	}
}

void
GmaVirtualInterface::ReceiveWifiDlDfpAction (const json& action)
{
	//std::cout << "wifi dl dfp:"<< action << std::endl;
	if (action == nullptr)
	{
		return;
	}
	if (m_gmaRxControl->QosFlowPrioritizationEnabled())
	{
		if(m_linkParamsMap.find(WIFI_CID) != m_linkParamsMap.end())
		{
			m_linkParamsMap[WIFI_CID]->m_qosMarking = action.get<int>();
		}
	}
}

void
GmaVirtualInterface::ReceiveLteDlDfpAction (const json& action)
{
	//std::cout << "lte dl dfp:"<< action << std::endl;
	if (action == nullptr)
	{
		return;
	}
	if (m_gmaRxControl->QosFlowPrioritizationEnabled())
	{
		if(m_linkParamsMap.find(CELLULAR_LTE_CID) != m_linkParamsMap.end())
		{
			m_linkParamsMap[CELLULAR_LTE_CID]->m_qosMarking = action.get<int>();
		}
	}
}

void
GmaVirtualInterface::ReceiveNrDlDfpAction (const json& action)
{
	//std::cout << "nr dl dfp:"<< action << std::endl;
	if (action == nullptr)
	{
		return;
	}
	if (m_gmaRxControl->QosFlowPrioritizationEnabled())
	{
		if(m_linkParamsMap.find(CELLULAR_NR_CID) != m_linkParamsMap.end())
		{
			m_linkParamsMap[CELLULAR_NR_CID]->m_qosMarking = action.get<int>();
		}
	}
}


void
GmaVirtualInterface::ReceiveDlSplitWeightAction (const json& action)
{
	//std::cout << "dl split weight:" << action << std::endl;
	if (action == nullptr)
	{
		if (m_gmaRxControl->QosSteerEnabled())
		{
			m_qosTestingAllowed = true; //start system default qos testing after empty action arrives.
		}

		m_missedAction++;
		return;
	}

	if(m_linkParamsMap.size() != action.size())
	{
		NS_FATAL_ERROR("the action size: " <<  action.size() << " is not equal the number of links:" << m_linkParamsMap.size());
	}

	
	//this is action for QoS splitting.
	if (m_gmaRxControl->QosSteerEnabled())
	{
		m_qosSteerActionReceived = true;
		//qos_steer
		//std::cout << "qos_steer" << std::endl;
 		int sumWeight = 0;
		for (uint32_t i = 0; i < action.size(); i++)
		{
			sumWeight += action[i].get<int>();
			//std::cout << action[i].get<int>() << std::endl;
		}
		
		if(sumWeight != 1)
		{
			NS_FATAL_ERROR("the sum of action (split_weight) must be one for QoS steer case!!");
		}

		uint8_t steerLinkCid = UINT8_MAX;
		auto iterLink = m_linkParamsMap.begin();
		uint32_t linkId = 0;
		while (iterLink != m_linkParamsMap.end())
		{	
			if(action[linkId].get<int>() == 1)
			{
				steerLinkCid = iterLink->first;
				break;
			}
			linkId++;
			iterLink++;
		}

		if (steerLinkCid == UINT8_MAX)
		{
			NS_FATAL_ERROR("No CID is found to have step == 1!");
		}
		//make sure the backuplink is correct!!

		if(steerLinkCid != m_linkState->GetHighPriorityLinkCid())
		{
			if(!m_measurementManager->GetObject<QosMeasurementManager>())
			{
				NS_FATAL_ERROR("QosMeasurementManager not initialed");
			}
			m_measurementManager->GetObject<QosMeasurementManager>()->QuitQosMeasurement(); //stop qos measurement for previous backup link
			//qos testing enabled for one of the none default link.
			//chekck the prohibit timer...
			auto iter = m_linkState->m_cidToLastTestFailTime.find(steerLinkCid);
			if (iter == m_linkState->m_cidToLastTestFailTime.end())
			{
				//no prohibit timer, qos testing will start...
				m_qosTestingAllowed = true;
			}
			else//we found a prohibit timer
			{
				if (Now() >= iter->second + m_linkState->MIN_QOS_TESTING_INTERVAL)//qos test prohibit timer expires
				{
					//prohibit timer expires, qost testing will start...
					m_qosTestingAllowed = true;
				}
				//else ignore this action.
			}

		}
		else
		{
			//steer to the default link.
			//stop qos measurement...
			if(!m_measurementManager->GetObject<QosMeasurementManager>())
			{
				NS_FATAL_ERROR("QosMeasurementManager not initialed");
			}
			m_measurementManager->GetObject<QosMeasurementManager>()->QuitQosMeasurement();
			m_qosTestingAllowed = false;
			Ptr<SplittingDecision> decision = m_gmaRxControl->GenerateTrafficSplittingDecision(m_linkState->GetHighPriorityLinkCid());//generate a tsu to switch traffic over default link
			if(decision->m_update)
			{
				SendTsu(decision);
			}
		}
	}
	else
	{
		//nqos_split
		//std::cout << "nqos_split" << std::endl;
		//convert the traffic split weight to step size per link.
		double sumWeight = 0;
		for (uint32_t i = 0; i < action.size(); i++)
		{
			sumWeight += action[i].get<double>();
		}
		
		if(sumWeight == 0)
		{
			NS_FATAL_ERROR("the sum of action (split_weight) cannot be zero!!");
		}

		//we will use the last link to take care of the rounding error.

		std::vector < uint8_t > burstPerLink;
		int burstSum = 0;
		for (uint32_t i = 0; i < action.size(); i++)
		{
			burstPerLink.push_back(std::round(action[i].get<double>()*m_gmaRxControl->GetMaxSplittingBurst()/sumWeight));
			burstSum += burstPerLink.at(i);

		}

		//std::cout << " burst 0: " << +burstPerLink.at(0) << " burst 1: " << +burstPerLink.at(1) << " burst 2: " << +burstPerLink.at(2) << std::endl;

		//take care of rounding error.

		//if total burst size is smaller than the required splitting burst size, increase 1 burst for the highest traffic link?
		//the reason for this it the lowest link may be zero, and it may be itentional, e.g., one link is never used.
		while(burstSum < m_gmaRxControl->GetMaxSplittingBurst())
		{
			std::cout << "sum smaller than splitting burst size!!!" << std::endl;
			auto index = std::distance(burstPerLink.begin(),std::max_element(burstPerLink.begin(), burstPerLink.end()));
			burstPerLink.at(index) += 1;
			burstSum += 1;
		}

		//if total burst size is smaller than the required splitting burst size, decrease 1 burst for the highest traffic link.
		while(burstSum > m_gmaRxControl->GetMaxSplittingBurst())
		{
			std::cout << "sum greater than splitting burst size!!!" << std::endl;
			auto index = std::distance(burstPerLink.begin(),std::max_element(burstPerLink.begin(), burstPerLink.end()));
			burstPerLink.at(index) -= 1;
			burstSum -= 1;
		}
		
		//std::cout << "Fix Rouding Error | burst 0: " << +burstPerLink.at(0) << " burst 1: " << +burstPerLink.at(1) << " burst 2: " << +burstPerLink.at(2) << std::endl;

		//assign the ml action to split ratio.
		m_gmaRxControl->SetAlgorithm("RlSplit");
		Ptr<RlAction> rlAction = Create<RlAction>();

		rlAction->m_links = m_linkParamsMap.size();
		auto iterLink = m_linkParamsMap.begin();
		while (iterLink != m_linkParamsMap.end())
		{	
			rlAction->m_cidList.push_back(iterLink->first);
			iterLink++;
		}
		rlAction->m_ratio = burstPerLink;

		uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
		Ptr<SplittingDecision> decision = m_gmaRxControl->GetTrafficSplittingDecision(rlAction);
		if(decision->m_update)
		{
			SendTsu(decision);
			if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
			{
				//split mode & default link cid changed, we need to send back a end marker...
				SendEndMarker(oldCid);
			}
		}
	}
}

void
GmaVirtualInterface::SetId (Ptr<Node> node, uint32_t nodeId, uint16_t InterfaceId)
{
	NS_LOG_FUNCTION (this);
	m_gmaInterfaceId = InterfaceId;
	m_nodeId = nodeId;
	m_node = node;
	m_linkState->SetId(m_nodeId);
	Simulator::Schedule(m_measurementGuardInterval, &GmaVirtualInterface::MeasurementGuardIntervalEnd, this);
}

void
GmaVirtualInterface::SetFixedTxCid(uint8_t cid)
{
	m_linkState->SetFixedDefaultCid(cid);
	m_gmaTxControl->SetAlgorithm("FixedHighPriority");
}


void
GmaVirtualInterface::SetTxMode(bool flag)
{
	m_txMode = flag;
}

void
GmaVirtualInterface::SetRxMode(bool flag)
{
	m_rxMode = flag;
}

TypeId
GmaVirtualInterface::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GmaVirtualInterface")
    .SetParent<Object> ()
    .SetGroupName("Gma")
    .AddAttribute ("UseLsnReordering",
           "If true, enable LSN to detect loss in reordering process at receiver.",
           BooleanValue (true),
           MakeBooleanAccessor (&GmaVirtualInterface::m_useLsnReordering),
           MakeBooleanChecker ())
    .AddAttribute ("EnableMeasureReport",
           "If true, enable measure report",
           BooleanValue (true),
           MakeBooleanAccessor (&GmaVirtualInterface::m_enableMeasureReport),
           MakeBooleanChecker ())
    .AddAttribute ("WifiLowPowerThresh",
                   "Report Wi-Fi link Weak if receive power (dBm) is below this value"
                   "and the previous report is link okey",
                   DoubleValue (-80.0),
                   MakeDoubleAccessor (&GmaVirtualInterface::SetWifiLowPowerThresh),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("WifiHighPowerThresh",
                   "Report Wi-Fi link Okey if receive power (dBm) is higher than this value"
                   "and the previous report is link weak",
                   DoubleValue (-77.0),
                   MakeDoubleAccessor (&GmaVirtualInterface::SetWifiHighPowerThresh),
                   MakeDoubleChecker<double> ())
	.AddAttribute ("MeasurementInterval",
                   "GMA measurement interval",
                   TimeValue(Seconds(1.0)),
                   MakeTimeAccessor (&GmaVirtualInterface::m_measurementInterval),
                   MakeTimeChecker ())
	.AddAttribute ("MeasurementGuardInterval",
                   "the guard interval between 2 measurements",
                   TimeValue(Seconds(0.0)),
                   MakeTimeAccessor (&GmaVirtualInterface::m_measurementGuardInterval),
                   MakeTimeChecker ())
  ;
  return tid;
}

void
GmaVirtualInterface::AddPhyLink(Ptr<Socket> socket, const Ipv4Address& phyAddr, uint8_t cid, int apId)
{
	//std::cout << Now().GetSeconds() << " addr: " << phyAddr  << "  cid: " << +cid << " apid:" << apId<< "\n";
	if(m_linkParamsMap.find(cid) == m_linkParamsMap.end())
	{
		Ptr<LinkParams> linkParams = Create<LinkParams>();
		linkParams->m_phyAccessContrl = CreateObject<PhyAccessControl>();
		linkParams->m_phyAccessContrl->SetSocket(socket);
		linkParams->m_phyAccessContrl->SetPortNum(START_PORT_NUM+cid);
		linkParams->m_phyAccessContrl->SetIp(phyAddr);
		linkParams->m_phyAccessContrl->SetApId(apId);

		m_linkParamsMap[cid] = linkParams;
		m_linkState->AddLinkCid(cid);
		if (m_linkState->GetHighPriorityLinkCid() != cid)//not high priority link
		{
			m_linkState->CtrlMsgDown(cid);//cannot use this link before the first probe or other ctrl msg is acked.
		}
		m_measurementManager->AddDevice(cid, m_acceptableDelay);
	}
	else
	{
		//this link already exist
		if(m_linkParamsMap[cid]->m_phyAccessContrl->GetIp() != phyAddr)
		{
			//std::cout << "AddPhyLink: update ip from " << m_linkParamsMap[cid]->m_phyAccessContrl->GetIp() <<" to "<< phyAddr << "\n";
			m_linkParamsMap[cid]->m_phyAccessContrl->SetSocket(socket);
			m_linkParamsMap[cid]->m_phyAccessContrl->SetIp(phyAddr, true);//change ip and set link down.
			m_linkParamsMap[cid]->m_phyAccessContrl->SetApId(apId);
			m_ctrRto = INITIAL_CONTROL_RTO; //reset rto timer;
			//IP changed -> Wi-Fi to Wi-Fi handover, we set the control msg failed... Do not use this link until at least one control msg is received from this link.
			uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
			if(m_linkParamsMap[cid]->m_phyAccessContrl->GetLinkDownTime() > Seconds(0))//simulate single radio...
			{
				if(m_linkState->CtrlMsgDown(cid))//cid updates
				{
					Ptr<SplittingDecision> decision = m_gmaRxControl->LinkDownTsu(cid);//chekc if we need tsu to notify the other side
					if(decision->m_update)
					{
						SendTsu(decision);
						if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
						{
							//split mode & default link cid changed, we need to send back a end marker...
							SendEndMarker(oldCid);
						}
					}
				}
			}
		}
		else
		{
			//std::cout << "same IP, do not update\n";
		}
	}

	if(m_gmaRxControl->QosFlowPrioritizationEnabled())
	{
		m_linkParamsMap[cid]->m_qosMarking = 1.0;//enable qos marking by default
	}

}

void
GmaVirtualInterface::AddPhyCandidate(Ptr<Socket> socket, const Ipv4Address& phyAddr,  const Mac48Address& macAddr, uint8_t cid, int apId)
{
	if(m_linkParamsMap.find(cid) == m_linkParamsMap.end())
	{
		Ptr<LinkParams> linkParams = Create<LinkParams>();
		linkParams->m_phyAccessContrl = CreateObject<PhyAccessControl>();
		linkParams->m_phyAccessContrl->SetSocket(socket);
		linkParams->m_phyAccessContrl->SetPortNum(START_PORT_NUM+cid);
		linkParams->m_phyAccessContrl->SetIp(phyAddr);
		linkParams->m_phyAccessContrl->SetApId(apId);

		m_linkParamsMap[cid] = linkParams;
		m_linkState->AddLinkCid(cid);
		if (m_linkState->GetHighPriorityLinkCid() != cid)//not high priority link
		{
			m_linkState->CtrlMsgDown(cid);//cannot use this link before the first probe or other ctrl msg is acked.
		}
		m_measurementManager->AddDevice(cid, m_acceptableDelay);
	}

	m_linkParamsMap[cid]->m_phyAccessContrl->AddCandidate(socket, phyAddr, macAddr, apId);


}

void 
GmaVirtualInterface::SetDuplicateMode (bool flag)
{
	m_duplicateMode = flag;
}

GmaVirtualInterface::~GmaVirtualInterface ()
{
	NS_LOG_FUNCTION (this);
}


void
GmaVirtualInterface::ConfigureTxAlgorithm(std::string algorithm, uint8_t splittingBurst)
{
	// splittingBurst == 0 -> enable dynamic configured splitting burst split.
	// splittingBurst == 1 -> steer
	// splittingBurst > 1 -> split (fixed splittingBurst)

	if (algorithm.compare("gma") == 0)
	{
		//do nothing.
	}
	else
    {
      NS_FATAL_ERROR("GMA mx_algorithm only support gma");
    }

}


void
GmaVirtualInterface::ConfigureRxAlgorithm(std::string algorithm, uint8_t splittingBurst)
{
	// splittingBurst == 0 -> enable dynamic configured splitting burst split.
	// splittingBurst == 1 -> steer
	// splittingBurst > 1 -> split (fixed splittingBurst)
	//we first configure links state and rx control.
	m_gmaRxControl->SetSplittingBurst(splittingBurst);
	if (algorithm.compare("gma") == 0)
	{
		//gma 1.0 algorithm.
		//split algorithm start from low priority and slowly move traffic to high priority.
		m_gmaRxControl->SetAttribute("SplittingAlgorithm", EnumValue (GmaRxControl::gma));
	}
	else
    {
      NS_FATAL_ERROR("GMA mx_algorithm only support gma");
    }

	//build the m_measurementManager here!
	if (m_gmaRxControl->QosSteerEnabled())
	{
		m_measurementFactory.SetTypeId("ns3::QosMeasurementManager");
	}
	else
	{
		m_measurementFactory.SetTypeId("ns3::MeasurementManager");
	}
	m_measurementManager = m_measurementFactory.Create()->GetObject<MeasurementManager> ();
	m_measurementManager->SetRxControlApp(m_gmaRxControl);
	m_measurementManager->SetSendTsuCallback (MakeCallback (&GmaVirtualInterface::SendTsu, this));
	if(m_rxMode == false)
	{
		//disable Prob message and measurement
		//m_periodicProbeEvent.Cancel(); we need probe for wifi roaming..
		m_measurementManager->DisableMeasurement();
	}

	if (algorithm.compare("gma") == 0)
	{
		//gma 1.0 algorithm.
		m_measurementManager->SetAttribute("MaxMeasureInterval", UintegerValue (10)); //gma requires measurement converge or max of 10 intervals.
		m_measurementManager->SetAttribute("EnableSenderSideOwdAdjustment", BooleanValue (false)); //gma equalize owd at the receiver side.
	}
	else if (algorithm.compare("gma2") == 0)
    {
		//gma 2.0 algorithm.
		if (splittingBurst == 1)
		{
			//qos aware steer
		}
		else
		{
			//split
			m_measurementManager->SetAttribute("MaxMeasureInterval", UintegerValue (1)); //gma2 does not the measurement interval to converge.
			m_measurementManager->SetAttribute("EnableSenderSideOwdAdjustment", BooleanValue (true));  //gma2 equalize owd at the transmitter side.
		}
    }
    else
    {
      NS_FATAL_ERROR("GMA mx_algorithm only support gma or gma2.");
    }
}


void
GmaVirtualInterface::MeasurementGuardIntervalEnd()
{
	//reset all measurements.
	m_numOfTxPacketsPerFlow = 0;
	m_TxBytesPerFlow = 0;
	m_numOfInOrderPacketsPerFlow = 0;
	m_numOfMissingPacketsPerFlow = 0;
	m_numOfAbnormalPacketsPerFlow = 0;
	m_flowParam = Create<MeasureParam>();

	m_measureParamPerCidMap.clear();
	m_receivedBytes = 0;

	m_reorderingTimeoutCounter = 0;
	m_tsuCounter = 0;
	Simulator::Schedule(m_measurementInterval, &GmaVirtualInterface::CollectMeasureResults, this);
}
void
GmaVirtualInterface::CollectMeasureResults()
{
	//compute our reordering timeout

	if (Now() > m_nextReorderingUpdateTime)
	{
		//here we compute the rerodering timeout from long measurement, e.g., > 10 seconds
		uint32_t maxOwd = m_measurementManager->GetMaxOwdMs();
		uint32_t minOwd = m_measurementManager->GetMinOwdMs();
		if(maxOwd!=0 && minOwd!=UINT32_MAX)
		{
			//reordering timeout equals 2* (max OWD - min OWD), it is also in the rage of [MIN..., MAX_REORDERING_TIMEOUT]
			m_reorderingTimeout = std::max(MIN_REORDERING_TIMEOUT, std::min(MAX_REORDERING_TIMEOUT, MilliSeconds(2*(maxOwd-minOwd))));
			//std::cout << Now().GetSeconds() << " Cal reordering timeout maxOwd:" << maxOwd 
			//<< "ms, min Owd:" << minOwd << "ms, timeout:" << m_reorderingTimeout.GetMilliSeconds() << "ms@@@@@@@@\n";

		}
	}

	//here we compute a reordering timout from a short measurement, if the short measurement has higher rto, we use the short measurement rto.
	Time newReorderingTimeout = m_reorderingTimeout;

	uint32_t maxOwd = 0;
	uint32_t minOwd = UINT32_MAX;

	std::map < uint8_t, Ptr<MeasureParam> >::iterator iter = m_measureParamPerCidMap.begin();
	while(iter!=m_measureParamPerCidMap.end())
	{
		if(maxOwd < iter->second->m_maxOwd)
		{
			maxOwd = iter->second->m_maxOwd;
		}

		if(minOwd > iter->second->m_minOwd)
		{
			minOwd = iter->second->m_minOwd;
		}
		iter++;
	}
	if(maxOwd!=0 && minOwd!=UINT32_MAX)
	{
		//reordering timeout equals 2* (max OWD - min OWD), it is also in the rage of [MIN..., MAX_REORDERING_TIMEOUT]
		if(m_measureParamPerCidMap.size() > 1)
		{
			newReorderingTimeout = std::max(MIN_REORDERING_TIMEOUT, std::min(MAX_REORDERING_TIMEOUT, MilliSeconds(2*(maxOwd-minOwd))));
			if(newReorderingTimeout > m_reorderingTimeout)
			{
				m_reorderingTimeout = newReorderingTimeout;
				//update reordering timeout to the bigger one.
				//std::cout << Now().GetSeconds() << " node: " << m_nodeId << " UPDATE reordering timeout maxOwd:" << maxOwd 
				//<< "ms, min Owd:" << minOwd << "ms, timeout:" << m_reorderingTimeout.GetMilliSeconds() << "ms@@@@@@@@\n";
			}
		}

	}

	if(m_enableMeasureReport)
	{
		//std::cout <<"node " << m_nodeId <<" write to measurement report now------------>\n";
		std::ofstream myfile;
		Time timeNow = Simulator::Now ();
		//uint64_t start_ts = (timeNow-m_measurementInterval).GetMilliSeconds();
		uint64_t end_ts =  timeNow.GetMilliSeconds();
		if(m_saveToFile)
		{
			//log file
			std::ostringstream fileName;
			if(m_txMode && m_rxMode)
			{
				fileName <<"node-"<<m_nodeId<<"-interface-"<<+m_gmaInterfaceId<<".csv";
			}
			else if(m_txMode && !m_rxMode)
			{
				//fileName <<"TX-mode-node-"<<m_nodeId<<"-interface-"<<+m_gmaInterfaceId<<".csv";
				fileName <<"node-"<<m_nodeId<<"-interface-"<<+m_gmaInterfaceId<<".csv";
			}
			else if(!m_txMode && m_rxMode)
			{
				//fileName <<"RX-mode-node-"<<m_nodeId<<"-interface-"<<+m_gmaInterfaceId<<".csv";
				fileName <<"node-"<<m_nodeId<<"-interface-"<<+m_gmaInterfaceId<<".csv";
			}
			else
			{
				fileName <<"node-"<<m_nodeId<<"-interface-"<<+m_gmaInterfaceId<<".csv";
			}
			myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
			if(m_fileTile == false)
			{
				myfile << "time,\tcid,\tAP,\tpowDbm,\tkbps,\tqosKbps,\trate%,\tminOwd,\taveOwd,\tmaxOwd,\tlossR,\tmissing,\toutOrdr,\tinOrder,\tLate,\ttimeout,\tisReord,\ttsu,\trevTx,\tx,\ty,\tz\n" ;
				m_fileTile = true;
			}
		}

		std::string directionStr = "";
		std::string revDirectionStr = "";
		if (m_clientRoleEnabled)
		{
			directionStr = "dl";
			revDirectionStr = "ul";
		}
		else if (m_serverRoleEnabled)
		{
			directionStr = "ul";
			revDirectionStr = "dl";
		}
		ns3::Ptr<ns3::NetworkStats> element = ns3::CreateObject<ns3::NetworkStats>("gma", m_clientId, end_ts);
		ns3::Ptr<ns3::NetworkStats> sliceElementSum = ns3::CreateObject<ns3::NetworkStats>("gma", m_clientId, end_ts);
		ns3::Ptr<ns3::NetworkStats> sliceElementMean = ns3::CreateObject<ns3::NetworkStats>("gma", m_clientId, end_ts);

		uint64_t aveOwd = 0;
		if(m_flowParam->m_count != 0)
		{
			aveOwd = m_flowParam->m_owdSum/m_flowParam->m_count;
		}
		uint64_t rate = 0;
		if (m_measurementInterval.GetMilliSeconds() == 0)
		{
			NS_FATAL_ERROR("Measurement Interval Cannot be Zero!");
		}
		if(m_flowParam->m_rcvBytes != 0 )
		{
			rate = m_flowParam->m_rcvBytes/(m_measurementInterval.GetMilliSeconds()) * 8; //kbps
		}

		if(m_lastDecision)
		{
			std::vector<uint8_t> cidList = m_linkState->GetCidList();
			for (uint8_t link = 0; link < m_lastDecision->m_splitIndexList.size(); link++)
			{
				std::string cidStr = LinkState::ConvertCidFormat(cidList.at(link));
				element->Append(cidStr+"::"+directionStr+"::split_ratio", 100.0*m_lastDecision->m_splitIndexList.at(link)/m_gmaRxControl->GetSplittingBurst());
			}
		}
		element->Append(directionStr+"::missed_action", m_missedAction);

		m_missedAction = 0;

		uint64_t per = 0;
		if(m_receivedBytes!=0)
		{
			per = 100*m_flowParam->m_rcvBytes/m_receivedBytes;
		}
		double timeSec = timeNow.GetSeconds () ;

		double delayVilation = 0;
		double t1Violation = 0;
		double t2Violation = 0;

		if(m_numOfAbnormalPacketsPerFlow + m_numOfInOrderPacketsPerFlow > 0)
		{
			delayVilation = 1.0*m_flowParam->m_numOfHighDelayPkt/(m_numOfAbnormalPacketsPerFlow + m_numOfInOrderPacketsPerFlow);
			t1Violation = 1.0*m_flowParam->m_numofT1DelayPkt/(m_numOfAbnormalPacketsPerFlow + m_numOfInOrderPacketsPerFlow);
			t2Violation = 1.0*m_flowParam->m_numofT2DelayPkt/(m_numOfAbnormalPacketsPerFlow + m_numOfInOrderPacketsPerFlow);
		}

		double lossFlow = 0;

		if (m_numOfMissingPacketsPerFlow > m_numOfAbnormalPacketsPerFlow && (m_numOfMissingPacketsPerFlow + m_numOfInOrderPacketsPerFlow) > 0)
		{
			lossFlow = 1.0*(m_numOfMissingPacketsPerFlow - m_numOfAbnormalPacketsPerFlow)/(m_numOfMissingPacketsPerFlow + m_numOfInOrderPacketsPerFlow);
		}

		double flowQosMet = 0;//the qos rate equals rate * flowQosMet.

		if (delayVilation <= m_gmaRxControl->m_qosDelayViolationTarget && lossFlow <= m_gmaRxControl->m_qosLossTarget)
		{
			flowQosMet = 1.0;
		}

		if(m_qosClientTestingActive)
		{
			element->Append(directionStr+"::measurement_ok", 0);
		}
		else
		{
			element->Append(directionStr+"::measurement_ok", 1);
		}

		element->Append(directionStr+"::rate", (double)rate/1e3);
		sliceElementSum->Append(directionStr+"::network::rate", (double)rate/1e3);
		element->Append(directionStr+"::qos_rate", (double)rate*flowQosMet/1e3);
		sliceElementSum->Append(directionStr+"::network::qos_rate", (double)rate*flowQosMet/1e3);

		element->Append(directionStr+"::delay_violation", std::round(delayVilation*100*1000)/1000);
		sliceElementMean->Append(directionStr+"::network::delay_violation", std::round(delayVilation*100*1000)/1000);

		element->Append(directionStr+"::delay_test_1_violation", std::round(t1Violation*100*1000)/1000);
		element->Append(directionStr+"::delay_test_2_violation", std::round(t2Violation*100*1000)/1000);
		
		if(m_saveToFile)
		{
			myfile << timeSec << ",\t" 
			<< -1 << ",\t"
			<< "null,\t"
			<< "null,\t"
			<< rate << ",\t" 
			<< rate*flowQosMet << ",\t"
			<< per <<",\t";
		}
		if(m_flowParam->m_minOwd!=UINT32_MAX)
		{
			if(m_saveToFile)
			{
				myfile<< m_flowParam->m_minOwd << ",\t"
				<< aveOwd << ",\t"
				<< m_flowParam->m_maxOwd << ",\t";
			}
			element->Append(directionStr+"::owd", aveOwd);
			element->Append(directionStr+"::max_owd", m_flowParam->m_maxOwd);

		}
		else
		{	
			if(m_saveToFile)
			{
				myfile<< "null" << ",\t"
				<< "null" << ",\t"
				<< "null" << ",\t";
			}
			element->Append(directionStr+"::owd", -1.0);
			element->Append(directionStr+"::max_owd", -1.0);
		}

		if(m_saveToFile)
		{
			myfile << lossFlow <<",\t"<<m_numOfMissingPacketsPerFlow<< ",\t"<<m_numOfAbnormalPacketsPerFlow << ",\t" 
				<< m_numOfInOrderPacketsPerFlow << ",\t" << m_flowParam->m_numOfHighDelayPkt <<  ",\t" 
				<< m_reorderingTimeoutCounter << ",\t" << m_enableReordering << ",\t" << m_tsuCounter << ",\t" << m_numOfTxPacketsPerFlow << ",\t";
		}

		//std::cout <<m_TxBytesPerFlow*8/m_measurementInterval.GetMilliSeconds()/1e3 << std::endl;
		//for tx_rate, it measures the reverse direction...
		element->Append(revDirectionStr+"::tx_rate", m_TxBytesPerFlow*8/m_measurementInterval.GetMilliSeconds()/1e3);
		sliceElementSum->Append(revDirectionStr+"::network::tx_rate", m_TxBytesPerFlow*8/m_measurementInterval.GetMilliSeconds()/1e3);
		if(m_node->GetObject<MobilityModel> ())
		{
			Vector pos = m_node->GetObject<MobilityModel> ()->GetPosition ();
			if(m_saveToFile)
			{
				myfile <<+pos.x <<",\t"<< +pos.y << ",\t" << +pos.z<< "\n";
			}

			if (m_clientRoleEnabled)
			{
				element->Append("x_loc", std::round(pos.x*1000)/1000);
				element->Append("y_loc", std::round(pos.y*1000)/1000);
			}

		}
		else
		{
			if(m_saveToFile)
			{
				myfile <<"null"<<",\t"<<"null"<< ",\t" <<"null"<< "\n";
			}

		}

		m_numOfTxPacketsPerFlow = 0;
		m_TxBytesPerFlow = 0;
		m_numOfInOrderPacketsPerFlow = 0;
		m_numOfMissingPacketsPerFlow = 0;
		m_numOfAbnormalPacketsPerFlow = 0;

		m_flowParam = Create<MeasureParam>();

		std::map < uint8_t, Ptr<LinkParams> >::iterator iterLink = m_linkParamsMap.begin();
		while(iterLink!=m_linkParamsMap.end())
		{
			uint8_t cid = iterLink->first;
			
			if(m_saveToFile)
			{
				myfile << timeSec << ",\t" << +cid << ",\t";

				if(cid == WIFI_CID)
				{
					uint16_t wifiCellId = m_linkParamsMap[cid]->m_phyAccessContrl->GetApId();
					if(wifiCellId == 255)
					{
						myfile << "null" << ",\t";
					}
					else
					{
						myfile << wifiCellId+1 << ",\t";
					}

					if(m_wifiPowerAvailable)
					{
						myfile << std::setprecision(4) << m_linkParamsMap[cid]->m_phyAccessContrl->GetCurrentApRssi() ;
					} 
					else
					{
						myfile<<"null" ;
					}
				}
				else
				{
					myfile << "null" << ",\t" << "null";

				}
			}


			std::string cidStr = LinkState::ConvertCidFormat(cid);
			element->Append(cidStr+"::"+revDirectionStr+"::priority", m_linkParamsMap[cid]->m_qosMarking);

			std::map < uint8_t, Ptr<MeasureParam> >::iterator iter = m_measureParamPerCidMap.find(cid);
			if(iter!=m_measureParamPerCidMap.end())
			{
				uint64_t linkrate = 0;

				if(iter->second->m_rcvBytes != 0 )
				{
					linkrate = iter->second->m_rcvBytes/(m_measurementInterval.GetMilliSeconds()) * 8; //kbps
				}

				double percent = 0;
				if(m_receivedBytes!=0)
				{
					percent = std::round(100.0*iter->second->m_rcvBytes/m_receivedBytes);
				}

				uint64_t inOrder = iter->second->m_numOfInOrderPacketsForReport;
				uint64_t missing = iter->second->m_numOfMissingPacketsForReport;
				uint64_t abnormal = iter->second->m_numOfAbnormalPacketsForReport;
				uint64_t highDelay = iter->second->m_numOfHighDelayPkt;

				//double linkDelayVilation = 0;

				//if(abnormal + inOrder > 0)
				//{
				//	linkDelayVilation = 1.0*highDelay/(abnormal + inOrder);
				//}

				double linkLoss = 0;

				if (missing > abnormal && (missing + inOrder) > 0)
				{
					linkLoss = 1.0*(missing - abnormal)/(missing + inOrder);
				}

				//double linkQosMet = 0;//the qos rate equals rate * linkQosMet.

				//if (linkDelayVilation <= m_gmaRxControl->m_qosDelayViolationTarget && linkLoss <= m_gmaRxControl->m_qosLossTarget)
				//{
				//	linkQosMet = 1.0;
				//}

				if(m_saveToFile)
				{
					myfile << ",\t" << linkrate << ",\t" << linkrate*flowQosMet << ",\t" << +percent << ",\t" 
					<< iter->second->m_minOwd << ",\t" 
					<< iter->second->m_owdSum/iter->second->m_count << ",\t"
					<< iter->second->m_maxOwd << ",\t";
				}

				std::string cidStr = LinkState::ConvertCidFormat(cid);

				element->Append(cidStr+"::"+directionStr+"::rate", (double)linkrate/1e3);
				element->Append(cidStr+"::"+directionStr+"::qos_rate", (double)linkrate*flowQosMet/1e3);
				element->Append(cidStr+"::"+directionStr+"::traffic_ratio", (double)percent);
				element->Append(cidStr+"::"+directionStr+"::owd", iter->second->m_owdSum/iter->second->m_count);
				element->Append(cidStr+"::"+directionStr+"::max_owd", iter->second->m_maxOwd);

				if(m_gmaDataProcessor && m_gmaRxControl->QosFlowPrioritizationEnabled())
				{
					if(m_clientRoleEnabled)
					{
						m_gmaDataProcessor->SaveDlQosMeasurement(m_clientId, (double)linkrate/1e3, m_linkParamsMap[cid]->m_qosMarking, (int)cid);
					}
					else if(m_serverRoleEnabled)
					{
						m_gmaDataProcessor->SaveUlQosMeasurement(m_clientId, (double)linkrate/1e3, m_linkParamsMap[cid]->m_qosMarking, (int)cid);
					}
				}
				if(m_saveToFile)
				{
					myfile << linkLoss <<",\t"<< missing << ",\t"<<abnormal<< ",\t"<<inOrder << ",\t" << highDelay<<"\n";
				}
	

			}
			else
			{
				element->Append(cidStr+"::"+directionStr+"::rate", 0.0);
				element->Append(cidStr+"::"+directionStr+"::qos_rate", 0.0);
				element->Append(cidStr+"::"+directionStr+"::traffic_ratio", 0.0);
				element->Append(cidStr+"::"+directionStr+"::owd", -1.0);
				element->Append(cidStr+"::"+directionStr+"::max_owd", -1.0);

				if(m_gmaDataProcessor && m_gmaRxControl->QosFlowPrioritizationEnabled())
				{
					if(m_clientRoleEnabled)
					{
						m_gmaDataProcessor->SaveDlQosMeasurement(m_clientId, 0.0, m_linkParamsMap[cid]->m_qosMarking, (int)cid);
					}
					else if(m_serverRoleEnabled)
					{
						m_gmaDataProcessor->SaveUlQosMeasurement(m_clientId, 0.0, m_linkParamsMap[cid]->m_qosMarking, (int)cid);
					}
				}
				if(m_saveToFile)
				{
					myfile << "\n";
				}
			}
			iterLink++;
		}

		m_measureParamPerCidMap.clear();
		myfile.close();
		m_receivedBytes = 0;

		m_reorderingTimeoutCounter = 0;
		m_tsuCounter = 0;
		if (m_gmaDataProcessor)
		{
			m_gmaDataProcessor->AppendMeasurement(element);
			m_gmaDataProcessor->AppendSliceMeasurement(sliceElementSum, NETWORK_CID);
			m_gmaDataProcessor->AppendSliceMeasurement(sliceElementMean, NETWORK_CID, true);

			if (m_clientRoleEnabled)
			{
				if(m_linkParamsMap.find(WIFI_CID) != m_linkParamsMap.end())
				{
					ns3::Ptr<ns3::NetworkStats> elementWifi = ns3::CreateObject<ns3::NetworkStats>(LinkState::ConvertCidFormat(WIFI_CID), m_clientId, end_ts);
					elementWifi->Append("cell_id", (double)m_linkParamsMap[WIFI_CID]->m_phyAccessContrl->GetApId());
					m_gmaDataProcessor->AppendMeasurement(elementWifi);
					m_gmaDataProcessor->UpdateCellId(m_clientId, (double)m_linkParamsMap[WIFI_CID]->m_phyAccessContrl->GetApId(), LinkState::ConvertCidFormat(WIFI_CID));
				}

				/*if(m_linkParamsMap.find(CELLULAR_NR_CID) != m_linkParamsMap.end())
				{
					//TODO: NR use wifi cell id for now... Fix it later.
					ns3::Ptr<ns3::NetworkStats> elementNr = ns3::CreateObject<ns3::NetworkStats>(LinkState::ConvertCidFormat(CELLULAR_NR_CID), m_clientId, end_ts);
					elementNr->Append("cell_id", (double)m_linkParamsMap[WIFI_CID]->m_phyAccessContrl->GetApId());
					m_gmaDataProcessor->AppendMeasurement(elementNr);
					m_gmaDataProcessor->UpdateCellId(m_clientId, (double)m_linkParamsMap[WIFI_CID]->m_phyAccessContrl->GetApId(), LinkState::ConvertCidFormat(CELLULAR_NR_CID));
				}*/
			}
		}

	}
	Simulator::Schedule(m_measurementGuardInterval, &GmaVirtualInterface::MeasurementGuardIntervalEnd, this);

}

void
GmaVirtualInterface::Transmit (Ptr<Packet> packet)
{
	//send ul qos testing request after transmit a new packet...
	if(m_clientRoleEnabled && m_gmaTxControl->QosSteerEnabled() )//client side && QOS enabled
	{
		std::map < uint8_t,  Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();
		while (iter!= m_linkParamsMap.end())
		{
			uint8_t cid = iter->first;
			if (cid != m_linkState->GetHighPriorityLinkCid() && m_linkState->IsLinkUp(cid))//not primary link, and the link is up
			{					
				if(m_qosClientTestingActive == false)//no active qos testing session
				{
					if (m_qosTestingAllowed && m_gmaTxControl->QoSTestingRequestCheck (cid))//check if we need to start Qos test Request (for backup links) from the client side... uplink
					{
						//check if the previous qos result is valid. If true, skip the testing and switch immediately.
						if(m_gmaRxControl->QoSValidCheck(cid))
						{
							//skip qos testing
							std::cout << Now().GetSeconds() << " CID:" << +cid << "QoS still valid!" << std::endl;
							Ptr<SplittingDecision> decision = m_gmaRxControl->GenerateTrafficSplittingDecision(cid, true);//generate a reverse tsu to switch traffic over cid
							if(decision->m_update)
							{
								SendTsu(decision);
							}
						}
						else
						{
							//send qos testing request
							m_qosClientTestingActive = true;//qos testing running
							if (m_qosSteerActionReceived)//ml action received
							{
								//only initiate qos test once, the next qos test should be triggered by a new action.
								m_qosTestingAllowed = false;
							}
							uint8_t qosDuration = m_linkState->m_qosTestDurationUnit100ms;
							//will schedule end qos testing after receives Notification or request msg retx timeout
							//Simulator::Schedule(Seconds(qosDurationS), &GmaVirtualInterface::QosTestingSessionEnd, this);//stop qos testing session after qosDurationS
							QosTestingRequestMsg(cid, qosDuration, 1);//1 stands for uplink qos testing
						}
					}
				}
			}
			iter++;
		}
	}

	m_numOfTxPacketsPerFlow++;
	m_TxBytesPerFlow += packet->GetSize();
	Ipv4Header ipv4Header;
	packet->PeekHeader (ipv4Header);

	//tos is 8 bits: [3 MSB for priority] [3 bits for slice id] [2 LSB for ECN] or [6 MSB for dscp] [2 LSB for ECN]
	//we only want to overwrite the 3MSB and preserve the slice id and ecn marking.
	auto ip_tos = ipv4Header.GetTos();
	//auto priority = ip_tos >> 5; //first 3 bits
	auto sliceId = (ip_tos & 0x1F) >> 2; //middle 3 bits
	//auto ecn = (ip_tos & 0x3); //last 2 bits.
	//std::cout <<Now().GetSeconds() <<" IP header: " << ipv4Header << std::endl;
	//std::cout <<Now().GetSeconds() <<" ip_tos: " << +ip_tos <<" priority: " << priority <<" sliceId: " << sliceId << " ecn: " << ecn << std::endl;

	if (sliceId != m_sliceId)
	{
		NS_FATAL_ERROR("currently, one interface only supports one slice id. This interface is configured to m_slice:" << +m_sliceId << ". but received slice: " << +sliceId);
	}
	uint64_t timeMs = (uint64_t) Simulator::Now ().GetMilliSeconds ();
	if(m_duplicateMode)
	{
		std::map < uint8_t, Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();
		while(iter!=m_linkParamsMap.end())
		{
			uint8_t cid = iter->first;

			//send packet to all links that are up
			if(m_linkState->IsLinkUp(cid))
			{
				GmaHeader gmaHeader;

				gmaHeader.SetTimeStamp(timeMs & 0xFFFFFFFF);
				//set GMA sequence #
				gmaHeader.SetSequenceNumber(m_gmaTxSn);

				gmaHeader.SetLocalSequenceNumber(m_linkParamsMap[cid]->m_gmaTxLocalSn);
				gmaHeader.SetConnectionId(cid); // later I might remove this since the CID may be referenced from port number
				gmaHeader.SetFlowId (DUPLICATE_FLOW_ID);//duplicated packets

				Ptr<Packet> dummyP = packet->Copy();
				dummyP->AddHeader(gmaHeader);
				//std::cout <<Now().GetSeconds() <<" TX IP: " <<ipv4Header.GetSource()<<  " -> " << ipv4Header.GetDestination() <<" link CID:" << +cid
				//<< " SN:" << m_gmaTxSn << " LSN:"<< +iter->second->m_gmaTxLocalSn << "\n";

				//m_linkParamsMap[cid]->m_socket->SendTo (dummyP, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
				if (ipv4Header.GetProtocol() == 17)
				{
					SendByCid(cid, dummyP, (TOS_AC_VI & 0xE0) + (ip_tos & 0x1F)); //udp traffic use video. Overwrite the 3 MSB (priority) in tos.
				}
				else
				{
					SendByCid(cid, dummyP, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F)); //tcp and other traffic use Best effort. Overwrite the 3 MSB (priority) in tos.
				}

				//log file
				/*std::ostringstream fileName;
				fileName <<"tx-"<<ipv4Header.GetSource()<<"-"<<ipv4Header.GetDestination()<<".csv";
				std::ofstream myfile;
				myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
				myfile << Simulator::Now ().GetSeconds () << ",\t" << +cid<< ",\t" << m_gmaTxSn<< ",\t" << +m_linkParamsMap[cid]->m_gmaTxLocalSn <<"\n";
				myfile.close();*/

				m_linkParamsMap[cid]->m_gmaTxLocalSn = (m_linkParamsMap[cid]->m_gmaTxLocalSn + 1) & MAX_GMA_LSN;
			}
			iter++;

		}
		// the max size of GMA sequence number is 3 Bytes.
		m_gmaTxSn = (m_gmaTxSn + 1) & MAX_GMA_SN;
	}
	else
	{
		GmaHeader gmaHeader;

		gmaHeader.SetTimeStamp(timeMs & 0xFFFFFFFF);
		//set GMA sequence #
		gmaHeader.SetSequenceNumber(m_gmaTxSn);

		//cid is selected by control algorithm. We havenot implement duplicate mode yet.
		//Maybe we can reserve a ID for dupilcate packets, e.g., 255.

		uint8_t cid = m_linkState->GetLowPriorityLinkCid();
		if (m_gmaTxControl->GetSplittingBurstFromTsu() == 0)
		{
			//no tsu received yet
			cid = m_gmaTxControl->GetDeliveryLinkCid();
		}
		else if (m_gmaTxControl->GetSplittingBurstFromTsu() == 1)
		{
			//steer mode:
			cid = m_gmaTxControl->GetDeliveryLinkCid();
		}
		else if (m_gmaTxControl->GetSplittingBurstFromTsu() >  1) //tsu received (>0), and not steer mode (>1)!
		{
			// Conditions
			// Link Up or Down: a link may be down due to low power, ctrl fail, etc.;
			// Low Delay or High Delay: the link queueing delay may be reported from TSU.

			//case 1: all links are down. (should not happen)
			//-> send over default link.

			//case 2: if there are links up, but all of them are high queueiung delay.
			//-> send over high queueing delay links, stop ongoing "skip" high queueing delay link process.

			//case 3: there exist links that meets both conditions (link is up and queueing delay is low).
			//-> send over the links that are up and with low queueing delay, "skip" the links with high delay for a period to drain the queue.		

			uint8_t upLinkCount = 0;
			uint8_t UpAndlowDelayLinkCount = 0;
			auto iter = m_linkParamsMap.begin();
			
			while (iter!= m_linkParamsMap.end())
			{
				if (m_linkState->IsLinkUp(iter->first))//link is up
				{
					upLinkCount++;
					//find a link is up. check if this link has low queuing delay (not skipped).
					if (m_linkState->IsLinkLowQueueingDelay(iter->first))
					{
						UpAndlowDelayLinkCount++;
					}
				}
				iter++;
			}


			if (upLinkCount == 0)
			{
				//case 1: all links are down
				std::cout << "ALL LINK ARE DOWN, SEND TO DEFAULT, SHOULD NEVER HAPPEN!" <<std::endl;
				//use the default link cid.
			}
			else
			{
				if (UpAndlowDelayLinkCount == 0)
				{
					//case 2: all uplinks have high delay
					m_linkState->StopLinkSkipping(); //Now all links are considered as low delay.
				}
				//we will find a link is up and with low delay  to transmit.

				bool linkFound = false;
				//if no tsu is received,  m_gmaTxControl->GetSplittingBurstFromTsu() == 0.
				for (uint8_t step = 0; step < m_gmaTxControl->GetSplittingBurstFromTsu(); step++)
				{
					cid = m_gmaTxControl->GetNextCidFromTsu();
					if (cid == UINT8_MAX)
					{
						NS_FATAL_ERROR("no TSU received yet!!!!");
					}

					if (m_linkState->IsLinkUp(cid) && m_linkState->IsLinkLowQueueingDelay(cid))
					{
						// this link is up and low latency.
						linkFound = true;
						break;
					}

				}
				if(!linkFound)
				{
					std::cout << "cannot find the link cid!!! upLinkCount: " << +upLinkCount << " UpAndlowDelayLinkCount: " << +UpAndlowDelayLinkCount;
					std::cout << "All link in TSU are down, we will use default link.";
					cid = m_gmaTxControl->GetDeliveryLinkCid();
				}

			}

		}
		//else no tsu, use default link.

		//uint8_t cid = m_gmaTxControl->GetDeliveryLinkCid();
		if (m_linkParamsMap.find(cid) == m_linkParamsMap.end())
		{
			//std::cout << " DROP PKT, link not configured yet!! need probe!\n";
			return;
		}

		gmaHeader.SetLocalSequenceNumber(m_linkParamsMap[cid]->m_gmaTxLocalSn);
		gmaHeader.SetConnectionId(cid); // later I might remove this since the CID may be referenced from port number
		if (m_gmaTxControl->QosSteerEnabled())
		{
			gmaHeader.SetFlowId (QOS_FLOW_ID);
		}
		else
		{
			gmaHeader.SetFlowId (BEST_EFFORT_FLOW_ID);
		}

		Ptr<Packet> dummyP = packet->Copy();
		dummyP->AddHeader(gmaHeader);
		//std::cout <<Now().GetSeconds() <<" TX IP: " <<ipv4Header.GetSource()<<  " -> " << ipv4Header.GetDestination() <<" link CID:" << +cid
		//<< " SN:" << m_gmaTxSn << " LSN:"<< +m_linkParamsMap[cid]->m_gmaTxLocalSn << "\n";

		//m_linkParamsMap[cid]->m_socket->SendTo (dummyP, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
		auto markIter = m_linkParamsMap.find(cid);

		if(markIter != m_linkParamsMap.end() && m_gmaRxControl->QosFlowPrioritizationEnabled())// find action for this user
		{
			if(m_linkParamsMap[cid]->m_qosMarking > 0)//enable qos
			{
				SendByCid(cid, dummyP, (TOS_AC_VI & 0xE0) + (ip_tos & 0x1F)); //qos, mark as video. Overwrite the 3 MSB (priority) in tos.
			}
			else
			{
				SendByCid(cid, dummyP, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F)); //nqos mark as best effort. Overwrite the 3 MSB (priority) in tos.
			}
		}
		else
		{
			if (ipv4Header.GetProtocol() == 17)
			{
				SendByCid(cid, dummyP, (TOS_AC_VI & 0xE0) + (ip_tos & 0x1F)); //udp traffic use video. Overwrite the 3 MSB (priority) in tos.
			}
			else
			{
				SendByCid(cid, dummyP, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F)); //tcp and other traffic use Best effort. Overwrite the 3 MSB (priority) in tos.
			}
		}


		//log file
		/*std::ostringstream fileName;
		fileName <<"tx-"<<ipv4Header.GetSource()<<"-"<<ipv4Header.GetDestination()<<".csv";
		std::ofstream myfile;
		myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
		myfile << Simulator::Now ().GetSeconds () << ",\t" << +cid<< ",\t" << m_gmaTxSn<< ",\t" << +m_linkParamsMap[cid]->m_gmaTxLocalSn <<"\n";
		myfile.close();*/

		// the max size of GMA sequence number is 3 Bytes.
		m_gmaTxSn = (m_gmaTxSn + 1) & MAX_GMA_SN;
		m_linkParamsMap[cid]->m_gmaTxLocalSn = (m_linkParamsMap[cid]->m_gmaTxLocalSn + 1) & MAX_GMA_LSN;

		//Send duplicated packet over backup links for testing QOS. we do not need reordering.

		auto dupCidList = m_gmaTxControl->GetDuplicateLinkCidList();
		for(uint32_t ind = 0; ind < dupCidList.size(); ind++)
		{
			uint8_t cid = dupCidList.at(ind);

			if (m_linkParamsMap.find(cid) == m_linkParamsMap.end())
			{
				std::cout << " DROP PKT, link not configured yet!! need probe!\n";
				continue;
			}

			//send packet to all links that are up
			if(m_linkState->IsLinkUp(cid))
			{
				GmaHeader gmaHeader;

				gmaHeader.SetTimeStamp(timeMs & 0xFFFFFFFF);
				//set GMA sequence #
				gmaHeader.SetSequenceNumber(m_gmaTxSn);

				gmaHeader.SetLocalSequenceNumber(m_linkParamsMap[cid]->m_gmaTxLocalSn);
				gmaHeader.SetConnectionId(cid); // later I might remove this since the CID may be referenced from port number

				if (m_gmaTxControl->QosSteerEnabled())
				{
					gmaHeader.SetFlowId (QOS_FLOW_ID);
				}
				else
				{
					gmaHeader.SetFlowId (BEST_EFFORT_FLOW_ID);
				}
				Ptr<Packet> dummyP = packet->Copy();
				dummyP->AddHeader(gmaHeader);
				//std::cout <<Now().GetSeconds() <<" TX (DUP) IP: " <<ipv4Header.GetSource()<<  " -> " << ipv4Header.GetDestination() <<" link CID:" << +cid
				//<< " SN:" << m_gmaTxSn << " LSN:"<< +m_linkParamsMap[cid]->m_gmaTxLocalSn << "\n";

				//m_linkParamsMap[cid]->m_socket->SendTo (dummyP, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
				SendByCid(cid, dummyP, (TOS_AC_BK & 0xE0) + (ip_tos & 0x1F));//testing packet, mark as background. Overwrite the 3 MSB (priority) in tos.

				m_linkParamsMap[cid]->m_gmaTxLocalSn = (m_linkParamsMap[cid]->m_gmaTxLocalSn + 1) & MAX_GMA_LSN;
			}
		}

	}
}

void
GmaVirtualInterface::Receive (Ptr<Packet> packet, const Ipv4Address& phyAddr, uint16_t fromPort)
{
	/*Address from;//the sender Address(IP, port)
	Ptr<Packet> packet = socket->RecvFrom (65535, 0 ,from);
	InetSocketAddress iaddr = InetSocketAddress::ConvertFrom (from);*/

	NS_LOG_DEBUG ("SocketRecv: " << *packet);
	GmaHeader gmaHeader;
	packet->RemoveHeader(gmaHeader);

	//Ipv4Header ipv4Header;
	//packet->PeekHeader (ipv4Header);
	//std::cout << " IP header:" << ipv4Header << "\n";

	if(gmaHeader.GetFlags() == 0 || gmaHeader.GetFlags() == 256)
	{
		// 0 is no info in gma header, 256 stands for client ID.
		//control message
		//std::cout <<Now().GetSeconds() <<" " << this << " Ctrl from port:" << +fromPort <<  "\n";
		MxControlHeader mxHeaderPeek;
		packet->PeekHeader(mxHeaderPeek);
		if(m_linkParamsMap[mxHeaderPeek.GetConnectionId()]->m_phyAccessContrl->ReceiveOk(phyAddr) == false)
		{
			return;
		}

		RecvCtrlMsg (packet, phyAddr);
	}
	else if(packet->GetSize() == 0) //end markder
	{
		if(m_linkParamsMap[gmaHeader.GetConnectionId()]->m_phyAccessContrl->ReceiveOk(phyAddr) == false)
		{
			return;
		}

		if(m_newLinkCid == UINT8_MAX)
		{
			//reordering already stopped. no action.
		}
		else 
		{
			if(m_newLinkCid == gmaHeader.GetConnectionId())
			{
				//std::cout << Now().GetSeconds() << " node: " << m_nodeId << " Receive [End Marker] over old link :" << +m_newLinkCid << " WIll SKIP NEXT REORDER\n";
				m_newLinkCid = UINT8_MAX;
			}
			else
			{
				StopReordering(1);// stop reordering due to received end marker from old link
			}
		}
	}
	else
	{
		//receive data begins.
		if(m_linkParamsMap[gmaHeader.GetConnectionId()]->m_phyAccessContrl->ReceiveOk(phyAddr) == false)
		{
			return;
		}

		if(m_gmaRxControl->GetSplittingBurst() == 1)//steer mode, check if we need to enable reordering.
		{
			if(m_newLinkCid == UINT8_MAX)
			{
				//already received end marker or the initial receive, no action.
				//std::cout << Now().GetSeconds() << " node: " << m_nodeId << " steer mode -> SkipReordering (either after End Marker or First Receive) pkt sn: " << +gmaHeader.GetSequenceNumber() << " cid: " << +gmaHeader.GetConnectionId()<< "\n";
				m_newLinkCid = gmaHeader.GetConnectionId();
			}
			else if(m_newLinkCid == gmaHeader.GetConnectionId())
			{
				//new pkt cid = previoud pkt cid, no action
			}
			else
			{
				if(m_enableReordering == false)
				{
					//start reordering right now...
					m_newLinkCid = gmaHeader.GetConnectionId();
					m_enableReordering = true;
					//std::cout << Now().GetSeconds() << " node: " << m_nodeId << " steer mode -> Enable Reordering For :" << m_reorderingTimeout.GetMilliSeconds()<< " ms. SN:" <<+gmaHeader.GetSequenceNumber() << " New link cid:" << +m_newLinkCid << "\n";
				}
				//else reordering already started, no action...

			}
			
		}

		if(gmaHeader.GetFlowId() == DUPLICATE_FLOW_ID && m_receiveDuplicateFlow == false)//duplicate mode
		{
			m_receiveDuplicateFlow = true;//the sender is sending duplicate packets
			if(m_useLsnReordering == true)
			{
				m_useLsnReordering = false;//we cannot use this algorithm in duplicate mode
			}
			if(m_enableReordering == false)
			{
				m_enableReordering = true;//must enable reordering for duplicate mode
			}

		}

		if(m_gmaRxControl->QosSteerEnabled() && gmaHeader.GetFlowId() == QOS_FLOW_ID)//QOS mode
		{
			m_receiveDuplicateFlow = true;//the sender is sending duplicate packets
			if(m_useLsnReordering == true)
			{
				m_useLsnReordering = false;//we cannot use this algorithm in duplicate mode
			}
			if(m_enableReordering == true)
			{
				m_enableReordering = false;//dont enable reordering for QOS flow
			}
		}

		//std::cout <<Now().GetSeconds() <<" " << this <<  "RX from port:" << +fromPort  << " CID:"
		//<<+gmaHeader.GetConnectionId() << " SN:"<<gmaHeader.GetSequenceNumber() << " LSN:"<< +gmaHeader.GetLocalSequenceNumber() <<  "\n";


		//log file
		/*Ipv4Header ipv4Header;
		packet->PeekHeader (ipv4Header);
		std::ostringstream fileName;
		fileName <<"rx-"<<ipv4Header.GetSource()<<"-"<<ipv4Header.GetDestination()<<".csv";
		std::ofstream myfile;
		myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
		myfile << Simulator::Now ().GetSeconds () << ",\t" << +gmaHeader.GetConnectionId()
		<< ",\t" << gmaHeader.GetSequenceNumber()<< ",\t" << +gmaHeader.GetLocalSequenceNumber() <<"\n";
		myfile.close();*/

		NS_ASSERT_MSG(gmaHeader.GetConnectionId() == fromPort-START_PORT_NUM, "the port number should equals START_PORT_NUM + cid");

		uint8_t cid = gmaHeader.GetConnectionId();

		if (m_rxMode)
		{
			if(m_measurementManager->IsMeasurementOn())//this check whether measure cycle is on 
			{
				//start measurement interval if condition meets
				//m_measurementManager->MeasureIntervalStartCheck(Now(), m_gmaRxExpectedSn);
				m_measurementManager->MeasureIntervalStartCheck(Now(), gmaHeader.GetSequenceNumber(), m_gmaRxExpectedSn);//use sn from the pkt.
				//measure owd and loss from lsn
				m_measurementManager->DataMeasurementSample(Now().GetMilliSeconds() - gmaHeader.GetTimeStamp(), gmaHeader.GetLocalSequenceNumber(), cid);
				//check if the interval should end
				m_measurementManager->MeasureIntervalEndCheck(Now());
			}

			if(m_clientRoleEnabled && m_gmaRxControl->QosSteerEnabled() )//client side && QOS enabled
			{
				std::map < uint8_t,  Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();
				while (iter!= m_linkParamsMap.end())
				{
					uint8_t cid = iter->first;
					if (cid != m_linkState->GetHighPriorityLinkCid() && m_linkState->IsLinkUp(cid))//not high priority link and the link is up
					{
						if(m_qosClientTestingActive == false)//no active qos testing session
						{
							if (m_qosTestingAllowed && m_gmaRxControl->QoSTestingRequestCheck (cid))//check if we need to start Qos test Request (for backup links) from the client side... downlink
							{
								//check if the previous qos result is valid. If true, skip the testing and switch immediately.
								if(m_gmaRxControl->QoSValidCheck(cid))
								{
									//skip qos testing
									std::cout << Now().GetSeconds() << " CID:" << +cid << "QoS still valid!" << std::endl;
									uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
									Ptr<SplittingDecision> decision = m_gmaRxControl->GenerateTrafficSplittingDecision(cid);//generate a tsu to switch traffic over cid
									if(decision->m_update)
									{
										SendTsu(decision);
										if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
										{
											//split mode & default link cid changed, we need to send back a end marker...
											SendEndMarker(oldCid);
										}
									}

								}
								else
								{
									//send qos testing request
									m_qosClientTestingActive = true;//start qos testing session
									if (m_qosSteerActionReceived)//ml action enabled
									{
										//only start qos test once, the next qos test should be triggered by a new action.
										m_qosTestingAllowed = false;
									}
									uint8_t qosDuration = m_linkState->m_qosTestDurationUnit100ms;
									//will schedule end qost testing after receives Notification or request msg retx timeout
									//Simulator::Schedule(Seconds(qosDurationS), &GmaVirtualInterface::QosTestingSessionEnd, this);//stop qos testing after qosDurationS
									QosTestingRequestMsg(cid, qosDuration, 0); //0 stands for downlink qos testing
								}
							}
						}
					}
					iter++;
				}
			}

		}

		if(m_enableMeasureReport)
		{
			uint32_t owd = Now().GetMilliSeconds() - gmaHeader.GetTimeStamp();
			m_receivedBytes += packet->GetSize();

			if(m_measureParamPerCidMap.find(cid) == m_measureParamPerCidMap.end())
			{
				Ptr<MeasureParam> measureParam = Create<MeasureParam>();
				m_measureParamPerCidMap[cid] = measureParam;
			}

			if(m_measureSnPerCidMap.find(cid) == m_measureSnPerCidMap.end())
			{
				Ptr<MeasureSn> measureSn = Create<MeasureSn>();
				m_measureSnPerCidMap[cid] = measureSn;
			}


			m_measureParamPerCidMap[cid]->m_rcvBytes += packet->GetSize();

			if(m_measureParamPerCidMap[cid]->m_maxOwd < owd)
			{
				m_measureParamPerCidMap[cid]->m_maxOwd = owd;
			}

			if(m_measureParamPerCidMap[cid]->m_minOwd > owd)
			{
				m_measureParamPerCidMap[cid]->m_minOwd = owd;
			}

			if(owd > m_acceptableDelay)
			{
				m_measureParamPerCidMap[cid]->m_numOfHighDelayPkt += 1;
			}

			m_measureParamPerCidMap[cid]->m_owdSum += owd;
			m_measureParamPerCidMap[cid]->m_count++;

			uint8_t lastLsn = gmaHeader.GetLocalSequenceNumber();
			//determing in order or not
			if(LsnDiff(lastLsn, m_measureSnPerCidMap[cid]->m_lastLsn) == 1)
			{
				// in order packets
				m_measureParamPerCidMap[cid]->m_numOfInOrderPacketsForReport++;
				m_measureSnPerCidMap[cid]->m_lastLsn = lastLsn;
				m_measureSnPerCidMap[cid]->m_lastGsn = gmaHeader.GetSequenceNumber();
			}
			else if(LsnDiff(lastLsn, m_measureSnPerCidMap[cid]->m_lastLsn) > 1)
			{
				// detect a gap: received Lsn larger than expected value.
				//std::cout << "--------------------------------new:" <<+lastLsn
				//<< " last:" << +m_measureSnPerCidMap[cid]->m_lastLsn 
				//<< " missing:" << LsnDiff(lastLsn, m_measureSnPerCidMap[cid]->m_lastLsn)-1 << "\n";
				m_measureParamPerCidMap[cid]->m_numOfMissingPacketsForReport = m_measureParamPerCidMap[cid]->m_numOfMissingPacketsForReport + LsnDiff(lastLsn, m_measureSnPerCidMap[cid]->m_lastLsn)-1;
				m_measureParamPerCidMap[cid]->m_numOfInOrderPacketsForReport++;
				m_measureSnPerCidMap[cid]->m_lastLsn = lastLsn;
				m_measureSnPerCidMap[cid]->m_lastGsn = gmaHeader.GetSequenceNumber();
			}
			else 
			{
				//std::cout << "--------------------------------new:" <<+lastLsn
				//<< " last:" << +m_measureSnPerCidMap[cid]->m_lastLsn 
				//<< " abormal: 1 \n";
				//abnormal packets

				if(SnDiff(gmaHeader.GetSequenceNumber(), m_measureSnPerCidMap[cid]->m_lastGsn) > 0)
				{
					//miss more than 128 packets!!!
					m_measureParamPerCidMap[cid]->m_numOfMissingPacketsForReport = m_measureParamPerCidMap[cid]->m_numOfMissingPacketsForReport + 256 + LsnDiff(lastLsn, m_measureSnPerCidMap[cid]->m_lastLsn)-1;
					m_measureParamPerCidMap[cid]->m_numOfInOrderPacketsForReport++;
					m_measureSnPerCidMap[cid]->m_lastLsn = lastLsn;
					m_measureSnPerCidMap[cid]->m_lastGsn = gmaHeader.GetSequenceNumber();
				}
				else
				{	
					m_measureParamPerCidMap[cid]->m_numOfAbnormalPacketsForReport++;
				}
			}

		}
		if(m_gmaRxControl->QosSteerEnabled() && gmaHeader.GetFlowId() == QOS_FLOW_ID && m_discardBackupLinkPackets && cid != m_linkState->GetHighPriorityLinkCid())
		{
			//for qos testing packets, we measure them for per link measurement, but not for flow measurement!!!
			return; //ignore testing packet...
		}

		if(m_enableReordering)
		{
			InOrderDelivery (packet, gmaHeader, cid);
		}
		else
		{
			// no reordering, forward to GMA
			m_gmaRxExpectedSn = (gmaHeader.GetSequenceNumber() + 1) & MAX_GMA_SN;//we update the expected sn even without reordering.
			MeasureAndForward (packet, gmaHeader);
		}
	}
}

void
GmaVirtualInterface::MeasureAndForward (Ptr<Packet> packet, const GmaHeader& gmaHeader)
{
	if(m_enableMeasureReport)
	{
		//determing in order or not
		if(SnDiff(gmaHeader.GetSequenceNumber(), m_lastGsn) == 1)
		{
			// in order packets
			m_numOfInOrderPacketsPerFlow++;
			m_lastGsn = gmaHeader.GetSequenceNumber();
		}
		else if(SnDiff(gmaHeader.GetSequenceNumber(), m_lastGsn) > 1)
		{
			//std::cout << "--------------------------------new:" <<gmaHeader.GetSequenceNumber() << " last:" << m_lastGsn << " missing:" << SnDiff(gmaHeader.GetSequenceNumber(), m_lastGsn)-1 << "\n";
			// detect a gap: received Lsn larger than expected value.
			m_numOfMissingPacketsPerFlow = m_numOfMissingPacketsPerFlow + SnDiff(gmaHeader.GetSequenceNumber(), m_lastGsn)-1;
			m_numOfInOrderPacketsPerFlow++;
			m_lastGsn = gmaHeader.GetSequenceNumber();
		}
		else 
		{
			//abnormal packets
			m_numOfAbnormalPacketsPerFlow++;
		}

	}

	//deliver all pkts..
	//if(SnDiff(gmaHeader.GetSequenceNumber(), m_gmaRxExpectedSn) >= 0)
	//{
		//only measure the owd of the packets need to be delivered
		if(m_enableMeasureReport)
		{
			uint32_t owd = Now().GetMilliSeconds() - gmaHeader.GetTimeStamp();
			if(m_flowParam->m_maxOwd < owd)
			{
				m_flowParam->m_maxOwd = owd;
			}

			if(m_flowParam->m_minOwd > owd)
			{
				m_flowParam->m_minOwd = owd;
			}

			if(owd > m_acceptableDelay)
			{
				m_flowParam->m_numOfHighDelayPkt += 1;
			}

			if(owd > m_delaythresh1)
			{
				m_flowParam->m_numofT1DelayPkt += 1;
			}

			if(owd > m_delaythresh2)
			{
				m_flowParam->m_numofT2DelayPkt += 1;
			}

			m_flowParam->m_owdSum += owd;
			m_flowParam->m_count++;

			m_flowParam->m_rcvBytes += packet->GetSize();

		}
		m_forwardPacketCallback (packet);
	//}
}

void
GmaVirtualInterface::InOrderDelivery (Ptr<Packet> packet, const GmaHeader& gmaHeader, uint8_t cid)
{
	NS_ASSERT_MSG (m_linkParamsMap.find(cid) != m_linkParamsMap.end(), "this cid doesnot exit");
	NS_ASSERT_MSG(gmaHeader.GetConnectionId() == cid, "Now the cid from GMA header should be the same converted from port number");
	if(gmaHeader.GetSequenceNumber() == m_gmaRxExpectedSn) // in order packets
	{
		MeasureAndForward (packet, gmaHeader);
		m_gmaRxExpectedSn = (m_gmaRxExpectedSn + 1) & MAX_GMA_SN;
		ReleaseInOrderPackets();//release in order packets in the queue
		if(m_newLinkCid == cid)//stop reordering if the inoder packet arrived at the new link:
		{
			//std::cout << "receive in order pkt sn:" << +gmaHeader.GetSequenceNumber() << " from cid:" << +cid << "\n";
			StopReordering(0);
		}
	}
	else if (SnDiff(gmaHeader.GetSequenceNumber(), m_gmaRxExpectedSn) < 0)
	{
		//do not deliver out of order packet that sn is smaller than expected sn.
		//MeasureAndForward (packet, gmaHeader);
		std::cout << Now().GetSeconds() << "------[small]------ last SN:" << m_linkParamsMap[cid]->m_gmaRxLastSn << " "
		<< " new SN:" << gmaHeader.GetSequenceNumber () << " "
		<< "last LSN:" << +m_linkParamsMap[cid]->m_gmaRxLastLocalSn << " new LSN:" 
		<< +gmaHeader.GetLocalSequenceNumber() << " SN diff:" 
		<< SnDiff(gmaHeader.GetSequenceNumber(), m_linkParamsMap[cid]->m_gmaRxLastSn) - 1
		<<  "\n";
	}
	else
	{
		//out of order;
		//if lost = gap, we still deliver 

		int numOfLostPacket = LsnDiff(gmaHeader.GetLocalSequenceNumber(), m_linkParamsMap[cid]->m_gmaRxLastLocalSn) - 1;
		//NS_ASSERT_MSG(numOfLostPacket >=0, "num of lost packets cannot be negative");

		//lsn should be always in order!!!!
		/*std::cout << Now().GetSeconds() << "------------ last SN:" << m_linkParamsMap[cid]->m_gmaRxLastSn << " "
		<< " new SN:" << gmaHeader.GetSequenceNumber () << " "
		<< "last LSN:" << +m_linkParamsMap[cid]->m_gmaRxLastLocalSn << " new LSN:" 
		<< +gmaHeader.GetLocalSequenceNumber() << " SN diff:" 
		<< SnDiff(gmaHeader.GetSequenceNumber(), m_linkParamsMap[cid]->m_gmaRxLastSn) - 1
		<< " lost:" << +numOfLostPacket << "\n";*/

		if(m_useLsnReordering && (numOfLostPacket == SnDiff(gmaHeader.GetSequenceNumber(), m_linkParamsMap[cid]->m_gmaRxLastSn) - 1))
		{
			if (m_linkParamsMap[cid]->m_reorderingQueue.size() == 0)//no reordering over this link, release this packet
			{
				MeasureAndForward (packet, gmaHeader);
				m_gmaRxExpectedSn =  (gmaHeader.GetSequenceNumber()+1) & MAX_GMA_SN;
				ReleaseInOrderPackets();
				if(m_newLinkCid == cid)//stop reordering if the inoder packet arrived at the new link:
				{
					//std::cout << "receive (LSN) in order pkt sn:" << +gmaHeader.GetSequenceNumber() << " from cid:" << +cid << "\n";
					StopReordering(0);
				}
			}
			else 
			{
				//put this packet into the reordering queue, but mark it as inorder such that it will be delivered if the packet before it is released.
				NS_ASSERT_MSG(m_totalQueueSize!=0, "it cannnot be empty here");
				NS_ASSERT_MSG(m_reorderingTimeoutEvent.IsRunning(), "The reordering cannot be expired");
				Ptr<RxQueueIterm> item = Create <RxQueueIterm> ();
				item->m_packet = packet;
				item->m_gmaHeader = gmaHeader;
				item->m_inOrder = true;
				item->m_receivedTime = Now();
				m_linkParamsMap[cid]->m_reorderingQueue.push(item);
				m_totalQueueSize++;

			}
		}
		else
		{
			if(m_newLinkCid == cid && m_stopReorderEvent.IsExpired())
			{
				//std::cout << "receive out of order pkt sn:" << +gmaHeader.GetSequenceNumber() << " from cid:" << +cid << "\n";
				//in steer mode, receive out of order pkt, stop reordering after timeout
				UpdateReorderTimeout();
				m_stopReorderEvent = Simulator::Schedule(m_reorderingTimeout, &GmaVirtualInterface::StopReordering, this, 2); //m_reorderingTimeout = 2 x (MAX - MIN);
			}
			//out of order packet, put into the queue
			Ptr<RxQueueIterm> item = Create <RxQueueIterm> ();
			item->m_packet = packet;
			item->m_gmaHeader = gmaHeader;
			item->m_receivedTime = Now();
			m_linkParamsMap[cid]->m_reorderingQueue.push(item);
			m_totalQueueSize++;
		}
		ReleaseMinSnPacket();//if all queues are not empty, compare the sn of first packet and release the one with min SN
		//std::cout << "min SN:" << minSn << " min index:" << +minCid << "\n";
		ReleaseInOrderPackets();//release inorder packets in the reordering queue (including timeout ones)
	}
		
	m_linkParamsMap[cid]->m_gmaRxLastLocalSn = gmaHeader.GetLocalSequenceNumber();
	m_linkParamsMap[cid]->m_gmaRxLastSn = gmaHeader.GetSequenceNumber();

}

void
GmaVirtualInterface::ReleaseMinSnPacket()
{
	int minSn = MAX_GMA_SN;
	uint8_t minCid = 255;

	while(minSn >= 0)//minSn < 0 means one or more queues are empty, break the look
	{
		minSn = MAX_GMA_SN;

		std::map < uint8_t, Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();
		while(iter != m_linkParamsMap.end())
		{
			 //this while loop find the minimual sn and its cid of all reordering queues, 
			//if more than one queue is empty, return minSN = -1
			if(iter->second->m_reorderingQueue.size() == 0)// one queue is empty, break
			{
				if(m_linkState->IsLinkUp(iter->first))//this link is up
				{
					//we now do not care if the link is up or down for reordering.
					minSn = -1;	
					break;
				}
				//else this link is failed, skip it
			}
			else
			{
				if(minSn == MAX_GMA_SN || SnDiff(minSn, iter->second->m_reorderingQueue.front()->m_gmaHeader.GetSequenceNumber()) > 0)
				{
					//find the minimual sequence of all queues.
					minSn = iter->second->m_reorderingQueue.front()->m_gmaHeader.GetSequenceNumber();
					minCid = iter->first;
				}
			}
			iter++;
		}

		if(minSn == MAX_GMA_SN)
		{
			minSn = -1;
		}

		if(minSn >= 0)//no queue is empty
		{
			//deliver the packet with minimual sn
			NS_ASSERT_MSG( minCid!= 255, " cannot be the initial value");

			Ptr<RxQueueIterm> itemDequeue = m_linkParamsMap[minCid]->m_reorderingQueue.front();
			m_linkParamsMap[minCid]->m_reorderingQueue.pop();
			m_totalQueueSize--;

			if(SnDiff(itemDequeue->m_gmaHeader.GetSequenceNumber(), m_gmaRxExpectedSn) >=0)
			{
				//deliver if the packet's sn is greater or equal to the expected SN
				MeasureAndForward (itemDequeue->m_packet, itemDequeue->m_gmaHeader);
				m_gmaRxExpectedSn = (itemDequeue->m_gmaHeader.GetSequenceNumber() + 1) & MAX_GMA_SN;
				//std::cout << "deliver sn " << itemDequeue->m_gmaHeader.GetSequenceNumber() << "\n";
			}
			else
			{
				MeasureAndForward (itemDequeue->m_packet, itemDequeue->m_gmaHeader);
				//discard duplicated packets.
			}
		}
	}

}

void
GmaVirtualInterface::UpdateReorderTimeout(){
	//update the reordering timeout if the current measurement measures a higher timeout value;
	Time newReorderingTimeout = m_reorderingTimeout;

	uint32_t maxOwd = 0;
	uint32_t minOwd = UINT32_MAX;

	std::map < uint8_t, Ptr<MeasureParam> >::iterator iter = m_measureParamPerCidMap.begin();
	while(iter!=m_measureParamPerCidMap.end())
	{
		if(maxOwd < iter->second->m_maxOwd)
		{
			maxOwd = iter->second->m_maxOwd;
		}

		if(minOwd > iter->second->m_minOwd)
		{
			minOwd = iter->second->m_minOwd;
		}
		iter++;
	}
	if(maxOwd!=0 && minOwd!=UINT32_MAX)
	{
		//reordering timeout equals 2* (max OWD - min OWD), it is also in the rage of [MIN..., MAX_REORDERING_TIMEOUT]
		if(m_measureParamPerCidMap.size() > 1)
		{
			newReorderingTimeout = std::max(MIN_REORDERING_TIMEOUT, std::min(MAX_REORDERING_TIMEOUT, MilliSeconds(2*(maxOwd-minOwd))));
			if(newReorderingTimeout > m_reorderingTimeout)
			{
				m_reorderingTimeout = newReorderingTimeout;
				//update reordering timeout to the bigger one.
				//std::cout << Now().GetSeconds() << " node: " << m_nodeId << " UPDATE reordering timeout maxOwd:" << maxOwd 
				//<< "ms, min Owd:" << minOwd << "ms, timeout:" << m_reorderingTimeout.GetMilliSeconds() << "ms@@@@@@@@\n";
			}
		}

	}
}

void
GmaVirtualInterface::ReleaseInOrderPackets()
{
	//this function should not be called if all queues are full!!!!
	//call ReleaseMinSnPacket() first!!!

	//now we try to deliver all in order packets
	UpdateReorderTimeout();
	uint32_t minSn = MAX_GMA_SN;
	uint8_t minCid = 255;

	while(m_totalQueueSize > 0)
	{
		//compare the first packet of all queues and get the min SN. If it is out of order, all other packets are out of order
		std::map < uint8_t, Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();

		minSn = MAX_GMA_SN;

		while(iter != m_linkParamsMap.end())
		{
			//this while loop find the min SN and its cid, ingores the empty queue
			if(iter->second->m_reorderingQueue.size() == 0)// queue is empty
			{
				//do nothing
			}
			else
			{
				if(minSn == MAX_GMA_SN || SnDiff(minSn, iter->second->m_reorderingQueue.front()->m_gmaHeader.GetSequenceNumber()) > 0)
				{
					//find the minimual sequence of all queues.
					minSn = iter->second->m_reorderingQueue.front()->m_gmaHeader.GetSequenceNumber();
					minCid = iter->first;
				}
			}
			iter++;
		}

		if(minSn == m_gmaRxExpectedSn)//in order delivery
		{
			NS_ASSERT_MSG( minCid!= 255, " cannot be the initial value");
			Ptr<RxQueueIterm> itemDequeue = m_linkParamsMap[minCid]->m_reorderingQueue.front();
			m_linkParamsMap[minCid]->m_reorderingQueue.pop();
			m_totalQueueSize--;

			MeasureAndForward (itemDequeue->m_packet, itemDequeue->m_gmaHeader);
			m_gmaRxExpectedSn = (itemDequeue->m_gmaHeader.GetSequenceNumber()+1) & MAX_GMA_SN;
			//std::cout << "deliver sn " << itemDequeue->m_gmaHeader.GetSequenceNumber() << "\n";
			if(m_newLinkCid == itemDequeue->m_gmaHeader.GetConnectionId())//stop reordering if the inoder packet arrived at the new link:
			{
				//std::cout << "(after reordering) in order pkt sn:" << +itemDequeue->m_gmaHeader.GetSequenceNumber() << " from cid:" << +itemDequeue->m_gmaHeader.GetConnectionId() << "\n";
				StopReordering(0);
			}
		}
		else if (SnDiff(minSn, m_gmaRxExpectedSn) < 0)
		{
			//smaller sn, may happen in duplicate mode
			//NS_FATAL_ERROR("should not happen, the smaller sn should be delivered in the previous step");
			Ptr<RxQueueIterm> itemDequeue = m_linkParamsMap[minCid]->m_reorderingQueue.front();
			m_linkParamsMap[minCid]->m_reorderingQueue.pop();
			m_totalQueueSize--;
			MeasureAndForward (itemDequeue->m_packet, itemDequeue->m_gmaHeader);
		}
		else
		{
			//larger sn than next sn
			NS_ASSERT_MSG( minCid!= 255, " cannot be the initial value");
	
			Ptr<RxQueueIterm> itemFront = m_linkParamsMap[minCid]->m_reorderingQueue.front();
			if(itemFront->m_inOrder)
			{
				//even if there is a gap, if this packet is marked as in order use LSN, we delivery
				// a packet is makred as inorder if this SN diff is the same as LSN diff.
				m_linkParamsMap[minCid]->m_reorderingQueue.pop();
				m_totalQueueSize--;

				MeasureAndForward (itemFront->m_packet, itemFront->m_gmaHeader);
				m_gmaRxExpectedSn = (itemFront->m_gmaHeader.GetSequenceNumber()+1) & MAX_GMA_SN;

				//std::cout << "deliver sn " << itemFront->m_gmaHeader.GetSequenceNumber() << "\n";
			}
			else
			{
				//the min SN is lager than next sn. not inorder(from the LSN algorithm)
				bool expiredPacketExist = false;
				iter = m_linkParamsMap.begin();
				while(iter != m_linkParamsMap.end())
				{
					if(iter->second->m_reorderingQueue.size() != 0)
					{
						//Check is there any expired packet in the queue!
						if(Now() >= iter->second->m_reorderingQueue.front()->m_receivedTime + m_reorderingTimeout)
						{
							//find an expired packet!
							expiredPacketExist = true;
							break;
						}
					}
					iter++;
				}

				if(expiredPacketExist)
				{
					//if there is an expired packet, we will release the min sn packet first;
					//if the expired packet is not the min sn, we will repeast releasing packets until we release this expired packets
					//this way, we make sure the packets are released in order!
					m_reorderingTimeoutCounter++;
					m_linkParamsMap[minCid]->m_reorderingQueue.pop();
					m_totalQueueSize--;

					MeasureAndForward (itemFront->m_packet, itemFront->m_gmaHeader);
					m_gmaRxExpectedSn = (itemFront->m_gmaHeader.GetSequenceNumber()+1) & MAX_GMA_SN;

					//std::cout << "deliver sn " << itemFront->m_gmaHeader.GetSequenceNumber() << "\n";
				}
				else
				{
					//the min SN packet in the queue is greater than next sn and no packet is expired, break the loop and return
					break;
				}
			}
		}
	}

	if(m_totalQueueSize > 0)
	{
		m_reorderingTimeoutEvent.Cancel();
		//add 1 milli second as gurad time
		//here I just assume the packet with min CID is the packet received earliest (obviously not correct).
		//because if I check the "actual" expired packet, the packets with smaller SN will also need to be cleared!!!!

		Time delay = m_reorderingTimeout + MilliSeconds(1) - (Now() - m_linkParamsMap[minCid]->m_reorderingQueue.front()->m_receivedTime);
		m_reorderingTimeoutEvent = Simulator::Schedule(delay, &GmaVirtualInterface::ReorderingTimeout, this);
	}
	else if (m_reorderingTimeoutEvent.IsRunning())
	{
		m_reorderingTimeoutEvent.Cancel();
	}
}

void
GmaVirtualInterface::ReleaseAllPackets()
{
	uint32_t minSn = MAX_GMA_SN;
	uint8_t minCid = 255;

	while(m_totalQueueSize > 0)
	{
		//compare the first packet of all queues and get the min SN. If it is out of order, all other packets are out of order
		std::map < uint8_t, Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();

		minSn = MAX_GMA_SN;

		while(iter != m_linkParamsMap.end())
		{
			//this while loop find the min SN and its cid, ingores the empty queue
			if(iter->second->m_reorderingQueue.size() == 0)// queue is empty
			{
				//do nothing
			}
			else
			{
				if(minSn == MAX_GMA_SN || SnDiff(minSn, iter->second->m_reorderingQueue.front()->m_gmaHeader.GetSequenceNumber()) > 0)
				{
					//find the minimual sequence of all queues.
					minSn = iter->second->m_reorderingQueue.front()->m_gmaHeader.GetSequenceNumber();
					minCid = iter->first;
				}
			}
			iter++;
		}

		NS_ASSERT_MSG( minCid!= 255, " cannot be the initial value");
		Ptr<RxQueueIterm> itemDequeue = m_linkParamsMap[minCid]->m_reorderingQueue.front();
		m_linkParamsMap[minCid]->m_reorderingQueue.pop();
		m_totalQueueSize--;

		MeasureAndForward (itemDequeue->m_packet, itemDequeue->m_gmaHeader);
		m_gmaRxExpectedSn = (itemDequeue->m_gmaHeader.GetSequenceNumber()+1) & MAX_GMA_SN;
		//std::cout << "deliver sn " << itemDequeue->m_gmaHeader.GetSequenceNumber() << "\n";
	}

	
}

void
GmaVirtualInterface::ReorderingTimeout()
{
	std::cout << Now().GetSeconds() << "**************************REOEDERING TIMEOUT\n";
	//now we try to deliver all packets in the queue, from small sn to large sn
	//it is more efficient to have a function just check expired packets!!
	//here is a problem that I use check whether the minSn packet is expired or not. Not actually check the expired packet.
	//because if I check the "actual" expired packet, the packets with smaller SN will also need to be cleared!!!!
	ReleaseInOrderPackets();
	//ReleaseAllPackets();
	m_reorderingTimeout = std::max(MIN_REORDERING_TIMEOUT, std::min(MAX_REORDERING_TIMEOUT, 2*m_reorderingTimeout));//double reordering timeout after a reordering timeout.
	m_nextReorderingUpdateTime = Now() + Seconds(10); //detect reordering timeout, double rto and do not update reordering timeout for 10 seconds.
}

void 
GmaVirtualInterface::PeriodicProbeMsg()
{
	//send probe message every PROBE_INTERVAL
	//in andorid app, we do not use periodic probe message anymore, it is active triggered.
	uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();

	std::map < uint8_t,  Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();
	while (iter!= m_linkParamsMap.end())//duplicate packets over all links.
	{
		uint8_t cid = iter->first;

		MxControlHeader mxHeader;
		mxHeader.SetType (1);
		mxHeader.SetConnectionId (cid);
		mxHeader.SetSequenceNumber(m_mxCtrSeqNumber++);

		if(m_wifiPowerAvailable)
		{
			mxHeader.SetProbeFlag(1);// let us use 1 for update phy address, 0 for not update.
		}
		else
		{
			mxHeader.SetProbeFlag(0);
		}
		mxHeader.SetRCid(cid);
		mxHeader.SetTimeStamp(now & 0xFFFFFFFF);

		SendCtrlMsg(mxHeader);
		iter++;
	}
	
	//we send multiple probes to detect the min RTT. (This may help eliminate the uplink scheduling request delay for Cellular.)

	//std::cout << " m_probeCounter: " << m_probeCounter << std::endl;
	m_probeCounter++;
	if (m_probeCounter < PROBE_BURST_SIZE)
	{
		//send another probe after the gap
		m_periodicProbeEvent = Simulator::Schedule(m_probeBurstGap, &GmaVirtualInterface::PeriodicProbeMsg, this);
		//m_probeBurstGap += m_probeBurstGap;//double the gap every time
	}
	else
	{
		//all probes in this burst is been sent. Resent probe burst (multiple probe msgs) after PROBE_INTERVAL.
		m_periodicProbeEvent = Simulator::Schedule(PROBE_INTERVAL, &GmaVirtualInterface::PeriodicProbeMsg, this);
		m_probeBurstGap = MilliSeconds(1);
		m_probeCounter = 0;
	}
}

void
GmaVirtualInterface::QosTestingRequestMsg (uint8_t cid, uint8_t duration, uint8_t direction)
{
	NS_ASSERT_MSG(m_gmaRxControl->QosSteerEnabled(), "only call this in Qos Enabled mode!!!");
	//prepare the qos testing request msg
	uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();
	MxControlHeader mxHeader;
	mxHeader.SetType (8);
	mxHeader.SetConnectionId (cid);
	mxHeader.SetSequenceNumber(m_mxCtrSeqNumber++);
	mxHeader.SetTimeStamp(now & 0xFFFFFFFF);
	mxHeader.SetFlowId(QOS_FLOW_ID);
	mxHeader.SetTrafficDirection(direction);
	mxHeader.SetTestDuration(duration);

	//we only know the channel ID for WiFi.
	uint16_t apId = m_linkParamsMap[cid]->m_phyAccessContrl->GetApId();
	if(apId == UINT8_MAX)
	{
		//NS_FATAL_ERROR("cannot find the AP ID for this node");
		//we only know the apId for WiFi link.
		apId = 1;//for other type of links assign apId = 1
	}

	if(m_qosSteerActionReceived)
	{
		//set to this channel id, the server will by pass the request scheduler. and start qos test right way.
		apId = UINT8_MAX;
	}
	mxHeader.SetChannelId(apId); //APs use different channels.
	
	std::cout << Now().GetSeconds() << " send QOS Start Request AP: "<<+ apId << " " << mxHeader << std::endl;
	SendCtrlMsg(mxHeader);
}

void
GmaVirtualInterface::QosTestingStartNotifyMsg (const MxControlHeader& header)
{
	NS_ASSERT_MSG(m_gmaTxControl->QosSteerEnabled(), "only call this in Qos Enabled mode!!!");
	//prepare the qos testing request msg
	uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();
	MxControlHeader mxHeader;
	mxHeader.SetType (9);
	mxHeader.SetConnectionId (header.GetConnectionId());
	mxHeader.SetSequenceNumber(m_mxCtrSeqNumber++);
	mxHeader.SetTimeStamp(now & 0xFFFFFFFF);
	mxHeader.SetFlowId(QOS_FLOW_ID);
	mxHeader.SetTrafficDirection(header.GetTrafficDirection()); //
	mxHeader.SetTestDuration(header.GetTestDuration());//
	std::cout << Now().GetSeconds() << " send QOS Start Notify " << mxHeader << std::endl;
	SendCtrlMsg(mxHeader);
}

void
GmaVirtualInterface::QosViolationMsg (uint8_t direction)
{
	NS_ASSERT_MSG(m_gmaRxControl->QosSteerEnabled(), "only call this in Qos Enabled mode!!!");
	//prepare the qos testing request msg
	uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();
	MxControlHeader mxHeader;
	mxHeader.SetType (10);
	mxHeader.SetConnectionId (m_linkState->GetHighPriorityLinkCid());
	mxHeader.SetSequenceNumber(m_mxCtrSeqNumber++);
	mxHeader.SetTimeStamp(now & 0xFFFFFFFF);
	mxHeader.SetFlowId(QOS_FLOW_ID);
	mxHeader.SetTrafficDirection(direction);
	std::cout << Now().GetSeconds() << " send QOS Violation MSG " << mxHeader << std::endl;
	SendCtrlMsg(mxHeader);
}

void
GmaVirtualInterface::StopReordering(uint8_t reason){
	//std::cout << Now().GetSeconds() << " node: " << m_nodeId << " **************************STOP REORDERING REASON:";
	
	if(m_enableReordering == true)
	{
		m_enableReordering = false;
		m_newLinkCid = UINT8_MAX;
		m_stopReorderEvent.Cancel();
	}
	else
	{
		//std::cout << "[--Already Stopped--]";
	}
	if(reason == 0)
	{
		//std::cout << "inorder packet arrived at the new link.\n";
	}
	else if(reason == 1)
	{
		//std::cout << "end marker.\n";
	}
	else if(reason == 2)
	{
		//std::cout << "timeout.\n";
	}
	else
	{
		//std::cout << "unkown.\n";
	}
	ReleaseAllPackets();
}

void
GmaVirtualInterface::SendTsu (Ptr<SplittingDecision> decision)
{
	NS_ASSERT_MSG(decision->m_update, "must be true");
	//send probe message every PROBE_INTERVAL

	if(m_serverRoleEnabled)//server tells client QoS violation since client does not do measurement for uplink.
	{
		//std::cout << " fail:"<< m_linkState->m_cidToLastTestFailTime.size() << " expire: " << m_linkState->m_cidToValidTestExpireTime.size() << " " << std::endl;
		auto iter = m_linkState->m_cidToLastTestFailTime.begin();
		while (iter != m_linkState->m_cidToLastTestFailTime.end())
		{
			//this only happen when traffic steer from backup to primary link, all other cases this map should be empty!
			QosViolationMsg(1);//we only implemented uplink case...
			iter++;
		}
	}

	MxControlHeader mxHeader;
	mxHeader.SetType (5);
	mxHeader.SetConnectionId (0);//it will be overwrited before sending to all links.

	mxHeader.SetSequenceNumber(m_mxCtrSeqNumber++);

	if (m_gmaTxControl->QosSteerEnabled() || m_gmaRxControl->QosSteerEnabled())
	{
		mxHeader.SetFlowId (QOS_FLOW_ID);
		if(decision->m_reverse)
		{
			mxHeader.SetReverseFlag (1);
		}
	}
	else
	{
		mxHeader.SetFlowId (BEST_EFFORT_FLOW_ID);
	}

	//update splitting ratio and min owd measurement.
	//if we only need to update min owd measurement, the splitting ratio keeps the same as the previous tsu.
	mxHeader.SetTsuKandMeasureVectors(decision->m_splitIndexList, decision->m_minOwdLongTerm, decision->m_queueingDelay);

	//now the L is the summation of the splitIndexList
	/*uint8_t fieldL = 0;
	for (uint8_t ind = 0; ind < decision->m_splitIndexList.size(); ind++)
	{
		fieldL += decision->m_splitIndexList.at(ind);
	}*/
	uint8_t fieldL = m_gmaRxControl->GetSplittingBurst();
	mxHeader.SetL(fieldL);
	SendCtrlMsg(mxHeader);
	m_tsuCounter++;

	m_lastDecision = decision;


	if(m_receiveDuplicateFlow == false)//sender is not duplicating packets
	{
		if(fieldL == 1)//steer mode, enable reordering.
		{
			//use the cid method to start reordering...
			/*std::vector<uint8_t> cidList = m_linkState->GetCidList();
			for (uint8_t link = 0; link < decision->m_splitIndexList.size(); link++)
			{
				if(decision->m_splitIndexList.at(link) == 1)
				{
					m_newLinkCid = cidList.at (link);
					break;
				}
			}
			NS_ASSERT_MSG(m_newLinkCid != UINT8_MAX, "the cid of the new link cannot be the UINT8_MAX!");

			std::cout << Now().GetSeconds() << " node: " << m_nodeId << " steer mode -> Enable Reordering For :" << m_reorderingTimeout.GetMilliSeconds()<< " ms. New link cid:" << +m_newLinkCid << "\n";
			//steer mode.
			if(m_enableReordering == false)
			{
				//start reordering right now...
				m_enableReordering = true;
			}*/
		}
		else//split mode, enable reordering if TSU indicates both link has data.
		{
			m_lastTsuNoneZeroLink = 0;
			for (uint8_t link = 0; link < decision->m_splitIndexList.size(); link++)
			{
				if(decision->m_splitIndexList.at(link) != 0)
				{
					m_lastTsuNoneZeroLink++;
				}
			}

			NS_ASSERT_MSG(m_lastTsuNoneZeroLink > 0, "TSU cannot be all zeros!!!");

			if(m_lastTsuNoneZeroLink > 1)
			{
				//splitting mode, must enable reordering right way.
				if(m_enableReordering == false)
				{
					m_enableReordering = true;
				}
			}
			//we do not disable reordering for split mode..
		}
	}

}


void
GmaVirtualInterface::RcvReverseTsu (const MxControlHeader& header)
{
	//generate TSU based on the reverse TSU
	NS_ASSERT_MSG(header.GetReverseFlag() == 1, "must be a reverse tsu");
	//update the rx app with the new tsu

	auto splitVector = header.GetKVector();
	NS_ASSERT_MSG(splitVector.size() == m_linkParamsMap.size(), "size not the same!!");

	uint16_t sum_of_elems = 0;
	for(auto it = splitVector.begin(); it != splitVector.end(); ++it)
	{
		sum_of_elems += *it;
	}
	NS_ASSERT_MSG(sum_of_elems > 0, "all links are empty split ratio!!");

	std::map < uint8_t,  Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();
	uint8_t index = 0;
	while (iter!= m_linkParamsMap.end())//iterate all links
	{
		if(splitVector.at(index) !=0 )//traffic over this link
		{
			uint8_t cid = iter->first;
			Ptr<SplittingDecision> decision = m_gmaRxControl->GenerateTrafficSplittingDecision(cid);//this will update the split ratio in the RX app
		}
		iter++;
		index++;
	}
	
	//send forward tsu
	MxControlHeader mxHeader = header;
	mxHeader.SetReverseFlag (0);
	SendCtrlMsg(mxHeader);
	m_tsuCounter++;
}


void
GmaVirtualInterface::SendCtrlMsg (const MxControlHeader& header, bool retx)
{
	//std::cout << Now().GetSeconds() << " " << this<< " Send GMA retx (" << retx <<") Ctr msg: " << header <<"\n";
	uint8_t ip_tos = 0;
	ip_tos += m_sliceId << 2;//add slice id (shift 2 bits) to tos.
	if(header.GetType() == 5)
	{
		//this is TSU message, send TSU over the links with none empty split ratio...

		auto splitVector = header.GetKVector();
		NS_ASSERT_MSG(splitVector.size() == m_linkParamsMap.size(), "size not the same!!");

		uint16_t sum_of_elems = 0;
		for(auto it = splitVector.begin(); it != splitVector.end(); ++it)
		{
     		sum_of_elems += *it;
		}
		NS_ASSERT_MSG(sum_of_elems > 0, "all links are empty split ratio!!");

		std::map < uint8_t,  Ptr<LinkParams> >::iterator iter = m_linkParamsMap.begin();
		uint8_t index = 0;
		while (iter!= m_linkParamsMap.end())//iterate all links
		{
			if(splitVector.at(index) !=0 )//traffic over this link
			{
				uint8_t cid = iter->first;
				MxControlHeader newHeader = header;
				if(m_enableMarkCtrlLinkMap)//set link bitmap for probe and control
				{
					newHeader.SetLinkStatus(m_linkState->IsLinkUp(WIFI_CID));//first bit for wifi cid 0
				}
				else
				{
					newHeader.SetLinkStatus(1);//wifi bit is 1.
				}
				newHeader.SetConnectionId(cid);
				Ptr<Packet> newP = Create<Packet>();
				newP->AddHeader (newHeader);

				GmaHeader gmaHeader;
				newP->AddHeader (gmaHeader);
				//m_linkParamsMap[cid]->m_socket->SendTo (newP, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
				//std::cout << "send over new link:" << +cid << std::endl;
				SendByCid(cid, newP, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F));
				//std::cout << Now().GetSeconds() << " Send TSU:" << newHeader << std::endl;

				if(m_gmaTxControl->QosSteerEnabled() )//QOS enabled
				{
					if (cid != m_linkState->GetHighPriorityLinkCid())
					{
						DiscardBackupLinkPackets(false);// start receiving from backup links... traffic will over backup link.
					}
				}
			}
			iter++;
			index++;
		}
	}
	else if(header.GetType() == 1)
	{
		MxControlHeader newHeader = header;
		if(m_enableMarkCtrlLinkMap)//set linkbitmap for probe and control
		{
			newHeader.SetLinkStatus(m_linkState->IsLinkUp(WIFI_CID));//first bit for wifi cid 0
		}
		else
		{
			newHeader.SetLinkStatus(1);//wifi bit is 1.
		}
		uint8_t cid = newHeader.GetConnectionId();
		Ptr<Packet> newP = Create<Packet>();
		newP->AddHeader (newHeader);

		GmaHeader gmaHeader;
		gmaHeader.SetClientId(m_nodeId);

		newP->AddHeader (gmaHeader);
		//m_linkParamsMap[cid]->m_socket->SendTo (newP, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
		SendByCid(cid, newP, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F));

	}
	else
	{
		uint8_t cid = header.GetConnectionId();
		Ptr<Packet> ctrP = Create<Packet>();
		ctrP->AddHeader (header);

		GmaHeader gmaHeader;
		ctrP->AddHeader (gmaHeader);
		NS_ASSERT_MSG(m_linkParamsMap.find(cid) != m_linkParamsMap.end(), " no such cid in the map");

		//m_linkParamsMap[cid]->m_socket->SendTo (ctrP, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
		SendByCid(cid, ctrP, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F));
	}

	if (retx == false)//this is the first transmission of this control message
	{
		NS_ASSERT_MSG(m_txedCtrQueue.find(header.GetSequenceNumber()) == m_txedCtrQueue.end(), "all mesasges, new or retx should have a unique sn");

		//insert a new control msg to the texed queue
		//in the map, the item with largest sn is put in the back.
		//std::cout << Now().GetSeconds() << " TX queue: send ctrl type:" << +header.GetType() << " SN:" << header.GetSequenceNumber()<< "\n";
		Ptr<TexedCtrQueueItem> item = Create<TexedCtrQueueItem> ();
		item->m_mxControlHeader = header;
		NS_ASSERT_MSG(item->m_csnToTxtimeMap.find(header.GetSequenceNumber()) == item->m_csnToTxtimeMap.end(), "the sequence should not exit yet");
		item->m_csnToTxtimeMap[header.GetSequenceNumber()] = Now(); //tag the csn and send timestamp for this transmission
		m_txedCtrQueue[header.GetSequenceNumber()] = item;
		//RTO equals  2 x max (RTT(k) for all k).

		m_ctrRto = INITIAL_CONTROL_RTO; //reset rto timer;
		if(m_measurementManager->GetMaxRttMs() !=0 )
		{
			//All links have rtt measurement, update  the rto for contrl message retx.
			m_ctrRto = MilliSeconds(RTO_SCALER*m_measurementManager->GetMaxRttMs());
			//std::cout << Now().GetSeconds() << " " << this << " ---------------------Update RTO:" << m_ctrRto.GetSeconds() << "\n";
		}
		Simulator::Schedule(m_ctrRto, &GmaVirtualInterface::RetxCtrlMsgExpires, this, header.GetSequenceNumber());//retx if ack is not received after this event expires
	}
	else
	{
		//the m_txedCtrQueue is already edited in the RetxCtrlMsgExpires function.
		//std::cout << Now().GetSeconds() << " RETX ctrl type:" << +header.GetType() << " SN:" << header.GetSequenceNumber()<< "\n";
	}

}

void
GmaVirtualInterface::RetxCtrlMsgExpires(uint16_t csn)
{
	//std::cout << Now().GetSeconds() << " sn:" <<  csn << " expires ==================\n";
	if(m_txedCtrQueue.find(csn) == m_txedCtrQueue.end())
	{
		//std::cout << " already ACKED ~~~~~~~~\n";
	}
	else
	{
		std::cout << Now().GetSeconds() << " " << this << " first sn:" <<  csn << " sn:" << m_mxCtrSeqNumber<< " expires no ack ================== rto: " <<m_ctrRto << " msg:" << m_txedCtrQueue[csn]->m_mxControlHeader << std::endl;

		if (m_txedCtrQueue[csn]->m_csnToTxtimeMap.size() > MAX_RETX_ATTEMPT)// reach the max number of retx attempts
		{
			uint8_t cid = m_txedCtrQueue[csn]->m_mxControlHeader.GetConnectionId();
			uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
			if(m_linkState->CtrlMsgDown(cid))//cid updates
			{
				Ptr<SplittingDecision> decision = m_gmaRxControl->LinkDownTsu(cid);//chekc if we need tsu to notify the other side
				if(decision->m_update)
				{
					SendTsu(decision);
					if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
					{
						//split mode & default link cid changed, we need to send back a end marker...
						SendEndMarker(oldCid);
					}
				}
			}
			m_txedCtrQueue.erase(csn); //delete this msg from the txedqueue.

		}
		else
		{
			MxControlHeader header = m_txedCtrQueue[csn]->m_mxControlHeader;
			if(header.GetType() == 8 || header.GetType() == 9)
			{
				//for qos testing msg, no retransmission. Let it fail.
				m_linkState->m_cidToLastTestFailTime[header.GetConnectionId()] = Now();//mark the testing time
				m_txedCtrQueue.erase(csn); //delete this msg from the txedqueue.
				if(header.GetType() == 8)
				{
					m_qosClientTestingActive = false;//stop qos testing
				}
			}
			else
			{
				header.SetSequenceNumber(m_mxCtrSeqNumber++); // assign a new ctrl sn
				if (header.GetType() == 1) //update timestamp for Probe messages, type 1
				{
					uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();
					header.SetTimeStamp(now & 0xFFFFFFFF);
				}
				m_txedCtrQueue[csn]->m_mxControlHeader = header; //update the new header;
				m_txedCtrQueue[csn]->m_csnToTxtimeMap[header.GetSequenceNumber()] = Now(); //tag the new csn and its send time
				SendCtrlMsg(header, true);// retx is set to true.

				m_ctrRto = INITIAL_CONTROL_RTO; //reset rto timer;
				if(m_measurementManager->GetMaxRttMs() !=0 )
				{
					//All links have rtt measurement, update  the rto for contrl message retx.
					m_ctrRto = MilliSeconds(RTO_SCALER*m_measurementManager->GetMaxRttMs());
					//std::cout << Now().GetSeconds() << " " << this << " ---------------------Update RTO:" << m_ctrRto.GetSeconds() << "\n";
				}
				Simulator::Schedule(m_ctrRto*std::pow(2, m_txedCtrQueue[csn]->m_csnToTxtimeMap.size()-1), 
					&GmaVirtualInterface::RetxCtrlMsgExpires, this, csn); //schedule retx timeout, double the rto every retx
			}
		}

	}
}
 
void
GmaVirtualInterface::RecvCtrlMsg (Ptr<Packet> packet, const Ipv4Address& phyAddr)
{
	MxControlHeader mxHeader;
	packet->RemoveHeader(mxHeader);
	//std::cout <<Now().GetSeconds() <<" " << +m_nodeId << " Receive Ctrl:" << mxHeader <<  "\n";

	uint8_t ip_tos = 0;
	ip_tos += m_sliceId << 2;//add slice id (shift 2 bits) to tos.

	//add control into owd measurement...
	if(m_enableMeasureReport)
	{
		if(mxHeader.GetType() == 1 || mxHeader.GetType() == 6 || mxHeader.GetType() == 7)
		{
			//measrue owd for probe, ack and tsa. Only they have timestamps...
			uint32_t owd = Now().GetMilliSeconds() - mxHeader.GetTimeStamp();
			uint8_t txCid = mxHeader.GetConnectionId();
			//std::cout << Now().GetSeconds() <<" " << this << " txcid:" << +txCid << " type:" << +mxHeader.GetType() << " timestamp:" << mxHeader.GetTimeStamp() << " ctl owd: "<< +owd << "\n";

			if(m_measureParamPerCidMap.find(txCid) == m_measureParamPerCidMap.end())
			{
				Ptr<MeasureParam> measureParam = Create<MeasureParam>();
				m_measureParamPerCidMap[txCid] = measureParam;
			}

			if(m_measureParamPerCidMap[txCid]->m_maxOwd < owd)
			{
				m_measureParamPerCidMap[txCid]->m_maxOwd = owd;
			}

			if(m_measureParamPerCidMap[txCid]->m_minOwd > owd)
			{
				m_measureParamPerCidMap[txCid]->m_minOwd = owd;
			}
			m_measureParamPerCidMap[txCid]->m_owdSum += owd;
			m_measureParamPerCidMap[txCid]->m_count++;
		}
	}


	if (mxHeader.GetType () == 1 || mxHeader.GetType() == 5)
	{
		//mark link status from TSU or Probe
		if(CsnDiff(mxHeader.GetSequenceNumber(), m_maxLinkMapUpdateSn) > 0)//the msg has larger sn than previous ctrl msg
		{
			//now we only have wifi up and down, 2 cases.
			if(mxHeader.GetLinkStatus() == 0)
			{
				uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
				if(m_linkState->LinkBitMapDown(WIFI_CID))//cid updates
				{
					//link down due to TSU LINK BIT MAP,
					Ptr<SplittingDecision> decision = m_gmaRxControl->LinkDownTsu(WIFI_CID);//chekc if we need tsu to notify the other side
					if(decision->m_update)
					{
						SendTsu(decision);
						if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
						{
							//split mode & default link cid changed, we need to send back a end marker...
							SendEndMarker(oldCid);
						}
					}
				}

			}
			else
			{
				uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
				if(m_linkState->LinkBitMapUp(WIFI_CID))
				{
					//cid updated...
					// wait for probe to nofity the other side
					if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
					{
						//split mode & default link cid changed, we need to send back a end marker...
						SendEndMarker(oldCid);
					}
				}	
			}
		}
		m_maxLinkMapUpdateSn = mxHeader.GetSequenceNumber();

	}

	if (mxHeader.GetType () == 1)//this is a Probe msg, we need to send back ACK
	{
		uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();

		uint8_t cid = mxHeader.GetRCid();

		m_measurementManager->UpdateOwdFromProbe(Now().GetMilliSeconds() - mxHeader.GetTimeStamp(), mxHeader.GetConnectionId());// update OWD

		//std::cout << Now().GetSeconds() << "Receive Probe for link:" << (int)cid << "\n";

		MxControlHeader ackHeader;
		ackHeader.SetType (6);
		ackHeader.SetConnectionId (cid);

		ackHeader.SetSequenceNumber(mxHeader.GetSequenceNumber());
		ackHeader.SetTimeStamp(now & 0xFFFFFFFF);

		//send back ack
		Ptr<Packet> ackPacket = Create<Packet>();
		ackPacket->AddHeader (ackHeader);

		GmaHeader gmaHeader;
		ackPacket->AddHeader (gmaHeader);
		NS_ASSERT_MSG(m_linkParamsMap.find(cid) != m_linkParamsMap.end(), " no such cid in the map");
		//m_linkParamsMap[cid]->m_socket->SendTo (ackPacket, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
		if(mxHeader.GetProbeFlag() == 1)
		{
			SendByCid(cid, ackPacket, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F));//normal Probe
		}
		else
		{
			//test probe, we need to send ACK back with the same IP...
			Ipv4Address addrTemp = m_linkParamsMap[mxHeader.GetConnectionId()]->m_phyAccessContrl->GetIp();
			m_linkParamsMap[mxHeader.GetConnectionId()]->m_phyAccessContrl->SetIp(phyAddr);
			SendByCid(cid, ackPacket, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F));//normal Probe
			m_linkParamsMap[mxHeader.GetConnectionId()]->m_phyAccessContrl->SetIp(addrTemp);

		}

	}
	else if (mxHeader.GetType () == 5)//this is a TSU msg, we need to send back TSA
	{
		std::cout << Now().GetSeconds() << " node: "<< m_nodeId <<" receives TSU:"<< mxHeader << "\n";
		uint8_t maxDelay = 0;
		uint8_t maxDelayChangePerTSU = 127; //since the range of TSA's offset is -127 to 128, we limi the max difference a delay emulation per tsu msg.
		std::vector<int> emulateDelayOffset;
		std::vector<uint8_t> cidList = m_linkState->GetCidList();
		for (uint8_t link = 0; link < cidList.size(); link++)
		{
			emulateDelayOffset.push_back(0); //initial to zero offset.
		}
		if(m_txMode)
		{
			if (CsnDiff(mxHeader.GetSequenceNumber(), m_maxTsuSeqNum) > 0)
			{
				if(m_gmaRxControl->QosSteerEnabled())
				{
					if(m_serverRoleEnabled)
					{
						//if reverse TSU, send back ack and foward TSU
						if(mxHeader.GetReverseFlag() == 1)
						{
							//ACK
							std::cout << Now().GetSeconds() << " Rcv Reverse TSU:" << mxHeader << std::endl;
							RespondAck(mxHeader);

							//Send forward TSU
							RcvReverseTsu (mxHeader);
							return;
						}

					}

					if(m_clientRoleEnabled)
					{
						uint8_t currentCid = m_gmaTxControl->GetDeliveryLinkCid();
						if(currentCid != m_linkState->GetHighPriorityLinkCid())
						{
							m_linkState->m_cidToValidTestExpireTime.erase(currentCid);
							//current link not equal default link, check if this link is idle or qos failure
							if(m_linkState->m_cidToLastTestFailTime.find(currentCid) != m_linkState->m_cidToLastTestFailTime.end())//failed before
							{
								if(Now() > m_linkState->m_cidToLastTestFailTime[currentCid]+ m_linkState->MIN_QOS_TESTING_INTERVAL)//failure is from the previous interval
								{
									//no failure
									m_linkState->m_cidToValidTestExpireTime[currentCid] = Now() + m_linkState->MAX_QOS_VALID_INTERVAL;
								}
							}
							else
							{
								//no failure
								m_linkState->m_cidToValidTestExpireTime[currentCid] = Now() + m_linkState->MAX_QOS_VALID_INTERVAL;
							}

							/*auto iterExpire = m_linkState->m_cidToValidTestExpireTime.begin();
							while(iterExpire!=m_linkState->m_cidToValidTestExpireTime.end())
							{
								std::cout << " expire cid:" << +iterExpire->first << " time:" << iterExpire->second.GetSeconds() << std::endl;
								iterExpire++;
							}*/
						}
					}
				}

				// forwad msg to control only if this TSU has bigger sn
				m_maxTsuSeqNum = mxHeader.GetSequenceNumber();
				uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
				m_gmaTxControl->RcvCtrlMsg(mxHeader);
				if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
				{
					//split mode & default link cid changed, we need to send back a end marker...
					if(m_clientRoleEnabled && m_gmaRxControl->QosSteerEnabled() && m_gmaTxControl->GetDeliveryLinkCid() != m_linkState->GetHighPriorityLinkCid())
					{
						std::cout << "QoS testing success!" <<std::endl;
						m_linkState->m_cidToLastTestFailTime.erase(m_gmaTxControl->GetDeliveryLinkCid());
					}
					SendEndMarker(oldCid);
				}

				//emulate queuing delay based on tsu min owd measurement...
				auto minOwdMeasure = mxHeader.GetOwdVector();
				uint32_t minEmulatedDelay = UINT32_MAX;
				std::cout << " [cid, emulated delay, offset]: ";
				for (uint8_t link = 0; link < cidList.size(); link++)
				{
					auto iter = m_linkParamsMap.find(cidList.at(link));
					if (iter == m_linkParamsMap.end())
					{
						NS_FATAL_ERROR("cannot find cid:" << +cidList.at(link));
					}

					if(minOwdMeasure.at(link) != UINT8_MAX ) //min owd measurement available.
					{
						if (minOwdMeasure.at(link) <= maxDelayChangePerTSU)
						{
							iter->second->m_emulateDelay += minOwdMeasure.at(link);
							emulateDelayOffset.at(link) += minOwdMeasure.at(link); //increase delay offset;
						}
						else
						{
							//limit the max delay change per tsu.
							iter->second->m_emulateDelay += maxDelayChangePerTSU;
							emulateDelayOffset.at(link) += maxDelayChangePerTSU;//increase delay offset;
						}
					}

					if (minEmulatedDelay > iter->second->m_emulateDelay)
					{
						minEmulatedDelay = iter->second->m_emulateDelay;
					}
					std::cout << "[" << +iter->first << " " << iter->second->m_emulateDelay << ", " <<emulateDelayOffset.at(link) << "] ";
				}
				std::cout << std::endl;

				if (minEmulatedDelay == UINT32_MAX)
				{
					NS_FATAL_ERROR("the min of emulated delay should not be UINT32_MAX!");
				}
				else if(minEmulatedDelay > 0)
				{
					std::cout << " min emulated delay not zero: " << minEmulatedDelay << " [cid (order may be different), update emulated delay, update offset]: ";
					for (uint8_t link = 0; link < cidList.size(); link++)
					{
						auto iter = m_linkParamsMap.find(cidList.at(link));
						if (iter == m_linkParamsMap.end())
						{
							NS_FATAL_ERROR("cannot find cid:" << +cidList.at(link));
						}
						// emulated delay - minEmulatedDelay:
						iter->second->m_emulateDelay = iter->second->m_emulateDelay - minEmulatedDelay;
						emulateDelayOffset.at(link) -= minEmulatedDelay;//decrease delay offset;

						std::cout << "[" << +iter->first << " " << iter->second->m_emulateDelay << ", " <<emulateDelayOffset.at(link) << "] ";
						iter++;
					}
					std::cout << std::endl;
				}
				//else minEmulatedDelay == 0, do nothing.

				//"skip" transmitting to link for a duration that equals est queuing delay if queuing delay is greater than 0, and not 255.
				//disable drain queue for now.
				/*auto queueingDelayVector = mxHeader.GetQueueingDelayVector();
				double drainRatio = 0.0;
				for (uint32_t i = 0; i < queueingDelayVector.size(); i++)
				{
					queueingDelayVector.at(i) = drainRatio*queueingDelayVector.at(i);
					if (maxDelay < queueingDelayVector.at(i))
					{
						maxDelay = queueingDelayVector.at(i);
					}
				}*/
				
				//when a link's emulated delay offset is negative, we will stop sending to that link for abs(delay_offset) to prevent out of order pkt.
				std::vector<uint8_t> queueingDelayVector;
				//check if there exist at least one link has data to send and has zero queueing delay...
				bool findLink = false;
				auto kVector = mxHeader.GetKVector();
				for (uint32_t i = 0; i < emulateDelayOffset.size(); i++)
				{
					queueingDelayVector.push_back(0);//initial to zero drain time delay.
					if (emulateDelayOffset.at(i) >= 0 && kVector.at(i) > 0)//no drain time and splitting ratio > 0, have data to send
					{
						findLink = true;
					}
				}

				if (findLink)
				{
					for (uint32_t i = 0; i < emulateDelayOffset.size(); i++)
					{
						if (emulateDelayOffset.at(i) < 0 && kVector.at(i) > 0)//drain time > 0 and splitting ratio > 0, have data to send
						{
							//emulated delay reduced, stop sending for a short time to prevent out of order pkt.
							if (emulateDelayOffset.at(i) < -127)
							{
								NS_FATAL_ERROR("the delay offset should be in the range of -127 ~ 128");
							}
							queueingDelayVector.at(i) = (-1*emulateDelayOffset.at(i));//drain time equals -1 * delay offset.
							if (maxDelay < queueingDelayVector.at(i))
							{
								maxDelay = queueingDelayVector.at(i);//update maxDelay in TSA to delay measurement at receiver
							}
						}
					}
					m_linkState->UpdateLinkQueueingDelay(queueingDelayVector);// stop sending to a link with positive delay.
				}

				//log file
				if(m_saveToFile)
				{
					std::ostringstream fileName;
					fileName <<"rx-tsu-node-"<<m_nodeId<<"-interface-"<<+m_gmaInterfaceId<<".csv";
					std::ofstream myfile;
					myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
					myfile << Simulator::Now ().GetSeconds () << ":\t";
					//std::vector<uint8_t> splitVector = mxHeader.GetKVector();
					//for (uint8_t ind = 0; ind < splitVector.size(); ind++)
					//{
					//	myfile << +splitVector.at(ind)<<",\t";
					//}
					//myfile<< +mxHeader.GetL() <<std::endl;	
					myfile << mxHeader <<std::endl;	
					myfile.close();
				}
			}
		}

		//respond a TSA
		uint8_t cid = mxHeader.GetConnectionId();
		MxControlHeader tsaHeader;
		tsaHeader.SetType (7);
		tsaHeader.SetConnectionId (cid);
		tsaHeader.SetSequenceNumber (mxHeader.GetSequenceNumber ());

		uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();
		tsaHeader.SetTimeStamp(now & 0xFFFFFFFF);
		tsaHeader.SetFlowId(mxHeader.GetFlowId());
		tsaHeader.SetStartSn (m_gmaTxSn);
		tsaHeader.SetDelay (maxDelay);
		std::vector<uint8_t> owdOffset;
	
		for (uint8_t link = 0; link < cidList.size(); link++)
		{
			if (emulateDelayOffset.at(link) < -127 || emulateDelayOffset.at(link)  > 128)
			{
				NS_FATAL_ERROR("the delay offset should be in the range of -127 ~ 128");
			}
			owdOffset.push_back(emulateDelayOffset.at(link)+maxDelayChangePerTSU); // this equals zero for sigend int8.
		}
	
		tsaHeader.SetTsaOwdVectors(owdOffset);
		//send back TSA
		Ptr<Packet> tsaPacket = Create<Packet>();
		tsaPacket->AddHeader (tsaHeader);

		GmaHeader gmaHeader;
		tsaPacket->AddHeader (gmaHeader);
		NS_ASSERT_MSG(m_linkParamsMap.find(cid) != m_linkParamsMap.end(), " no such cid in the map");
		//m_linkParamsMap[cid]->m_socket->SendTo (tsaPacket, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
		SendByCid(cid, tsaPacket, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F));//normal Probe
	}
	else if (mxHeader.GetType () == 6 || mxHeader.GetType () == 7) // i use all ACK asd TSA to updat rtt and owd.
	{
		if(mxHeader.GetType () == 6)
		{
			if(m_linkParamsMap[mxHeader.GetConnectionId()]->m_phyAccessContrl->ProbeAcked(mxHeader.GetSequenceNumber()))
			{
				m_ctrRto = INITIAL_CONTROL_RTO; // reset rto
			}
		}
		//check if the ACK is already ACKed 
		//std::cout << "ACK :" << mxHeader <<  "\n";
		bool ackFound = false;
		uint8_t sendMsgType = 0;
		MxControlHeader sendMsg;
		Time sendTime;

		if(m_txedCtrQueue.find(mxHeader.GetSequenceNumber()) != m_txedCtrQueue.end())
		{
			//This ACK is corresponding to the first transmission
			ackFound = true;
			//the first element in the map is the send time of first transmission
			sendTime = m_txedCtrQueue[mxHeader.GetSequenceNumber()]->m_csnToTxtimeMap.begin()->second; 
			sendMsgType = m_txedCtrQueue[mxHeader.GetSequenceNumber()]->m_mxControlHeader.GetType();
			sendMsg = m_txedCtrQueue[mxHeader.GetSequenceNumber()]->m_mxControlHeader;
			m_txedCtrQueue.erase(mxHeader.GetSequenceNumber()); //delete this msg from the txedqueue.
		}
		else
		{
			//if this ACK is not for the first transmission, check the retransmit sn
			std::map<uint16_t, Ptr<TexedCtrQueueItem> >::iterator iter = m_txedCtrQueue.begin();
			while(iter!=m_txedCtrQueue.end())
			{
				if(iter->second->m_csnToTxtimeMap.find(mxHeader.GetSequenceNumber()) != iter->second->m_csnToTxtimeMap.end())
				{
					//in this case, a retransmission is received.
					ackFound = true;
					//the send time is recorded in the m_csnToTxtimeMap, the key is the csn of that control message.
					sendTime = iter->second->m_csnToTxtimeMap[mxHeader.GetSequenceNumber()];
					sendMsgType = iter->second->m_mxControlHeader.GetType();
					sendMsg = iter->second->m_mxControlHeader;
					m_txedCtrQueue.erase(iter);//delete this msg from the txedqueue
					break;
				}
				iter++;
			}
		}

		if (ackFound)
		{
			//std::cout << "ack found for type:" << +sendMsgType <<std::endl;
			if(sendMsgType == 8)//ack for qos testing request
			{
				std::cout << Now().GetSeconds() << " ACK for QoS Testing Request" << std::endl;
			}
			else if(sendMsgType == 9)//ack for qos testing start notify
			{
				std::cout << Now().GetSeconds() << " ACK for QoS Start Notify" << std::endl;

				if (sendMsg.GetConnectionId() != m_linkState->GetHighPriorityLinkCid())//test over backup link
				{
					if(sendMsg.GetTrafficDirection() == 0 && m_serverRoleEnabled)//downlink testing
					{
						//In downlink, start testing testing at server side...
						std::cout << Now().GetSeconds() <<" Start DL QOS testing at server side" << std::endl;
						if (sendMsg.GetConnectionId() != m_linkState->GetHighPriorityLinkCid())
						{
							m_gmaTxControl->StartQosTesting(sendMsg.GetConnectionId(), sendMsg.GetTestDuration());
						}
						else
						{
							NS_FATAL_ERROR("cid testing cannot be the default link!!");
						}	
					}
					else if(sendMsg.GetTrafficDirection() ==1 && m_serverRoleEnabled)//uplink testing
					{
						//uplink testing should start when qos notify is sent.

					}
					else
					{
						NS_FATAL_ERROR("Did not implement this cases -> direction:"<<+sendMsg.GetTrafficDirection() << " m_clientRoleEnabled:" << m_clientRoleEnabled << "m_serverRoleEnabled:" << m_serverRoleEnabled << std::endl);
					}

				}
				else
				{
					NS_FATAL_ERROR("cid testing cannot be the default link!!");
				}
			}
			//either the ack is for the first transmission or retransmissions.
			//std::cout << "ACK RECEIVED LINK: " << (int)mxHeader.GetConnectionId() << " RTT:" << 
			//Now().GetMilliSeconds() - sendTime.GetMilliSeconds() << " ACK seq: " << mxHeader.GetSequenceNumber() << "\n"; 
			m_measurementManager->UpdateRtt(Now().GetMilliSeconds() - sendTime.GetMilliSeconds(), Now().GetMilliSeconds() - mxHeader.GetTimeStamp(), mxHeader.GetConnectionId());// update RTT

			if (mxHeader.GetType () == 7 && m_rxMode && CsnDiff(mxHeader.GetSequenceNumber(), m_maxTsaSn) > 0)//TSA msg
			{
				//std::cout <<Now().GetSeconds() << " TSA:"<< mxHeader<< "\n";
				//may due to RX measurement based control, or link failure.
				//NS_ASSERT_MSG(m_measurementManager->IsMeasurementOn() == false, "the measurement should already ended!");
				//std::cout << "ACK seq" << mxHeader.GetSequenceNumber() << "\n"; 
				m_maxTsaSn = mxHeader.GetSequenceNumber();
				m_measurementManager->MeasureCycleStartByTsa (mxHeader.GetStartSn(), mxHeader.GetDelay(), mxHeader.GetOwdVector());//start measurement after receive sn# mxHeader.GetStartSn(), if delay is enabled, delay measurement.
				/*if(m_receiveDuplicateFlow == false && m_lastTsuNoneZeroLink == 1)
				{
					//not receiving duplicate flows and all traffic goes to one link mode...
					if(m_enableReordering == true)
					{
						m_enableReordering = false;
						ReleaseAllPackets();
						if (m_reorderingTimeoutEvent.IsRunning())
						{
							m_reorderingTimeoutEvent.Cancel();
						}
					}
				}*/

				if(m_gmaRxControl->QosSteerEnabled() )//client or server side && QOS enabled
				{
					std::cout << Now().GetSeconds() << " RX TSA: " <<mxHeader << std::endl;
					if (mxHeader.GetConnectionId() != m_linkState->GetHighPriorityLinkCid())
					{
						std::cout <<" Start QOS monitoring cid:" <<+mxHeader.GetConnectionId()<< std::endl;
						//receive tsa, start qos monitoring.
						//we will start qos measurement repeated until it fails and fall back to default link
						if(!m_measurementManager->GetObject<QosMeasurementManager>())
						{
							NS_FATAL_ERROR("QosMeasurementManager not initialed");
						}
						bool success = m_measurementManager->GetObject<QosMeasurementManager>()->QosMeasurementStart(mxHeader.GetConnectionId(), 0);//set the duration to 0, the measurement entity will load the duration for QOS test request msg
						NS_ASSERT_MSG(success, "error when running measurement");
					}
				}
			}

			uint8_t cid =  mxHeader.GetConnectionId();
			uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
			if(m_linkState->CtrlMsgUp(cid))
			{
				//cid updated... let delay algorithm to compute split ratio
				/*Ptr<SplittingDecision> decision = m_gmaRxControl->LinkUpTsu(cid);//check if we need tsu to notify the other side
				if(decision->m_update)
				{
					SendTsu(decision);
				}*/
				if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
				{
					//split mode & default link cid changed, we need to send back a end marker...
					SendEndMarker(oldCid);
				}
			}
		}
		else
		{
			//std::cout<< Now().GetSeconds() << " ACK:" <<mxHeader.GetSequenceNumber() <<  " already deleted!!!!!!!!!\n";
		}

	}
	else if (mxHeader.GetType () == 8)//this is a QoS Testing Request, we need to send back ACK
	{
		std::cout << Now().GetSeconds() << " Rcv QoS Testing Request:" << mxHeader << std::endl;
		RespondAck(mxHeader);

		//send QoS start notify Msg...
		QosTestingStartNotifyMsg(mxHeader);
		if (mxHeader.GetConnectionId() != m_linkState->GetHighPriorityLinkCid())//test over backup link
		{
			if(mxHeader.GetTrafficDirection() == 0 && m_serverRoleEnabled)//downlink testing
			{
				//donwlink
			}
			else if(mxHeader.GetTrafficDirection() ==1 && m_serverRoleEnabled)//uplink testing
			{
				//uplink
				std::cout << Now().GetSeconds() <<" Start UL QOS Measurement at server side" << std::endl;
				if(!m_measurementManager->GetObject<QosMeasurementManager>())
				{
					NS_FATAL_ERROR("QosMeasurementManager not initialed");
				}
				bool success = m_measurementManager->GetObject<QosMeasurementManager>()->QosMeasurementStart(mxHeader.GetConnectionId(), mxHeader.GetTestDuration());
				DiscardBackupLinkPackets(true);//Discard packets for qos testing
				NS_ASSERT_MSG(success, "error when running measurement");

			}
			else
			{
				NS_FATAL_ERROR("Did not implement this cases -> direction:"<<+mxHeader.GetTrafficDirection() << " m_clientRoleEnabled:" << m_clientRoleEnabled << "m_serverRoleEnabled:" << m_serverRoleEnabled << std::endl);
			}

		}
		else
		{
			NS_FATAL_ERROR("cid testing cannot be the default link!!");
		}

	}
	else if (mxHeader.GetType () == 9)//this is a QoS Testing Start notify, we need to send back ACK
	{
		std::cout << Now().GetSeconds() << " Rcv QoS Start Notify:" << mxHeader << std::endl;

		m_qosClientTestingActive = true;//start qos testing session if not
		uint8_t qosDuration = m_linkState->m_qosTestDurationUnit100ms;
		Simulator::Schedule(MilliSeconds(qosDuration*100 + m_measurementManager->GetMaxRttMs()), &GmaVirtualInterface::QosTestingSessionEnd, this);//stop qos testing after qosDurationS

		//Recv QoS Testing Start notify
		if (mxHeader.GetConnectionId() != m_linkState->GetHighPriorityLinkCid())
		{
			if(mxHeader.GetTrafficDirection() == 0 && m_clientRoleEnabled)//downlink testing at  client
			{
				std::cout << Now().GetSeconds() <<" Start DL QOS Measurement at client side header:" << mxHeader << std::endl;
				if(!m_measurementManager->GetObject<QosMeasurementManager>())
				{
					NS_FATAL_ERROR("QosMeasurementManager not initialed");
				}
				bool success = m_measurementManager->GetObject<QosMeasurementManager>()->QosMeasurementStart(mxHeader.GetConnectionId(), mxHeader.GetTestDuration());
				if(success)
				{
					//send ack
					RespondAck(mxHeader);
					DiscardBackupLinkPackets(true);//Discard packets for qos testing
				}
				else//fail
				{
					std::cout << "Ignore this msg, measurement is already still running" << std::endl;
				}
			}
			else if(mxHeader.GetTrafficDirection() ==1 && m_clientRoleEnabled)//uplink testing at client
			{
				std::cout << Now().GetSeconds() <<" Start UL QOS testing at client side" << std::endl;
				if (mxHeader.GetConnectionId() != m_linkState->GetHighPriorityLinkCid())
				{
					m_gmaTxControl->StartQosTesting(mxHeader.GetConnectionId(), mxHeader.GetTestDuration());
					RespondAck(mxHeader);
				}
				else
				{
					NS_FATAL_ERROR("cid testing cannot be the default link!!");
				}
			}
			else
			{
				NS_FATAL_ERROR("Did not implement this cases -> direction:"<<+mxHeader.GetTrafficDirection() << " m_clientRoleEnabled:" << m_clientRoleEnabled << "m_serverRoleEnabled:" << m_serverRoleEnabled << std::endl);
			}

		}
		else
		{
			NS_FATAL_ERROR("cid testing cannot be the default link!!");
		}
	}
	else if (mxHeader.GetType () == 10)//this is a QoS Violation MSG
	{
		RespondAck(mxHeader);

		std::cout << Now().GetSeconds() << " Rcv QoS Violation" << std::endl;
		//std::cout << " fail:"<< m_linkState->m_cidToLastTestFailTime.size() << " expire: " << m_linkState->m_cidToValidTestExpireTime.size() << " " << std::endl;
		//std::cout << " default:" << +m_gmaTxControl->GetDeliveryLinkCid() << std::endl;
		uint8_t failCid = m_gmaTxControl->GetDeliveryLinkCid();
		if(failCid != m_linkState->GetHighPriorityLinkCid())
		{
			//current link not equal default link, we set current delivery link as QoS fail!!
			m_linkState->m_cidToLastTestFailTime[failCid] = Now();//link failure
			m_linkState->m_cidToValidTestExpireTime.erase(failCid);
		}

		/*auto iterFail = m_linkState->m_cidToLastTestFailTime.begin();
		while(iterFail!=m_linkState->m_cidToLastTestFailTime.end())
		{
			std::cout << " fail cid:" << +iterFail->first << " time:" << iterFail->second.GetSeconds() << std::endl;
			iterFail++;
		}

		auto iterExpire = m_linkState->m_cidToValidTestExpireTime.begin();
		while(iterExpire!=m_linkState->m_cidToValidTestExpireTime.end())
		{
			std::cout << " expire cid:" << +iterExpire->first << " time:" << iterExpire->second.GetSeconds() << std::endl;
			iterExpire++;
		}*/
	}
	else
	{
		NS_FATAL_ERROR("unknown control message!");
	}

}

void
GmaVirtualInterface::RespondAck(const MxControlHeader& header)
{
	uint64_t now = (uint64_t) Simulator::Now ().GetMilliSeconds ();
	uint8_t cid = header.GetConnectionId();

	MxControlHeader ackHeader;
	ackHeader.SetType (6);
	ackHeader.SetConnectionId (cid);

	ackHeader.SetSequenceNumber(header.GetSequenceNumber());
	ackHeader.SetTimeStamp(now & 0xFFFFFFFF);

	//send back ack
	Ptr<Packet> ackPacket = Create<Packet>();
	ackPacket->AddHeader (ackHeader);

	GmaHeader gmaHeader;
	ackPacket->AddHeader (gmaHeader);
	NS_ASSERT_MSG(m_linkParamsMap.find(cid) != m_linkParamsMap.end(), " no such cid("<< +cid << ") in the map");
	//m_linkParamsMap[cid]->m_socket->SendTo (ackPacket, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
	uint8_t ip_tos = 0;
	ip_tos += m_sliceId << 2;//add slice id (shift 2 bits) to tos.
	SendByCid(cid, ackPacket, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F));//normal Probe
}

int
GmaVirtualInterface::SnDiff(int x1, int x2)
{
	int diff = x1 - x2;
	if (diff > 8388608)
	{
		diff = diff - 16777216;
	}
	else if (diff < -8388608)
	{
		diff = diff + 16777216;
	}
	return diff;
}

int
GmaVirtualInterface::CsnDiff(int x1, int x2)
{
	int diff = x1 - x2;
	if (diff > 32768)
	{
		diff = diff - 65536;
	}
	else if (diff < -32768)
	{
		diff = diff + 65536;
	}
	return diff;
}

int
GmaVirtualInterface::LsnDiff(int x1, int x2)
{
	int diff = x1 - x2;
	if (diff > 128)
	{
		diff = diff - 256;
	}
	else if (diff < -128)
	{
		diff = diff + 256;
	}
	return diff;
}

void
GmaVirtualInterface::SetForwardPacketCallBack(Callback<void, Ptr<Packet> > cb)
{
	m_forwardPacketCallback = cb;
}

void
GmaVirtualInterface::WifiPeriodicPowerTrace(uint8_t cid, uint8_t apId, double power)
{
	//std::cout << Now().GetSeconds() << " node:" << m_nodeId << " cid:" << +cid << " apId:" << +apId << " power:" << power <<"\n";
	NS_ASSERT_MSG(m_linkParamsMap.find(cid) != m_linkParamsMap.end(), " no such cid in the map");
	m_linkParamsMap[cid]->m_phyAccessContrl->ReportRssi(power, apId);


	if(m_wifiPowerAvailable == false)
	{
		m_wifiPowerAvailable = true;
	}

	if(apId == m_linkParamsMap[cid]->m_phyAccessContrl->GetApId())//power trace for connected ap
	{
		if(power < m_wifiLowPowerThreshDbm && m_wifiPowerRange != 0)
	    {
	        //in low power range, and the last message was not low power
	        m_wifiPowerRange = 0;
	        //std::cout <<Now().GetSeconds()  << " last powerdb: " << power << "\n";
	        WifiPowerTrace(m_wifiPowerRange, cid);//send low power signal
	    }
	    else if(power > m_wifiHighPowerThreshDbm && m_wifiPowerRange != 1)
	    {
	        //in high power range, and the last message was not high power
	        m_wifiPowerRange = 1;
	        //std::cout <<Now().GetSeconds() <<" last powerdb: " << power << "\n";
	        WifiPowerTrace(m_wifiPowerRange, cid);//send high power signal
	    }
	    //else in gap range [m_wifiLowPowerThreshDbm, m_wifiHighPowerThreshDbm]
   
	}
}

void
GmaVirtualInterface::SetWifiHighPowerThresh (double highPower)
{
  m_wifiHighPowerThreshDbm = highPower;
}

void
GmaVirtualInterface::SetWifiLowPowerThresh (double lowPower)
{
  m_wifiLowPowerThreshDbm = lowPower;
}


void
GmaVirtualInterface::SetQosRequirement (uint8_t sliceId, uint64_t duration, uint64_t qos_delay, uint64_t tresh1, uint64_t thresh2, double delayTarget, double lossTarget)
{
  m_linkState->m_qosTestDurationUnit100ms = duration/100;
  m_sliceId = sliceId;
  m_acceptableDelay = qos_delay;
  m_delaythresh1 = tresh1;
  m_delaythresh2 = thresh2;

  m_gmaRxControl->SetQosTarget(delayTarget, lossTarget);
}


void
GmaVirtualInterface::WifiPowerTrace(uint8_t powerRange, uint8_t cid)
{
	if(m_enableMarkCtrlLinkMap == false)
	{
		m_enableMarkCtrlLinkMap = true;
	}
	//powerRange 0 stands for low, 1 stands for high power
	//std::cout << Now().GetSeconds() << " node "<< m_nodeId<< " !!!!!!!!!!!!!!!!!!!!!!!!!cid:" << +cid << " rxpower range:" << +powerRange <<"\n";
	if(powerRange == 0)//low receive power
	{
		uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
	    if(m_linkState->LowRssiDown(cid))//this may change default Link CID
	    {
			Ptr<SplittingDecision> decision = m_gmaRxControl->LinkDownTsu(cid);//chekc if we need tsu to notify the other side
			if(decision->m_update)
			{
				SendTsu(decision);
				if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
				{
					//split mode & default link cid changed, we need to send back a end marker...
					SendEndMarker(oldCid);
				}
			}
		}
	}
	else if(powerRange == 1)//high receive power
	{
		uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
		if(m_linkState->HighRssiUp(cid))
		{
			//wait for delay algorihtm to update tsu
			//cid updated... 
			/*Ptr<SplittingDecision> decision = m_gmaRxControl->LinkUpTsu(cid);//chekc if we need tsu to notify the other side
			if(decision->m_update)
			{
				SendTsu(decision);
			}*/
			if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
			{
				//split mode & default link cid changed, we need to send back a end marker...
				SendEndMarker(oldCid);
			}
		}

	}
	else
	{
		//std::cout << "power level: " << +powerRange << "\n";
		NS_FATAL_ERROR("unkonw power level, it should be only 0(low) or 1(high)");
	}
}

void
GmaVirtualInterface::SendEndMarker(uint8_t cid)
{
	GmaHeader gmaHeader;

	gmaHeader.SetConnectionId(cid); // later I might remove this since the CID may be referenced from port number
	if (m_gmaTxControl->QosSteerEnabled() || m_gmaRxControl->QosSteerEnabled())
	{
		//this does not matter since we never use the flow id of the end marker.
		gmaHeader.SetFlowId (QOS_FLOW_ID);
	}
	else
	{
		gmaHeader.SetFlowId (BEST_EFFORT_FLOW_ID);
	}

	Ptr<Packet> dummyP = Create<Packet>();
	dummyP->AddHeader(gmaHeader);
	//std::cout <<Now().GetSeconds() <<" TX IP: " <<ipv4Header.GetSource()<<  " -> " << ipv4Header.GetDestination() <<" link CID:" << +cid
	//<< " SN:" << m_gmaTxSn << " LSN:"<< +m_linkParamsMap[cid]->m_gmaTxLocalSn << "\n";
	//std::cout << Now().GetSeconds() << " node: "<< m_nodeId <<" Respond end marker over link " <<+cid <<"\n";

	NS_ASSERT_MSG (m_linkParamsMap.find(cid) != m_linkParamsMap.end(), "this cid doesnot exit");
	//m_linkParamsMap[cid]->m_socket->SendTo (dummyP, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
	uint8_t ip_tos = 0;
	ip_tos += m_sliceId << 2;//add slice id (shift 2 bits) to tos.
	SendByCid(cid, dummyP, (TOS_AC_BE & 0xE0) + (ip_tos & 0x1F));//normal Probe
}

void
GmaVirtualInterface::SendByCid(uint8_t cid, Ptr<Packet> pkt, uint8_t tos)
{
	NS_ASSERT_MSG (m_linkParamsMap.find(cid) != m_linkParamsMap.end(), "this cid doesnot exit");
	if (m_linkParamsMap[cid]->m_emulateDelay > 0)
	{
		//emulate queueing delay, this might cause out of order.
    	Simulator::Schedule(MilliSeconds(m_linkParamsMap[cid]->m_emulateDelay), &GmaVirtualInterface::SendByCidNow, this, cid, pkt, tos);
	}
	else
	{
		//send now
		SendByCidNow(cid, pkt, tos);
	}
}

void
GmaVirtualInterface::SendByCidNow(uint8_t cid, Ptr<Packet> pkt, uint8_t tos)
{

	//check if this link is down due to simulated Wi-Fi AP switch...
	bool okeyToTx = true;

	if(okeyToTx)
	{
		//std::cout << "TX to IP:" << m_linkParamsMap[cid]->m_ipAddr << " port:" <<START_PORT_NUM+cid << "!\n";
		//m_linkParamsMap[cid]->m_phyAccessContrl->GetSocket()->SendTo (pkt, 0 ,InetSocketAddress (m_linkParamsMap[cid]->m_ipAddr, START_PORT_NUM+cid));
		NS_ASSERT_MSG(m_linkParamsMap.find(cid) != m_linkParamsMap.end(), " no such cid in the map");
		if(m_linkParamsMap[cid]->m_phyAccessContrl->SendPacket(pkt, tos))
		{
			//link down flag true
			m_ctrRto = INITIAL_CONTROL_RTO; //reset rto timer;
			//IP changed -> Wi-Fi to Wi-Fi handover, we set the control msg failed... Do not use this link until at least one control msg is received from this link.
			uint8_t oldCid = m_gmaTxControl->GetDeliveryLinkCid();
			if(m_linkParamsMap[cid]->m_phyAccessContrl->GetLinkDownTime() > Seconds(0))//simulate single radio...
			{
				if(m_linkState->CtrlMsgDown(cid))//cid updates
				{
					Ptr<SplittingDecision> decision = m_gmaRxControl->LinkDownTsu(cid);//chekc if we need tsu to notify the other side
					if(decision->m_update)
					{
						SendTsu(decision);
						if(m_gmaRxControl->GetSplittingBurst() == 1 && oldCid != m_gmaTxControl->GetDeliveryLinkCid())//steer mode
						{
							//split mode & default link cid changed, we need to send back a end marker...
							SendEndMarker(oldCid);
						}
					}
				}
			}
		}
	}
	else
	{
		//std::cout << "drop pakt due to simulated Wi-Fi AP switch... No TX!\n";
	}

}

void
GmaVirtualInterface::EnableServerRole (uint32_t clientId)
{
	m_serverRoleEnabled = true;
	m_clientId = clientId;
}

void
GmaVirtualInterface::EnableClientRole (uint32_t clientId)
{
	m_clientRoleEnabled = true;
	m_clientId = clientId;
}

void
GmaVirtualInterface::DiscardBackupLinkPackets (bool flag)
{
	m_discardBackupLinkPackets = flag;
}

void
GmaVirtualInterface::QosTestingSessionEnd()
{
	m_qosClientTestingActive = false;
}

void
GmaVirtualInterface::MonitorSniffRx(std::string path, 
                Ptr<const Packet> packet,
                uint16_t channelFreqMhz,
                WifiTxVector txVector,
                MpduInfo aMpdu,
                SignalNoiseDbm signalNoise,
                uint16_t staId)

{

	//std::cout << "path:" << path << " staId:" << staId <<  " signal: " << signalNoise.signal << " noise:" << signalNoise.noise << std::endl;

	std::string s = path;
	std::string delimiter = "/";

	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		//std::cout << token << std::endl;
		s.erase(0, pos + delimiter.length());
		if (token.compare("DeviceList") == 0)
		{
			break;
		}
	}

	//int device_id = std::stoi(s.substr(0, pos));

	//std::cout << "nodeId:" << m_nodeId << " device_id:" <<device_id << " signal:" << signalNoise.signal << std::endl;
	WifiMacHeader wifiHeader;
    packet->PeekHeader(wifiHeader);
	if (wifiHeader.IsBeacon() && wifiHeader.GetAddr1().IsGroup())
	{
		//std::cout << " addr1: " << wifiHeader.GetAddr1 () << " addr2: " << wifiHeader.GetAddr2 () << " addr3: " << wifiHeader.GetAddr3 () << " addr4: " << wifiHeader.GetAddr4 () <<std::endl;
		int cellId = -1;
		if(m_linkParamsMap.find(WIFI_CID) != m_linkParamsMap.end())
		{
			cellId = m_linkParamsMap[WIFI_CID]->m_phyAccessContrl->GetApIdFromMacAddr(wifiHeader.GetAddr2 ());//not sure to user addr2 or addr3??
		}
		if (cellId == -1)
		{
			NS_FATAL_ERROR("Cannot find wifi cell id for this user.");
		}
		//std::cout << path << " nodeId:" << m_nodeId << " device_id:" <<device_id << " signal:" << signalNoise.signal << std::endl;
		//std::cout << wifiHeader << std::endl;
		// The first device is LTE, We need to change this para if multiple link is enabled.
		WifiPeriodicPowerTrace(WIFI_CID, cellId, signalNoise.signal);
	}
}

}


