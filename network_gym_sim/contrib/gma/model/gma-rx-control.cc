/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "gma-rx-control.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GmaRxControl");

NS_OBJECT_ENSURE_REGISTERED (GmaRxControl);

GmaRxControl::GmaRxControl ()
{
  NS_LOG_FUNCTION (this);
  m_updateThreshold = 0.03;
}


TypeId
GmaRxControl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GmaRxControl")
    .SetParent<Object> ()
    .SetGroupName("Gma")
    .AddAttribute ("SplittingAlgorithm", 
    			"Receiver side traffic splitting algorithm.",
               EnumValue<GmaRxAlgorithm> (GmaRxControl::Delay),
               MakeEnumAccessor<GmaRxAlgorithm>(&GmaRxControl::m_algorithm),
               MakeEnumChecker (GmaRxControl::Delay, "optimize average delay",
			   					GmaRxControl::gma, "same as Delay, optimize average delay",
              					GmaRxControl::CongDelay, "after primary link congests, optimize average delay",
								GmaRxControl::QosSteer, "qos steer mode",
              					GmaRxControl::FixedHighPriority, "default high priority link"))
    .AddAttribute ("SplittingBurst",
               "The splitting burst size for traffic spliting algorithm, support 1(steer mode), 4, 8, 16, 32, 64, 128",
               UintegerValue (1),
               MakeUintegerAccessor (&GmaRxControl::m_splittingBurst),
               MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("DelayThresh",
               "traffic split only if the max link delay > min link dleay + DelayThresh [ms]",
			   DoubleValue (5),
               MakeDoubleAccessor (&GmaRxControl::m_delayThresh),
               MakeDoubleChecker<double> ())
    .AddAttribute ("EnableAdaptiveStep",
	           "If true, enable adaptive steps",
	           BooleanValue (true),
	           MakeBooleanAccessor (&GmaRxControl::m_enableAdaptiveStep),
	           MakeBooleanChecker ())
    .AddAttribute ("EnableStableAlgorithm",
	           "If true, enable stable version of the algorithm",
	           BooleanValue (true),
	           MakeBooleanAccessor (&GmaRxControl::m_enableStableAlgorithm),
	           MakeBooleanChecker ())
    .AddAttribute ("EnableLossAlgorithm",
	           "If true, if two links have the same delay range, favor the low loss one",
	           BooleanValue (true),
	           MakeBooleanAccessor (&GmaRxControl::m_enableLossAlgorithm),
	           MakeBooleanChecker ())
	.AddAttribute ("QosDelayViolationTarget",
               "the max acceptable ratio of high delay packet for Qos Flow",
			   DoubleValue (0.01),
               MakeDoubleAccessor (&GmaRxControl::m_qosDelayViolationTarget),
               MakeDoubleChecker<double> ())
	.AddAttribute ("QosLossTarget",
			"the max acceptable ratio of packet loss for qos flow",
			DoubleValue (1.0),
			MakeDoubleAccessor (&GmaRxControl::m_qosLossTarget),
			MakeDoubleChecker<double> ())
    .AddAttribute ("EnableQosFlowPrioritization",
	           "If true, Enable Qos Flow Prioritization",
	           BooleanValue (false),
	           MakeBooleanAccessor (&GmaRxControl::m_enableQosFlowPrioritization),
	           MakeBooleanChecker ())
  ;
  return tid;
}


GmaRxControl::~GmaRxControl ()
{
	NS_LOG_FUNCTION (this);
}

void
GmaRxControl::SetLinkState(Ptr<LinkState> linkstate)
{
	m_linkState = linkstate;
}

void
GmaRxControl::SetSplittingBurst(uint8_t splittingBurst)
{
	NS_ASSERT_MSG(splittingBurst == 1 || splittingBurst == 4 || splittingBurst == 8 || splittingBurst == 16 || splittingBurst == 32 || splittingBurst == 128,
			"Now only support size 1, 4, 8 16, 32, 64 and 128.");
	if (splittingBurst == 0)
	{
		//enable adaptive splitting burst.
		m_splittingBurst = GetMinSplittingBurst();
		m_adaptiveSplittingBurst = true;
	}
	else
	{
		m_splittingBurst = splittingBurst;
		m_adaptiveSplittingBurst = false;
	}
}

uint8_t
GmaRxControl::GetSplittingBurst(void)
{
	return m_splittingBurst;
}

void
GmaRxControl::SetDelayThresh(double delay)
{
	NS_ASSERT_MSG(delay >= 0, "delay thresh must be greater than 0");
	m_delayThresh = delay;
}

