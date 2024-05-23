/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "link-state.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LinkState");

NS_OBJECT_ENSURE_REGISTERED (LinkState);

LinkState::LinkState ()
{
  NS_LOG_FUNCTION (this);
}


TypeId
LinkState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LinkState")
    .SetParent<Object> ()
    .SetGroupName("Gma")
    .AddAttribute ("HighPriorityLinkCid",
               "The cid of the high priority link",
               UintegerValue (CELLULAR_LTE_CID),
               MakeUintegerAccessor (&LinkState::m_highPriorityLinkCid),
               MakeUintegerChecker<uint8_t> ())
	.AddAttribute ("LowPriorityLinkCid",
               "The cid of the low priority link",
               UintegerValue (WIFI_CID),
               MakeUintegerAccessor (&LinkState::m_lowPriorityLinkCid),
               MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("FixDefaultLink",
           "If true, the default link cannot be changed by link up/down triggers",
           BooleanValue (false),
           MakeBooleanAccessor (&LinkState::m_fixDefaultLink),
           MakeBooleanChecker ())
	.AddAttribute ("PreferLargeCidValue",
           "if true: large cid -> high priority; if false: small cid -> high priority. This is used for choosing new default link after link failure.",
           BooleanValue (true),
           MakeBooleanAccessor (&LinkState::m_preferLargeCidValue),
           MakeBooleanChecker ())
	.AddAttribute ("QosTestDurationUnit100ms",
               "The QoS test duration multiplied by 100ms",
               UintegerValue (1),
               MakeUintegerAccessor (&LinkState::m_qosTestDurationUnit100ms),
               MakeUintegerChecker<uint8_t> ())
  ;
  return tid;
}


LinkState::~LinkState ()
{
	NS_LOG_FUNCTION (this);
}

void
LinkState::SetFixedDefaultCid(uint8_t cid)
{
	m_highPriorityLinkCid = cid;
	m_lowPriorityLinkCid = cid;
	m_fixDefaultLink = true;
}

bool
LinkState::IsLinkUp(uint8_t cid)
{
	if(m_ctrFailedLinkMap.find(cid) == m_ctrFailedLinkMap.end() 
		&& m_lowQualityLinkMap.find(cid) == m_lowQualityLinkMap.end()
		 && m_bitmapFailedLinkMap.find(cid) == m_bitmapFailedLinkMap.end())
	{
		//this link is not failed, not low quality, nor tsu indicate down, --> link should be up
		return true;
	}
	else
	{
		//this link is either low quality, failed, or tsu indicate down -> this link should be down
		return false;
	}
}

void
LinkState::SetId(uint32_t id)
{
	m_nodeId = id;
}

void
LinkState::NoDataDown(uint8_t cid)
{
	CtrlMsgDown(cid);// use the same process as control msg down.
} 

bool
LinkState::CtrlMsgDown(uint8_t cid)
{
	bool update = false;
	if(IsLinkUp(cid))//if link is still up, need to set it as down!
	{
		m_ctrFailedLinkMap[cid] = true;//set this link as failed, m_linkState->IsLinkUp will return false now
		std::cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << " control message retx failed or no data -> LINK DOWN\n";
		update = UpdateDefaultLink();
	}
	else
	{
		if(m_ctrFailedLinkMap.find(cid) == m_ctrFailedLinkMap.end())
		{
			std::cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << " control message retx failed or no data \n";
			m_ctrFailedLinkMap[cid] = true;//set this link as failed, m_linkState->IsLinkUp will return false now
		}
	}
	return update;
}

bool
LinkState::CtrlMsgUp(uint8_t cid)
{
	bool update = false;
	if(!IsLinkUp(cid))//link is down
	{
		if(m_ctrFailedLinkMap.find(cid) != m_ctrFailedLinkMap.end())
		{
			m_ctrFailedLinkMap.erase(cid);//this will not mark this link as failed link any more

			if(IsLinkUp(cid))//if the link is up ,we need to notify the control
			{
				std::cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << " control message received -> LINK UP\n";
				UpdateDefaultLink();
				update = true;
			}
			else
			{
				std::cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << " control message received\n";
			}
		}
	}
	return update;
}

bool
LinkState::LowRssiDown(uint8_t cid)
{
	bool update = false;
	if(IsLinkUp(cid))//if link is still up, need to set it as down!
	{
		m_lowQualityLinkMap[cid] = true;//set this link as low quality, m_linkState->IsLinkUp will return false now
		std::cout << Now().GetSeconds() <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid <<  ". RSSI low -> LINK DOWN\n";
		update = UpdateDefaultLink();
		//Ptr<SplittingDecision> decision = m_gmaRxControl->LinkDown(cid);
		//m_gmaTxControl->LinkDown(cid);//if this link is the default reverse channel, change to a different one

		//if(decision->m_update)
		//{
		//	SendTsu(decision);
		//}
	}
	else
	{
		//link is already down.
		if(m_lowQualityLinkMap.find(cid) == m_lowQualityLinkMap.end())
		{
			std::cout << Now().GetSeconds() <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << ". RSSI low\n";
			m_lowQualityLinkMap[cid] = true;//set this link as low quality, m_linkState->IsLinkUp will return false now
		}
	}
	return update;
}

