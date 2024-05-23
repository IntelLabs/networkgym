/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "gma-tx-control.h"
#include <random>       // std::default_random_engine

namespace ns3 {

static const int REVERSE_BITS_INDEX4[4] = {0, 2, 1, 3}; //reversed index for 4 bits
static const int REVERSE_BITS_INDEX8[8] = {0, 4, 2, 6, 1, 5, 3, 7}; //reversed index for 8 bits
static const int REVERSE_BITS_INDEX16[16] = {0,  8,  4, 12,  2, 10,  6, 14,  1,  9,  5, 13,  3, 11,  7, 15}; //reversed index for 16 bits
static const int REVERSE_BITS_INDEX32[32] = {0, 16,  8, 24,  4, 20, 12, 28,  2, 18, 10, 26,  6, 22, 14, 30,
  1, 17, 9, 25,  5, 21, 13, 29,  3, 19, 11, 27,  7, 23, 15, 31}; //reversed index for 32 bits
static const int REVERSE_BITS_INDEX64[64] = {0, 32, 16, 48, 8, 40, 24, 56, 4, 36, 20, 52, 12, 44, 28, 60,
 2, 34, 18, 50, 10, 42, 26, 58, 6, 38, 22, 54, 14, 46, 30, 62,
  1, 33, 17, 49, 9, 41, 25, 57, 5, 37, 21, 53, 13, 45, 29, 61,
   3, 35, 19, 51, 11, 43, 27, 59, 7, 39, 23, 55, 15, 47, 31, 63};
static const int REVERSE_BITS_INDEX128[128] = {0, 64, 32, 96, 16, 80, 48, 112, 8, 72, 40, 104, 24, 88, 56, 120, 4, 68, 36, 100, 20, 84, 52, 116, 12, 76, 44, 108, 28, 92, 60, 124,
 2, 66, 34, 98, 18, 82, 50, 114, 10, 74, 42, 106, 26, 90, 58, 122, 6, 70, 38, 102, 22, 86, 54, 118, 14, 78, 46, 110, 30, 94, 62, 126,
  1, 65, 33, 97, 17, 81, 49, 113, 9, 73, 41, 105, 25, 89, 57, 121, 5, 69, 37, 101, 21, 85, 53, 117, 13, 77, 45, 109, 29, 93, 61, 125,
   3, 67, 35, 99, 19, 83, 51, 115, 11, 75, 43, 107, 27, 91, 59, 123, 7, 71, 39, 103, 23, 87, 55, 119, 15, 79, 47, 111, 31, 95, 63, 127}; //reversed index for 128 bits


NS_LOG_COMPONENT_DEFINE ("GmaTxControl");

NS_OBJECT_ENSURE_REGISTERED (GmaTxControl);

GmaTxControl::GmaTxControl ()
{
  NS_LOG_FUNCTION (this);
  m_splittingIndex = 0;
}


TypeId
GmaTxControl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GmaTxControl")
    .SetParent<Object> ()
    .SetGroupName("Gma")
    .AddAttribute ("SplittingAlgorithm", 
    			"Transmitter side traffic splitting algorithm.",
               EnumValue<GmaTxAlgorithm> (GmaTxControl::RxSide),
               MakeEnumAccessor<GmaTxAlgorithm> (&GmaTxControl::m_algorithm),
               MakeEnumChecker (GmaTxControl::RxSide, "traffic splitting control based on infomation only from RX side (TSU)",
                                GmaTxControl::FixedHighPriority, "traffic splitting control based on the high priority link. All traffic goes to this link."))
  ;
  return tid;
}


GmaTxControl::~GmaTxControl ()
{
	NS_LOG_FUNCTION (this);
}

void
GmaTxControl::SetLinkState(Ptr<LinkState> linkstate)
{
	m_linkState = linkstate;
}

void
GmaTxControl::SetAlgorithm (std::string algorithm)
{
	if (algorithm == "RxSide")
	{
		m_algorithm = GmaTxControl::RxSide;
	}
	else if (algorithm == "FixedHighPriority")
	{
		m_algorithm = GmaTxControl::FixedHighPriority;
	} 
	else
	{
		NS_FATAL_ERROR ("algorithm not supported");
	}
}