void
GmaRxControl::SetAlgorithm (std::string algorithm)
{
	if (algorithm == "Delay")
	{
		m_algorithm = GmaRxControl::Delay;
	}
	else if (algorithm == "gma")
	{
		m_algorithm = GmaRxControl::gma;
	}
	else if (algorithm == "CongDelay")
	{
		m_algorithm = GmaRxControl::CongDelay;
	} 
	else if (algorithm == "FixedHighPriority")
	{
		m_algorithm = GmaRxControl::FixedHighPriority;
	}
	else if (algorithm == "RlSplit")
	{
		m_algorithm = GmaRxControl::RlSplit;
	} 
	else if (algorithm == "QosSteer")
	{
		m_algorithm = GmaRxControl::QosSteer;
	} 
	else
	{
		NS_FATAL_ERROR ("algorithm not supported");
	}
}

Ptr<SplittingDecision>
GmaRxControl::GetTrafficSplittingDecision (Ptr<RxMeasurement> measurement)
{
	if(measurement->m_links == 0)
	{
		NS_FATAL_ERROR("Link cannot be 0!");
	}

	NS_ASSERT_MSG(measurement->m_links == measurement->m_cidList.size(), "size does not match!");
	NS_ASSERT_MSG(measurement->m_links == measurement->m_delayThisInterval.size(), "size does not match!");
	NS_ASSERT_MSG(measurement->m_links == measurement->m_delayList.size(), "size does not match!");
	NS_ASSERT_MSG(measurement->m_links == measurement->m_minOwdLongTerm.size(), "size does not match!");
	NS_ASSERT_MSG(measurement->m_links == measurement->m_lossRateList.size(), "size does not match!");
	//oscialltion happens because the optimal ratio may not be acheivable due to the finite number of splitting burst size.
	//In this case, the split ratio may bounce between 2 or few values.
	Ptr<SplittingDecision> params;

    if(m_algorithm == GmaRxControl::Delay)
	{
		params = DelayAlgorithm (measurement);
	}
	else if(m_algorithm == GmaRxControl::gma)
	{
		params = DelayAlgorithm (measurement);
	}
	else if(m_algorithm == GmaRxControl::CongDelay)
	{
		params = CongDelayAlgorithm (measurement);
	}
	else if(m_algorithm == GmaRxControl::FixedHighPriority)
	{
		//std::cout << "no update \n";
		Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
		decision->m_update = false;
		return decision;
	}
	else if(m_algorithm == GmaRxControl::RlSplit)
	{
		//std::cout << "no update \n";
		Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
		decision->m_update = false;
		return decision;
	}
	else
	{
		NS_FATAL_ERROR("Unknown link selection method");
	}


	return params;
}

Ptr<SplittingDecision>
GmaRxControl::GetTrafficSplittingDecision (Ptr<RlAction> action)
{
	if(action->m_links == 0)
	{
		NS_FATAL_ERROR("Link cannot be 0!");
	}

	NS_ASSERT_MSG(action->m_links == action->m_cidList.size(), "size does not match!");
	NS_ASSERT_MSG(action->m_links == action->m_ratio.size(), "size does not match!");
	//oscialltion happens because the optimal ratio may not be acheivable due to the finite number of splitting burst size.
	//In this case, the split ratio may bounce between 2 or few values.
	bool update = false;
	std::vector<uint8_t> splittingIndexList;
	for (uint8_t ind = 0; ind < action->m_links; ind++)
	{
		uint8_t cid = action->m_cidList.at(ind);
		uint8_t ratio = action->m_ratio.at(ind);
		if (m_lastSplittingIndexList.size() == 0)
		{
			update = true;
			std::vector<uint8_t> cidList = m_linkState->GetCidList();

			for (uint8_t i = 0; i < cidList.size(); i++)
			{
				if(cidList.at(i) == cid)
				{
					splittingIndexList.push_back(ratio);
				}
			}

		}
		else if (m_lastSplittingIndexList.at(m_linkState->m_cidToIndexMap[cid]) != ratio)
		{
			update = true;
			m_lastSplittingIndexList.at(m_linkState->m_cidToIndexMap[cid]) =  ratio;
		}
	}

	if(m_lastSplittingIndexList.size() > 0)
	{
		splittingIndexList = m_lastSplittingIndexList;
	}
	//check if the sum equals splitting burst size;
	uint8_t sumRatio = 0;
	for (uint8_t ind = 0; ind < action->m_links; ind++)
	{
		sumRatio += splittingIndexList.at (ind);
	}
	//For ML algorithm, we can use any splitting burst size.
	//NS_ASSERT_MSG(sumRatio == m_splittingBurst, "the summation must equals the splitting burst size");
	if(sumRatio != m_splittingBurst)
	{
		m_splittingBurst = sumRatio;
		NS_ASSERT_MSG(m_splittingBurst == 1 || m_splittingBurst == 4 || m_splittingBurst == 8 || m_splittingBurst == 16 || m_splittingBurst == 32 || m_splittingBurst == 64 || m_splittingBurst == 128,
			"Now only support size 1, 4, 8 16, 32, 64 and 128.");
	}

	Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
	decision->m_splitIndexList = splittingIndexList;
	decision->m_update = update;
	//std::cout << " update:" << decision->m_update << " decision:" << +decision->m_splitIndexList.at(0) << " " <<+decision->m_splitIndexList.at(1) << " splittingBurst size:" << +m_splittingBurst<< std::endl;
	return decision;

}