bool
LinkState::HighRssiUp(uint8_t cid)
{
	bool update = false;
	if(!IsLinkUp(cid))//link is down
	{
		//signal strength is high, this link is not low quality anymore.
		if(m_lowQualityLinkMap.find(cid) != m_lowQualityLinkMap.end())
		{
			m_lowQualityLinkMap.erase(cid);//this will not mark this link as low quality any more.

			if(IsLinkUp(cid))//if the link is up ,we need to notify the control
			{
				std::cout << Now().GetSeconds() <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << ". RSSI high -> LINK UP\n";
				UpdateDefaultLink();
				update = true;
				//m_gmaRxControl->LinkUp(cid);
				//m_gmaTxControl->LinkUp(cid);

			}
			else
			{
				std::cout << Now().GetSeconds() <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << ". RSSI high\n";
			}
		}
		
	}
	//else if the link is already up, nothing to do	
	return update;
}

bool
LinkState::LinkBitMapDown(uint8_t cid)
{
	bool update = false;
	//wifi link down due to tsu. mark in the bit map
	if(IsLinkUp(cid))//if wifi link is still up, need to set it as down!
	{

		m_bitmapFailedLinkMap[cid] = true;//set this link as failed, m_linkState->IsLinkUp will return false now
		std::cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << " Link Bit Map DOWN -> LINK DOWN\n";
		update = UpdateDefaultLink();
	}
	else
	{
		if(m_bitmapFailedLinkMap.find(cid) == m_bitmapFailedLinkMap.end())
		{
			std::cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << " Link Bit Map Down.\n";
			m_bitmapFailedLinkMap[cid] = true;//set this link as failed, m_linkState->IsLinkUp will return false now
		}
	}
	return update;
}

bool
LinkState::LinkBitMapUp(uint8_t cid)
{
	bool update = false;
	///wifi is marked up from TSU
	if(!IsLinkUp(cid))//link is down
	{

		if(m_bitmapFailedLinkMap.find(cid) != m_bitmapFailedLinkMap.end())
		{
			m_bitmapFailedLinkMap.erase(cid);//this will not mark this link as failed link any more

			if(IsLinkUp(cid))//if the link is up ,we need to notify the control
			{
				std::cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << " Link Bit Map UP -> LINK UP\n";
				UpdateDefaultLink();
				update = true;
			}
			else
			{
				std::cout <<  "!!!!!!!!!!!!!!!!!!!!!!!!!node:" << +m_nodeId << " cid:" << +cid << "Link Bit Map Up.\n";
			}
		}
	}
	return update;
	//else if the link is already up, nothing to do	
}

bool
LinkState::UpdateDefaultLink()
{
	if(m_fixDefaultLink)
	{
		//do not update the default link ID
		return false;
	}

	//find the min and max cid.
	uint8_t minOkCid = UINT8_MAX;
	int maxOkCid = -1;

	for (uint8_t ind = 0; ind < m_cidList.size(); ind++)//find a link with min cid still ok as default
	{
		if(IsLinkUp(m_cidList.at(ind)) && (int)m_cidList.at(ind) > maxOkCid)
		{
			maxOkCid = m_cidList.at(ind);
		}
		if(IsLinkUp(m_cidList.at(ind)) && m_cidList.at(ind) < minOkCid)
		{
			minOkCid = m_cidList.at(ind);
		}
	}
	
	std::cout << " minOkCid: " << +minOkCid << " maxOkCid: " << +maxOkCid << std::endl;

	if(maxOkCid != -1)
	{
		if(m_preferLargeCidValue)//high priority -> large cid
		{
			std::cout << "LINK STATE: change m_highPriorityLinkCid from " << +m_highPriorityLinkCid << " to :" << +maxOkCid << "\n";
			m_highPriorityLinkCid = maxOkCid;
		}
		else
		{
			std::cout << "LINK STATE: change m_lowPriorityLinkCid from " << +m_lowPriorityLinkCid << " to :" << +maxOkCid << "\n";
			m_lowPriorityLinkCid = maxOkCid;
		}
	}

	if(minOkCid != UINT8_MAX)
	{
		if(m_preferLargeCidValue)//high priority -> large cid
		{
			std::cout << "LINK STATE: change m_lowPriorityLinkCid from " << +m_lowPriorityLinkCid << " to :" << +minOkCid << "\n";
			m_lowPriorityLinkCid = minOkCid;
		}
		else
		{
			std::cout << "LINK STATE: change m_highPriorityLinkCid from " << +m_highPriorityLinkCid << " to :" << +minOkCid << "\n";
			m_highPriorityLinkCid = minOkCid;
		}
	}

	return true;
}