uint8_t
GmaTxControl::GetDeliveryLinkCid ()
{	
	//this function is called whenever a packet enters GMA.
	uint8_t cid= 0;
	if (m_algorithm == GmaTxControl::RxSide)
	{
		cid = RxSideAlgorithm();
	}
	else if (m_algorithm == GmaTxControl::FixedHighPriority)
	{
		cid = m_linkState->GetHighPriorityLinkCid();
	}
	else
	{
		NS_FATAL_ERROR("unknown algorithm");
	}
	return cid;
}

std::vector<uint8_t>
GmaTxControl::GetDuplicateLinkCidList ()
{	
	return m_cidDupVector;
}

uint8_t
GmaTxControl::GetNextCidFromTsu ()
{
	if(m_cidVector.size() == 0)
	{
		// no tsu message received yet. return MAX
		return UINT8_MAX;
	}
	else
	{
		//use reverse bit index to randomize the splitting process.
		uint8_t connectionId = m_cidVector.at(m_splittingIndex);

		m_splittingIndex++;
		if(m_splittingIndex >= m_paramL)
		{
			m_splittingIndex = 0;
		}
		return connectionId;
	}
	
}

uint8_t
GmaTxControl::GetSplittingBurstFromTsu ()
{
	return m_paramL;
}

uint8_t
GmaTxControl::RxSideAlgorithm ()
{
	uint8_t cidValue = m_linkState->GetLowPriorityLinkCid();
	if (m_qosSteerEnabled)
	{
		cidValue = m_linkState->GetHighPriorityLinkCid();
	}

	if(m_cidVector.size() == 0)
	{
		// no tsu message received yet. return default link cid
	}
	else
	{
		//todo if a link is down, we need allocate the traffic to other links.
		//if the traffic is already splitting, we skip the link that is down!!

		//use reverse bit index to randomize the splitting process.
		uint8_t connectionId = m_cidVector.at(m_splittingIndex);

		m_splittingIndex++;
		if(m_splittingIndex >= m_paramL)
		{
			m_splittingIndex = 0;
		}

		if(m_linkState->IsLinkUp(connectionId))
		{
			cidValue = connectionId;
		}
		//else this link is down, use the fault link, this is easy to implement...
		
	}
	return cidValue;
	
}