Ptr<SplittingDecision>
GmaRxControl::CongDelayAlgorithm (Ptr<RxMeasurement> measurement)
{
	//start from all traffic goes to the first link
	if(m_lastSplittingIndexList.size() == 0)
	{
		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			if(measurement->m_cidList.at(ind) == m_linkState->GetLowPriorityLinkCid())
			{
				m_lastSplittingIndexList.push_back(m_splittingBurst);
			}
			else
			{
				m_lastSplittingIndexList.push_back(0);
			}
			m_decreaseCounter.push_back(1);//all link start counter from 1
			if(m_enableStableAlgorithm)
			{
				m_lastOwd.push_back(measurement->m_delayList.at(ind));//initial last owd to the same as the first one
			}
		}
	}
	
	//std::cout << "RX APP ";
	if(m_lastSplittingIndexList.at(m_linkState->m_cidToIndexMap[m_linkState->GetLowPriorityLinkCid()]) == m_splittingBurst)
	{
		//std::cout << " Wi-Fi only loss:" << measurement->m_lossRateList.at(0);
		//all traffic goes to primary link, detect congestion
		if (measurement->m_lossRateList.at(0) > CONGESTION_LOSS_THRESHOLD)//detects congestion as packet loss
		{
			//use delay algorithm if primary link is congested.
			//std::cout <<" -> delay algorithm. \n";
			return DelayAlgorithm (measurement);
		}
		else
		{
			//not congested, stay in primary link
			//std::cout << "no update \n";
			Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
			decision->m_update = false;
			return decision;
		}
	}
	else
	{
		//already start splitting, keep using delay algorithm
		//std::cout <<"more than one link is used -> delay algorithm. \n";
		return DelayAlgorithm (measurement);
	}



}