void
LinkState::AddLinkCid(uint8_t cid)
{
	std::cout << "Link State add link cid:" << +cid << "\n";
	m_cidList.push_back(cid);
	m_cidToIndexMap[cid]=m_cidList.size()-1;
}

std::vector<uint8_t> 
LinkState::GetCidList()
{
	return m_cidList;
}


uint8_t
LinkState::GetHighPriorityLinkCid()
{

	return m_highPriorityLinkCid;
}

uint8_t
LinkState::GetLowPriorityLinkCid()
{
	return m_lowPriorityLinkCid;
}

std::string
LinkState::ConvertCidFormat(int cid)
{
	std::string cidStr = "";
	if (cid == WIFI_CID)
	{
		cidStr = "wifi";
	}
	else if(cid == CELLULAR_LTE_CID)
	{
		cidStr = "lte";
	}
	else if(cid == CELLULAR_NR_CID)
	{
		cidStr = "nr";
	}
	else if(cid == NETWORK_CID)
	{
		cidStr = "network";
	}
	else
	{
		NS_FATAL_ERROR("unkown cid int format.");
	}
	return cidStr;
}

//convert format from string to int
int
LinkState::ConvertCidFormat(std::string cidStr)
{
	int cid;
	if (cidStr.compare("wifi") == 0)
	{
		cid = WIFI_CID;
	}
	else if (cidStr.compare("lte") == 0)
	{
		cid = CELLULAR_LTE_CID;
	}
	else if (cidStr.compare("nr") == 0)
	{
		cid = CELLULAR_NR_CID;
	}
	else if (cidStr.compare("network") == 0)
	{
		cid = NETWORK_CID;
	}
	else
	{
		NS_FATAL_ERROR("unkonw CID string format");
	}
	return cid;
}

void
LinkState::UpdateLinkQueueingDelay (std::vector<uint8_t> queueingDelayVector)
{
	//we skip a link for a duration that equals the queueing delay to drain the queue.
	NS_ASSERT_MSG(queueingDelayVector.size() == m_cidList.size(), "The size of the delay measurement and cid list is not the same.");

	std::cout << Now().GetMilliSeconds() << " [cid, drain delay, expire time]: ";
	for (uint8_t link = 0; link < m_cidList.size(); link++)
	{
		if(queueingDelayVector.at(link) != UINT8_MAX && queueingDelayVector.at(link) > 0) //queuing delay available.
		{
			uint8_t drainTime = queueingDelayVector.at(link);
			m_skipCidMap[m_cidList.at(link)] = Now().GetMilliSeconds() + drainTime;
			std::cout << "[" << +m_cidList.at(link) << ", " <<+drainTime <<", "  << +m_skipCidMap[m_cidList.at(link)] << "] ";
		}
		else
		{
			if (m_skipCidMap.find(m_cidList.at(link)) != m_skipCidMap.end())//find map from previous update
			{
				if(m_skipCidMap[m_cidList.at(link)] < Now().GetMilliSeconds() )//map already expired
				{
					//already expired map, we can remove it now.
					m_skipCidMap.erase(m_cidList.at(link));
				}
				else
				{
					//we receive a not synced tsu, may due to link down or link up.
					std::cout << "This should only happen after link is up or link down. time: " << Now().GetMilliSeconds () <<  std::endl;

					//remove the delay
					m_skipCidMap.erase(m_cidList.at(link));
					/*auto iterS = m_skipCidMap.begin();
					while (iterS!=m_skipCidMap.end())
					{
						std::cout << " cid: " << +iterS->first << " value: " << iterS->second << std::endl;
						iterS++;
					}*/
				}
			}
			std::cout << "[" << +m_cidList.at(link) << ", " <<+queueingDelayVector.at(link) <<", NA] ";
		}

	}
	std::cout << std::endl;	

	if (m_skipCidMap.size() == m_cidList.size())
	{
		NS_FATAL_ERROR("cannot have all link be skipped!!!!");
	}
}

bool
LinkState::IsLinkLowQueueingDelay(uint8_t cid)
{
	if (m_skipCidMap.find(cid) == m_skipCidMap.end())//link is not in the skip map
	{
		//link is not skipped, therefore the queueing delay is low
		return true;
	}
	else
	{
		//found link in skip map, check if map is expired
		if(m_skipCidMap[cid] <= Now().GetMilliSeconds() )//map already expired
		{
			//already expired map, we can remove it now.
			m_skipCidMap.erase(cid);
			return true; //the queue should be drained, and the queueing delay is low
		}
	}
	return false;//high queueing delay.
}

void
LinkState::StopLinkSkipping ()
{
	m_skipCidMap.clear();
}

}