void
GmaTxControl::RcvCtrlMsg (const MxControlHeader& mxHeader)
{
	if (mxHeader.GetType () == 5)//tsu msg
	{
		NS_ASSERT_MSG(mxHeader.GetL() == 1 || mxHeader.GetL() == 4 || mxHeader.GetL() == 8 || mxHeader.GetL() == 16 || mxHeader.GetL() == 32 || mxHeader.GetL() == 128,
			"Now only support size 1, 4, 8 16, 32 and 128.");
		/*test random shuffle*/
		std::vector<uint8_t> splitIndexList = mxHeader.GetKVector();
		m_paramL = mxHeader.GetL();

		std::vector<uint8_t> inorderCidVector;
		std::vector<uint8_t> cidList = m_linkState->GetCidList();
		for (uint8_t link = 0; link < splitIndexList.size(); link++)
		{
			for(uint8_t ind = 0; ind < splitIndexList.at(link); ind++)
			{
				inorderCidVector.push_back(cidList.at(link));
			}
		}

		//Only steer mode implemented duplicating packets
		if(m_paramL != 1 && inorderCidVector.size() != m_paramL)//split mode and packet duplication
		{
			NS_FATAL_ERROR("Did not implemented duplicating packet for split mode...");
		}

		if (m_paramL != 1)//split mode
		{
			//we will reconfigure it later for split mode.
			m_cidVector.clear();
		}
		else
		{
			if(m_cidVector.size() == 0)
			{
				//no action..
			}
			else if (m_cidVector.size() == 1)
			{
				//if the new control msg includes the current link,
				//we keep the m_cidVector the same, and add other links into the m_cidDupcVector.
				//otherwise, we will clear the m_cidVector.

				bool linkChange = true;//check if the current link is in the new control msg
				for(uint8_t ind = 0; ind < inorderCidVector.size(); ind++)
				{
					if(m_cidVector.at(0) == inorderCidVector.at(ind))
					{
						//the new control msg includes the current link cid. Do not update the m_cideVector
						linkChange = false;
					}
				}

				//we will reconfigure it later for the steer mode.
				if(linkChange)
				{
					m_cidVector.clear();
				}
			}
			else
			{
				NS_FATAL_ERROR("duplicate mode should not have more than 1 primary link");
			}
			m_cidDupVector.clear();
		}

		//apply reverse index;
		for (uint8_t index = 0; index<inorderCidVector.size(); index++)
		{
			if(m_paramL == 4)
			{
				m_cidVector.push_back(inorderCidVector.at(REVERSE_BITS_INDEX4[index]));
			}
			else if(m_paramL == 8)
			{
				m_cidVector.push_back(inorderCidVector.at(REVERSE_BITS_INDEX8[index]));
			}
			else if(m_paramL == 16)
			{
				m_cidVector.push_back(inorderCidVector.at(REVERSE_BITS_INDEX16[index]));
			}
			else if(m_paramL == 32)
			{
				m_cidVector.push_back(inorderCidVector.at(REVERSE_BITS_INDEX32[index]));
			}
			else if(m_paramL == 64)
			{
				m_cidVector.push_back(inorderCidVector.at(REVERSE_BITS_INDEX64[index]));
			}
			else if(m_paramL == 128)
			{
				m_cidVector.push_back(inorderCidVector.at(REVERSE_BITS_INDEX128[index]));
			}
			else if(m_paramL == 1)//steer mode
			{
				if(m_cidVector.size() == 0)
				{
					//the new control msg does not include the old link, insert a new link
					m_cidVector.push_back(inorderCidVector.at(index));
				}
				else
				{
					//we already have primary link, add to the duplicated links.
					if(inorderCidVector.at(index) != m_cidVector.at(0))
					{
						m_cidDupVector.push_back(inorderCidVector.at(index));
					}
				}
				
			}
			else
			{
				NS_FATAL_ERROR("other size not implemented");
			}
		}
		/*std::cout << "transmit order:";
		for(uint8_t ind = 0; ind < m_cidVector.size(); ind++)
		{
			std::cout << +m_cidVector.at(ind) << " ";
		}
		std::cout << "\n";

		std::cout << "transmit (dup) order:";
		for(uint8_t ind = 0; ind < m_cidDupVector.size(); ind++)
		{
			std::cout << +m_cidDupVector.at(ind) << " ";
		}
		std::cout << "\n";*/

		if(m_paramL != 1)//split mode
		{
			NS_ASSERT_MSG(m_cidVector.size() == m_paramL, "the sum of cid vector should equal L");
		}

		m_splittingIndex = 0;
		//end test
		/*std::ostringstream fileName;
		fileName <<"rx-ratio.txt";
		std::ofstream myfile;
		myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
		myfile << Simulator::Now ().GetSeconds () <<" "<< (int)m_rxRatio << " "<< (int)m_rxSplittingBurst <<  "\n";
		myfile.close();*/
	}
	else
	{
		NS_FATAL_ERROR("Only implmented type 5, ratio change message");
	}
	//std::cout << m_portNumber <<" "<< Now().GetMilliSeconds() << " Tx receives: " << mxHeader <<"\n";
}

void
GmaTxControl::StartQosTesting(uint8_t cid, uint8_t testDuration)
{
	return;
}


void
GmaTxControl::StopQosTesting (uint8_t cid)
{
    return;
}


bool
GmaTxControl::QoSTestingRequestCheck (uint8_t cid)
{

  return false;

}


void
GmaTxControl::SetQosSteerEnabled (bool flag)
{
	m_qosSteerEnabled = flag;
}

bool
GmaTxControl::QosSteerEnabled ()
{
	return m_qosSteerEnabled;
}

}