Ptr<SplittingDecision>
GmaRxControl::DelayAlgorithm (Ptr<RxMeasurement> measurement, bool useMinOwd)
{
	if (useMinOwd)
	{
		//overwrite the delay measurement with owd.
		measurement->m_delayList = measurement->m_minOwdLongTerm;
		
	}
	//start from all traffic goes to the first link
	if(m_lastSplittingIndexList.size() == 0)
	{
		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			if(measurement->m_cidList.at(ind) == m_linkState->GetLowPriorityLinkCid())
			{
				m_lastSplittingIndexList.push_back(m_splittingBurst);
			}
			else
			{
				m_lastSplittingIndexList.push_back(0);
			}
			m_decreaseCounter.push_back(1);//all link start counter from 1
			if(m_enableStableAlgorithm)
			{
				m_lastOwd.push_back(measurement->m_delayList.at(ind));//initial last owd to the same as the first one
			}
		}
	}

	//we have steps in the delay algorithm:
	//(1) if max owd - min owd > Thresh, move traffic from max owd link to min owd link;
	//(2) all links have same delay, if max loss - min loss > loss thresh, move traffic from max loss link to min loss link;
	//(3) [For steer mode: all links have same delay and same loss, if Wi-Fi RSSI is high, move traffic to wifi].
	//find the max and min delay and index of all links.
	double minDelay = 0;
	double maxDelay = 0;
	uint8_t minIndex = 0; 
	uint8_t maxIndex = 0;
	bool initialDelay = false;
	//initial min and max link cid to any link with traffic.

	for (uint8_t ind = 0; ind < measurement->m_links; ind++)
	{
		if(m_lastSplittingIndexList.at(ind) != 0)
		{
			minDelay = measurement->m_delayList.at(ind);
			minIndex = ind;
			maxDelay = measurement->m_delayList.at(ind);
			maxIndex = ind;
			initialDelay = true;
			break;
		}
	}
	NS_ASSERT_MSG(initialDelay == true, "cannot initialize the min and max delay index");

	for (uint8_t ind = 0; ind < measurement->m_links; ind++)
	{
		if(m_linkState->IsLinkUp(measurement->m_cidList.at(ind)))//link is okey
		{
			if(minDelay > measurement->m_delayList.at(ind))
			{
				//find a link with lower delay. this can be idle link (no traffic)
				minDelay = measurement->m_delayList.at(ind);
				minIndex = ind;
			}

			if(maxDelay < measurement->m_delayList.at(ind) && m_lastSplittingIndexList.at(ind) != 0)
			{
				//find a link with active traffic and with higher delay,
				maxDelay = measurement->m_delayList.at(ind);
				maxIndex = ind;
			}

		}
	}

	//change ratio only if |max delay - min delay| > m_delayThresh, in order to make this algorithm converge

	//std::cout << Now().GetSeconds() << " minDelay: " << minDelay << " last minDelay: " << m_lastOwd.at(minIndex) << " minIndex: " << +minIndex << " minCid: " << +measurement->m_cidList.at(minIndex) << std::endl;
	//std::cout << Now().GetSeconds() << " maxDelay: " << maxDelay << " last maxDelay: " << m_lastOwd.at(maxIndex) << " maxIndex: " << +maxIndex << " maxCid: " << +measurement->m_cidList.at(maxIndex) << std::endl;

	bool update = false;
	if( maxDelay - minDelay > m_delayThresh)
	{
		if (m_enableStableAlgorithm == false || (m_enableStableAlgorithm == true && m_lastOwd.at(maxIndex) <= measurement->m_delayList.at(maxIndex) ))
		{
			//if statble algorithm not enabled, 
			//or stable algorithm enabled and this measurement delay is not decreasing
			if(m_lastSplittingIndexList.at(maxIndex) > 0)
			{
				if(m_enableAdaptiveStep)
				{
					for (uint8_t ind = 0; ind < m_decreaseCounter.size(); ind++)
					{
						if(ind == maxIndex)
						{
							//increase the number of decrease counter
							m_decreaseCounter.at(maxIndex)++;
						}
						else
						{
							//for other links, reset to 1.
							m_decreaseCounter.at(ind) = 1;
						}
					}
					//increase step if the maxIndex link continue decreases multiple times
					int step = std::max((int)m_decreaseCounter.at(maxIndex) - STEP_THRESH, 1);

					if (step >= m_lastSplittingIndexList.at(maxIndex))
					{
						//make sure the m_lastSplittingIndexList.at(maxIndex)-step not equal to zero!
						step = m_lastSplittingIndexList.at(maxIndex);
					}

					m_lastSplittingIndexList.at(maxIndex) -= step;
					m_lastSplittingIndexList.at(minIndex) += step;
				}
				else
				{
					m_lastSplittingIndexList.at(maxIndex)--;
					m_lastSplittingIndexList.at(minIndex)++;
				}
				update = true;
			}
		}
	}
	else//all links have same delay, reset all decrease counter to 1.
	{
		for (uint8_t ind = 0; ind < m_decreaseCounter.size(); ind++)
		{
			m_decreaseCounter.at(ind) = 1; //reset to 1.
		}
	}

	if(m_enableStableAlgorithm)
	{
		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			m_lastOwd.at(ind) = measurement->m_delayList.at(ind);
		}

	}

	if(update == false && (maxDelay - minDelay <= m_delayThresh) && m_enableLossAlgorithm)//the delay difference of all links are small.
	{
		//find the max and min Loss and index of all links.
		double minLoss = 0;
		double maxLoss= 0;
		uint8_t minLossInd = 0;
		uint8_t maxLossInd = 0;

		bool initialLoss = false;

		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			if(m_linkState->IsLinkUp(measurement->m_cidList.at(ind)))//link is okey
			{
				if(initialLoss == false)//min max loss not initialized yet
				{
					minLoss = measurement->m_lossRateList.at(ind);
					minLossInd = ind;
					maxLoss = measurement->m_lossRateList.at(ind);
					maxLossInd = ind;
					initialLoss = true;
				}
				else
				{
					if(minLoss > measurement->m_lossRateList.at(ind))
					{
						minLoss = measurement->m_lossRateList.at(ind);
						minLossInd = ind;
					}

					if(maxLoss < measurement->m_lossRateList.at(ind) && m_lastSplittingIndexList.at(ind) != 0)
					{
						maxLoss = measurement->m_lossRateList.at(ind);
						maxLossInd = ind;
					}
				}
			}
		}

		if(maxLoss > minLoss * LOSS_ALGORITHM_BOUND)
		{
			if(m_lastSplittingIndexList.at(maxLossInd) > 0)
			{
				//no need to use adaptive algorithm here
				m_lastSplittingIndexList.at(maxLossInd)--;
				m_lastSplittingIndexList.at(minLossInd)++;
				update = true;
			}
		}
		else
		{
			//For steer mode, if same delay and same loss, we check if the default link is up...
			//make sure the min and max is not the same link! If minIndex and maxIndex is the same link, we stay on this link.
			if(m_splittingBurst == 1 && minIndex != maxIndex && m_linkState->IsLinkUp(m_linkState->GetLowPriorityLinkCid()))
			{
				//steer mode, we move all traffic over wifi...
				NS_ASSERT_MSG(m_linkState->m_cidToIndexMap.find(m_linkState->GetLowPriorityLinkCid()) != m_linkState->m_cidToIndexMap.end(), "cannot find this cid in the map");
				if(m_lastSplittingIndexList.at(m_linkState->m_cidToIndexMap[m_linkState->GetLowPriorityLinkCid()]) == m_splittingBurst)
				{
					//do nothing...traffic is over Default link already
				}
				else
				{
					for (uint8_t ind = 0; ind < measurement->m_links; ind++)
					{
						if(measurement->m_cidList.at(ind) == m_linkState->GetLowPriorityLinkCid())
						{
							m_lastSplittingIndexList.at(ind) = m_splittingBurst;
							update = true;
						}
						else
						{
							m_lastSplittingIndexList.at(ind) = 0;
						}
					}
					NS_ASSERT_MSG(update, "Default CID should be in the cid list!!!");
				}
			}

		}
		//std::cout << " >>> max loss:" << maxLoss << " index:" << +maxLossInd << " min loss:" <<minLoss << " index:" << +minLossInd << "\n";
	}

	if(update)
	{
		/*std::cout << "RX APP ======= ";
		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			std::cout << "[link:" << +ind << " delay:"<<measurement->m_delayList.at (ind)
			<< " loss:" << measurement->m_lossRateList.at (ind)<<"] ";
		}
		std::cout << "\n";
		std::cout << " >>>>>> max delay:" << maxDelay << " index:" << +maxIndex << " min delay:" <<minDelay << " index:" << +minIndex << "\n";
		std::cout << " >>>>>> ";
		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			std::cout << "[link:" << +ind << " ratio:"<<+m_lastSplittingIndexList.at (ind)<< "] ";
		}
		std::cout << "\n";*/
	}

	
	//check if the sum equals splitting burst size;
	uint8_t sumRatio = 0;
	for (uint8_t ind = 0; ind < measurement->m_links; ind++)
	{
		sumRatio += m_lastSplittingIndexList.at (ind);
	}

	NS_ASSERT_MSG(sumRatio == m_splittingBurst, "the summation must equals the splitting burst size");

	Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
	decision->m_splitIndexList = m_lastSplittingIndexList;
	decision->m_update = update;
	return decision;

}

Ptr<SplittingDecision>
GmaRxControl::LinkDownTsu (uint8_t cid)
{
	bool update = false;

	if(m_algorithm == GmaRxControl::FixedHighPriority)
	{
		//do nothing
	}
	else
	{

		if(m_lastSplittingIndexList.size() == 0)
		{
			auto defaultCid = m_linkState->GetLowPriorityLinkCid();
			if (QosSteerEnabled())
			{
				//only qos steer use high priority as default.
				defaultCid = m_linkState->GetHighPriorityLinkCid();
			}

			if(cid != defaultCid)
			{
				//if no TSU sent yet, send one just in case...
				update = true;

				std::vector<uint8_t> splittingIndexList;
				std::vector<uint8_t> cidList = m_linkState->GetCidList();

				for (uint8_t ind = 0; ind < cidList.size(); ind++)
				{
					if(cidList.at(ind) == defaultCid)
					{
						splittingIndexList.push_back(m_splittingBurst);
					}
					else
					{
						splittingIndexList.push_back(0);
					}
				}

				/*std::cout << "RX APP link (CID):" << +cid << " failed\n";

				std::cout << "RX APP";
				for (uint8_t ind = 0; ind < splittingIndexList.size(); ind++)
				{
					std::cout << "[link:" << +ind << " ratio:"<<+splittingIndexList.at (ind)<< "] ";
				}
				std::cout << "\n";*/

				Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
				decision->m_splitIndexList = splittingIndexList;
				decision->m_update = update;
				return decision;

			}
			//else link down cid is the same as the new default link, no need to send tsu

		}
		else
		{
			NS_ASSERT_MSG(m_linkState->m_cidToIndexMap.find(cid) != m_linkState->m_cidToIndexMap.end(), "cannot find this cid in the map");

			uint8_t failedLink = m_linkState->m_cidToIndexMap[cid];//get the index of the failed link

			if(m_lastSplittingIndexList.at(failedLink) == 0)
			{
				//this link already have zero traffic splitting, do nothing
				update = false;
			}
			else
			{
				bool allLinkDown = true;
				std::vector<uint8_t> cidList = m_linkState->GetCidList();

				for (uint16_t ind = 0; ind < m_lastSplittingIndexList.size(); ind++ )
				{
					if(m_linkState->IsLinkUp(cidList.at(ind)))
					{
						allLinkDown = false;
						break;
					}
				}

				if(allLinkDown)
				{
					//all link down, do nothing..
					update = false;
				}
				else
				{

					update = true;

					uint8_t splittingIndex = m_lastSplittingIndexList.at(failedLink);
					m_lastSplittingIndexList.at(failedLink) = 0;//set its split index = 0
					uint8_t iterInd = 0;
					while(splittingIndex != 0)//distribute its split index to all links that are ok (not failed)
					{
						if(m_linkState->IsLinkUp(cidList.at(iterInd)))
						{
							m_lastSplittingIndexList.at(iterInd)++;
							splittingIndex--;
						}
						iterInd = (iterInd + 1) % m_lastSplittingIndexList.size();
					}
					/*std::cout << "RX APP link:" << +failedLink << " failed\n";

					std::cout << "RX APP";
					for (uint8_t ind = 0; ind < m_lastSplittingIndexList.size(); ind++)
					{
						std::cout << "[link:" << +ind << " ratio:"<<+m_lastSplittingIndexList.at (ind)<< "] ";
					}
					std::cout << "\n";*/
					//check if the sum equals splitting burst size;
					uint8_t sumRatio = 0;
					for (uint8_t ind = 0; ind < m_lastSplittingIndexList.size(); ind++)
					{
						sumRatio += m_lastSplittingIndexList.at (ind);
					}

					NS_ASSERT_MSG(sumRatio == m_splittingBurst, "the summation must equals the splitting burst size");

				}

			}
		}
	}

	Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
	decision->m_splitIndexList = m_lastSplittingIndexList;
	decision->m_update = update;
	return decision;

}

Ptr<SplittingDecision>
GmaRxControl::QosSteerAlgorithm (Ptr<RxMeasurement> measurement)
{
	NS_ASSERT_MSG(m_splittingBurst == 1, "this algorithm is for steering mode only");
	//NS_ASSERT_MSG (measurement->m_links==2, "qos steer should only measure 2 links, 1 primary and 1 backup");
	//NS_ASSERT_MSG (measurement->m_cidList.size()==2, "qos steer should only measure 2 links, 1 primary and 1 backup");
	//start from all traffic goes to the first link
	//the measurtement may record more than 2 links now, since the backup link can be changed by the ml action.

	if(m_lastSplittingIndexList.size() == 0)
	{
		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			if(measurement->m_cidList.at(ind) == m_linkState->GetHighPriorityLinkCid())
			{
				m_lastSplittingIndexList.push_back(m_splittingBurst);
			}
			else
			{
				m_lastSplittingIndexList.push_back(0);
			}
		}
	}

	//implement qos based traffic steering here!!!
	bool update = false;

	//we need to be careful that the traffic may be measured from the primary link, the backup link and other (the previous backup link).
	uint16_t trafficOverPrimaryLink = 0;
	uint16_t trafficOverOtherLinks = 0; //traffic over the previous backuplink.

	uint8_t primaryLinkIndex = m_linkState->m_cidToIndexMap[m_linkState->GetHighPriorityLinkCid()];
	std::vector<uint8_t> otherCidList;
	for (uint8_t ind = 0; ind < measurement->m_links; ind++)
	{
		if(ind == primaryLinkIndex)
		{
			trafficOverPrimaryLink += m_lastSplittingIndexList.at(ind);
		}
		else
		{
			trafficOverOtherLinks += m_lastSplittingIndexList.at(ind);
			otherCidList.push_back(measurement->m_cidList.at(ind));
		}
	}
	//std::cout<< " primary cid:" << +m_linkState->GetHighPriorityLinkCid() << " primary ind:" << +primaryLinkIndex
	//	<< " backup cid:" << +backupCid << std::endl;
	//trafficOverBackupLinks == 0 -> traffic over the primary link
	//trafficOverPrimaryLink == 0 -> traffic over the backup link
	//trafficOverBackupLinks !=0 && trafficOverPrimaryLink != 0 -> traffic over primary and backup links. (testing mode)

	if(trafficOverOtherLinks == 0) //no traffic over backup or other link, received measurement for QoS testing...
	{
		std::vector<uint8_t> qosOkList;
		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			if(ind != primaryLinkIndex && m_linkState->IsLinkUp(measurement->m_cidList.at(ind)))//not the primary link and this link is okey
			{
				if(measurement->m_highDelayRatioList.at(ind) >= 0 && measurement->m_highDelayRatioList.at(ind) < m_qosDelayViolationTarget 
					&& measurement->m_lossRateList.at(ind) >= 0 && measurement->m_lossRateList.at(ind) < m_qosLossTarget)
				{
					//pass the check of qos requirement
					qosOkList.push_back(ind);//add this link into qos OK list
				}
			}
		}

		//std::cout  << "links:" << +measurement->m_links << " Traffic over primary link ("<<+m_linkState->GetHighPriorityLinkCid()<<"):" << trafficOverPrimaryLink
		//<< " Traffic over Backup link("<<+backupCid<<"):" << trafficOverBackupLinks 
		//<< " m_lastTestingTime:" << m_linkState->m_cidToLastTestFailTime[backupCid].GetSeconds()
		//<< " m_lastTestingTime+m_linkState->MIN_QOS_TESTING_INTERVAL:" << (m_linkState->m_cidToLastTestFailTime[backupCid]+m_linkState->MIN_QOS_TESTING_INTERVAL).GetSeconds()
		//<< " now:" << Now().GetSeconds();

		//std::cout << " qosOkList:";
		//for (uint32_t i = 0; i < qosOkList.size(); i++)
		//{
		//	std::cout << +qosOkList.at(i) << "";
		//}
		//std::cout << std::endl;

		if(qosOkList.empty())//qos fail or no traffic
		{
			for (uint32_t i = 0; i < otherCidList.size(); i++)
			{
				if(measurement->m_highDelayRatioList.at(m_linkState->m_cidToIndexMap[otherCidList.at(i)]) >= 0 && measurement->m_lossRateList.at(m_linkState->m_cidToIndexMap[otherCidList.at(i)]) >= 0)//measurement available, and qos fail...
				{
					m_linkState->m_cidToLastTestFailTime[otherCidList.at(i)] = Now();//record the failed time
					m_linkState->m_cidToValidTestExpireTime.erase(otherCidList.at(i));
					
				}
				else//idle case
				{
					//m_linkState->m_cidToLastTestFailTime.erase(otherCidList.at(i));//remove this cid, client can re start testing right away (after receiving a packet)
				}
			}
		}
		else if(qosOkList.size() == 1)//qos pass for 1 link, switch to this link
		{
			
			//traffic over both primary and backup links, in testing mode -> switch to backup link that meet qos requirement
			for (uint8_t ind = 0; ind < measurement->m_links; ind++)
			{
				if(ind == qosOkList.at(0))
				{
					m_lastSplittingIndexList.at(ind) = m_splittingBurst;
				}
				else
				{
					m_lastSplittingIndexList.at(ind) = 0;
				}
			}
			update = true;
			m_linkState->m_cidToLastTestFailTime.erase(measurement->m_cidList.at(qosOkList.at(0)));//remove this cid, client can re start testing right away (after receiving a packet) if it fails again
			m_linkState->m_cidToValidTestExpireTime[measurement->m_cidList.at(qosOkList.at(0))] = Now() + m_linkState->MAX_QOS_VALID_INTERVAL; // we assume the qos of this cid link is valid for m_linkState->MAX_QOS_VALID_INTERVAL.

		}
		else //multiple links meet the requirement... did not implement this case yet
		{
			NS_FATAL_ERROR("did not implement testing over multiple backup links yet...");
		}
	}
	else //traffic over backup link, check qos requirement.
	{
		std::vector<uint8_t> qosOkList;
		for (uint8_t ind = 0; ind < measurement->m_links; ind++)
		{
			if(ind != primaryLinkIndex && m_linkState->IsLinkUp(measurement->m_cidList.at(ind)))//not the primary link and this link is okey
			{
				if(measurement->m_highDelayRatioList.at(ind) >= 0 && measurement->m_highDelayRatioList.at(ind) < m_qosDelayViolationTarget 
					&& measurement->m_lossRateList.at(ind) >= 0 && measurement->m_lossRateList.at(ind) < m_qosLossTarget)
				{
					//pass the check of qos requirement
					qosOkList.push_back(ind);
				}
			}
		}
		/*std::cout << Now().GetSeconds() << " QOS Monitoring Measurement End. qosOkList:";
		for (uint32_t i = 0; i < qosOkList.size(); i++)
		{
			std::cout << +qosOkList.at(i) << "";
		}
		std::cout << std::endl;*/

		if(qosOkList.empty())//may be qos fail or no traffic
		{
			//qos check failed, fall back to LTE...
			for (uint8_t ind = 0; ind < measurement->m_links; ind++)
			{
				if(ind == primaryLinkIndex)
				{
					m_lastSplittingIndexList.at(ind) = m_splittingBurst;
				}
				else
				{
					m_lastSplittingIndexList.at(ind) = 0;
				}
			}
			update = true;

			for (uint32_t i = 0; i < otherCidList.size(); i++)
			{
				if(measurement->m_highDelayRatioList.at(m_linkState->m_cidToIndexMap[otherCidList.at(i)]) >= 0 && measurement->m_lossRateList.at(m_linkState->m_cidToIndexMap[otherCidList.at(i)]) >= 0)//measurement available, and qos fail...
				{
					m_linkState->m_cidToLastTestFailTime[otherCidList.at(i)] = Now();//record the failed time
					m_linkState->m_cidToValidTestExpireTime.erase(otherCidList.at(i));
				}
				else//idle case
				{
					//m_linkState->m_cidToLastTestFailTime.erase(otherCidList.at(i));//remove this cid, client can re start testing right away (after receiving a packet)
				}
			}
		}
		else if(qosOkList.size() == 1)//qos pass for 1 link, continue sending on this link...
		{
			//check if the passed link is the current link...
			for (uint8_t ind = 0; ind < measurement->m_links; ind++)
			{
				if(ind == qosOkList.at(0))
				{
					if (m_lastSplittingIndexList.at(ind) == m_splittingBurst)
					{
						//traffic is over this link, no action
						m_linkState->m_cidToLastTestFailTime.erase(measurement->m_cidList.at(qosOkList.at(0)));//remove this cid, client can re start testing right away (after receiving a packet) if it fails again
					}
					else
					{
						//NS_FATAL_ERROR("did not implement QOS testing over multiple links yet..");
						//backup link changed, qos test also passed!
						for (uint8_t itemp = 0; itemp < measurement->m_links; itemp++)
						{
							if(itemp == ind)
							{
								m_lastSplittingIndexList.at(itemp) = m_splittingBurst;
							}
							else
							{
								m_lastSplittingIndexList.at(itemp) = 0;
							}
						}
						m_linkState->m_cidToLastTestFailTime.erase(measurement->m_cidList.at(qosOkList.at(0)));//remove this cid, client can re start testing right away (after receiving a packet) if it fails again
						update = true;
					}
				}
			}
			m_linkState->m_cidToValidTestExpireTime[measurement->m_cidList.at(qosOkList.at(0))] = Now() + m_linkState->MAX_QOS_VALID_INTERVAL; // we assume the qos of this cid link is valid for m_linkState->MAX_QOS_VALID_INTERVAL.

		}
		else //multiple links meet the requirement... did not implement this case yet
		{
			NS_FATAL_ERROR("did not implement testing over multiple backup links yet...");
		}

	}

	std::cout << "RX APP ======= ";
	for (uint8_t ind = 0; ind < measurement->m_links; ind++)
	{
		std::cout << "[link:" << +ind << " cid:" << +measurement->m_cidList.at(ind) <<" delay:"<<measurement->m_delayList.at (ind)
		<< " highdelay:" << measurement->m_highDelayRatioList.at (ind)
		<< " loss:" << measurement->m_lossRateList.at (ind)<<"] ";
	}
	std::cout << "\n";

	std::cout << "RX APP >>>>>>> ratio";
	for (uint8_t ind = 0; ind < measurement->m_links; ind++)
	{
		std::cout << " ["<<+m_lastSplittingIndexList.at (ind)<< "]";
	}
	if(update)
	{
		std::cout << " -> TSU ";
	}
	std::cout << "\n";

	Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
	decision->m_splitIndexList = m_lastSplittingIndexList;
	decision->m_update = update;
	return decision;
}

bool
GmaRxControl::QosSteerEnabled ()
{
	return false;
}

bool
GmaRxControl::QosFlowPrioritizationEnabled ()
{
	return false;
}

bool
GmaRxControl::QoSTestingRequestCheck (uint8_t cid)
{
	 return false;

}

bool
GmaRxControl::QoSValidCheck (uint8_t cid)
{
	return false;

}

Ptr<SplittingDecision>
GmaRxControl::GenerateTrafficSplittingDecision (uint8_t cid, bool reverse)
{
	bool update = true;
	//NS_ASSERT_MSG(m_linkState->m_cidToIndexMap.find(cid) != m_linkState->m_cidToIndexMap.end(), "cannot be end of this map cid:" << +cid);

	Ptr<SplittingDecision> decision = Create<SplittingDecision> ();

	std::vector<uint8_t> cidList = m_linkState->GetCidList();

	for (uint8_t i = 0; i < cidList.size(); i++)
	{
		if(cidList.at(i) == cid)
		{
			decision->m_splitIndexList.push_back(m_splittingBurst);
		}
		else
		{
			decision->m_splitIndexList.push_back(0);
		}
	}

	if(!reverse)
	{
		m_lastSplittingIndexList = decision->m_splitIndexList;
	}
	//for reverse TSU, do not update m_lastSplittingIndexList.

	std::cout << "RX APP Generate TSU for cid:" << +cid << " ---- ";
	for (uint8_t ind = 0; ind < decision->m_splitIndexList.size(); ind++)
	{
		std::cout << +decision->m_splitIndexList.at(ind) << "";
	}
	std::cout << std::endl;
	decision->m_update = update;
	decision->m_reverse = reverse;
	return decision;

}

int
GmaRxControl::GetMinSplittingBurst()
{
	return 8;
}

int
GmaRxControl::GetMaxSplittingBurst()
{
	return 128;
}

int
GmaRxControl::GetMeasurementBurstRequirement()
{
	if (m_splittingBurstScalar*m_splittingBurst == 0)
	{
		NS_FATAL_ERROR("the splitting burst requiremment cannot be zero!!!");
	}
	return m_splittingBurstScalar*m_splittingBurst;
}


uint32_t 
GmaRxControl::GetQueueingDelayTargetMs()
{
	return 	m_queueingDelayTargetMs;
}

void
GmaRxControl::SetQosTarget(double delayTarget, double lossTarge)
{
  m_qosDelayViolationTarget = delayTarget;
  m_qosLossTarget = lossTarge;
}

Ptr<SplittingDecision>
GmaRxControl::DelayViolationAlgorithm (Ptr<RxMeasurement> measurement)
{	
	Ptr<SplittingDecision> decision = Create<SplittingDecision> ();
	decision->m_splitIndexList = m_lastSplittingIndexList;
	decision->m_update = false;
	return decision;

}

}
