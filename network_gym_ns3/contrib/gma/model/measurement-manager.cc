/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "measurement-manager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MeasurementManager");

NS_OBJECT_ENSURE_REGISTERED (MeasurementManager);

//meausrement device

MeasureDevice::MeasureDevice ()
{
  NS_FATAL_ERROR ("should not use this constructor");
  NS_LOG_FUNCTION (this);
}

TypeId
MeasureDevice::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MeasureDevice")
    .SetParent<Object> ()
    .SetGroupName("Gma")
  ;
  return tid;
}

MeasureDevice::~MeasureDevice ()
{
	NS_LOG_FUNCTION (this);
}

MeasureDevice::MeasureDevice (uint8_t cid)
{
    m_cid = cid;
}

MeasureDevice::MeasureDevice (uint8_t cid, uint32_t owdTarget, uint32_t queueingDelayTargetMs)
{
    m_cid = cid;
	m_owdTargetMs = owdTarget;
	m_queueingDelayTargetMs = queueingDelayTargetMs;
}

uint8_t
MeasureDevice::GetCid ()
{
	return m_cid;
}

void
MeasureDevice::InitialMeasurementCycle ()
{
	m_lastIntervalOwd = UINT32_MAX; // MA_VALUE stands for unknown
	m_lastIntervalOwdDiff = 1.0;

    //the lost rate is measured during the entire measure cycle (not the last interval).
    m_numOfInOrderPacketsPerCycle = 0;
    m_numOfMissingPacketsPerCycle = 0;
    m_numOfAbnormalPacketsPerCycle = 0;
}

void
MeasureDevice::UpdateLastPacketOwd(uint32_t owdMs, bool dataFlag)
{
	//Measure one way delay
	//uint32_t currentTimeMs = Now().GetMilliSeconds();
	m_lastPacketOwd = owdMs;

	if (m_currentMinOwd > owdMs)
	{
		m_currentMinOwd = owdMs;
	}

	if (m_currentMaxOwd < owdMs)
	{
		m_currentMaxOwd = owdMs;
	}

	if(dataFlag)
	{
		m_numOfOwdPacketsPerInterval++;
		m_sumOwdPerInterval += m_lastPacketOwd;

		if (m_rtt + owdMs - m_owdSamePktOfRtt > m_owdTargetMs)
		{
			m_numOfHighOwdPacketsPerInterval++;
		}
		m_numOfDataPacketsPerInterval++;
		uint32_t minOwd = std::min(m_lastMinOwd, m_currentMinOwd);//we take the min of current and last interval.
		if(minOwd != UINT32_MAX)//only computes the delay violation from data.
		{
			//here we need both min owd offset and the sender owd adjustment (notified in tsa) to start the first delay violation measurement!!!!!!
			if (owdMs > minOwd + m_queueingDelayTargetMs)
			{
				m_numOfDelayViolationDataPacketsPerInterval++;
			}
		}
	}

	//std::cout << "------------ rtt:" << m_rtt
	//<< " owd:" << owdMs 
	//<< " m_owdSamePktOfRtt:" << m_owdSamePktOfRtt 
	//<<" output:" << m_rtt + owdMs - m_owdSamePktOfRtt  << std::endl; 
}


void
MeasureDevice::UpdateRtt (uint32_t rtt, uint32_t owd)
{
	//we now have two type of RTT measurement.
	// - (1) single probe or other control msg, this is triggered on demand
	// - (2) a burst of probe message, this is triggered periodically
	// For type (1), we update current rtt if the new rtt is smaller than current one.
	// For type (2), we update current rtt to the min of (new rtt burst).

	m_rtt = rtt;
	m_owdSamePktOfRtt = owd;
	//std::cout << " now: " << Now().GetSeconds() << " last update: " << m_lastRttUpdateTime.GetSeconds() << "------------ m_rtt:" << m_rtt
	//<< " m_owdSamePktOfRtt:" << m_owdSamePktOfRtt << " cid: " << LinkState::ConvertCidFormat(m_cid)
	//<< std::endl; 
	m_lastRttUpdateTime = Now();
}

uint32_t
MeasureDevice::GetRtt ()
{
	//rtt = rtt measured from control + (average owd measured from data - owd measured from control)
	if (m_lastIntervalOwd!= UINT32_MAX)//last interval OWD available
	{
		if (m_rtt + m_lastIntervalOwd < m_owdSamePktOfRtt)
		{
			NS_FATAL_ERROR("the new rtt value should never the smaller than 0!");
		}
		return m_rtt + m_lastIntervalOwd - m_owdSamePktOfRtt;
	}
	//else no owd measurement from data.
	else
	{

		return m_rtt;
	}
}


void
MeasureDevice::UpdateLsn(uint8_t lastLsn)
{
	if(m_numOfInOrderPacketsPerCycle + m_numOfAbnormalPacketsPerCycle == 0)
	{
		//first packet always assume in order
		m_numOfInOrderPacketsPerCycle++;
		m_lastLsn = lastLsn;
	}
	else
	{
		//determing in order or not
		if(LsnDiff(lastLsn, m_lastLsn) == 1)
		{
			// in order packets
			m_numOfInOrderPacketsPerCycle++;
			m_lastLsn = lastLsn;
		}
		else if(LsnDiff(lastLsn, m_lastLsn) > 1)
		{
			// detect a gap: received Lsn larger than expected value.
			m_numOfMissingPacketsPerCycle = m_numOfMissingPacketsPerCycle + LsnDiff(lastLsn, m_lastLsn)-1;
			m_numOfInOrderPacketsPerCycle++;
			m_lastLsn = lastLsn;
		}
		else 
		{
			//abnormal packets
			m_numOfAbnormalPacketsPerCycle++;
		}
	}
}

int
MeasureDevice::LsnDiff(int x1, int x2)
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

//meausrement manager

MeasurementManager::MeasurementManager ()
{
	NS_LOG_FUNCTION (this);
	m_sendTsuCallback = MakeNullCallback<void, Ptr<SplittingDecision> > ();
	m_delayMeasurementEvent.Cancel();
}

TypeId
MeasurementManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MeasurementManager")
    .SetParent<Object> ()
    .SetGroupName("Gma")
	.AddConstructor<MeasurementManager> ()
    .AddAttribute ("MaxMeasureInterval",
			"the max number of intervals per measurement cycle",
			UintegerValue (10),
			MakeUintegerAccessor (&MeasurementManager::m_maxMeasureInterval),
			MakeUintegerChecker<uint8_t> ())
	.AddAttribute ("EnableSenderSideOwdAdjustment",
			"Equalize the propagation delay of all links, always enable it for delay-based congestion control, such as BBR",
			BooleanValue (true),
			MakeBooleanAccessor (&MeasurementManager::m_senderSideOwdAdjustment),
			MakeBooleanChecker ())
  ;
  return tid;
}

MeasurementManager::~MeasurementManager ()
{
	NS_LOG_FUNCTION (this);
}

void
MeasurementManager::AddDevice (uint8_t cid)
{
	Ptr<MeasureDevice> device = CreateObject<MeasureDevice> (cid);
	m_deviceList.push_back(device);
}

void
MeasurementManager::AddDevice (uint8_t cid, uint32_t owdTarget)
{
	Ptr<MeasureDevice> device = CreateObject<MeasureDevice> (cid, owdTarget, m_rxControl->GetQueueingDelayTargetMs());
	m_deviceList.push_back(device);
}

Ptr<MeasureDevice>
MeasurementManager::GetDevice(uint8_t cid)
{
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if (m_deviceList.at(index)->GetCid() == cid)
		{
			return m_deviceList.at(index);
		}
	}
	NS_FATAL_ERROR("cannot find the device!!!!!");
	return Ptr<MeasureDevice>();
}

bool
MeasurementManager::IsMeasurementOn()
{
	return m_measurementOn;
}

void
MeasurementManager::DisableMeasurement()
{
	m_measurementOn = false;
}

void
MeasurementManager::DataMeasurementSample(uint32_t owdMs, uint8_t lsn, uint8_t cid)
{
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if (m_deviceList.at(index)->GetCid() == cid)
		{
			m_deviceList.at(index)->UpdateLastPacketOwd(owdMs, true);
			m_deviceList.at(index)->UpdateLsn(lsn);
			break;
		}
	}
}

void
MeasurementManager::UpdateRtt(uint32_t rtt, uint32_t owd, uint8_t cid)
{
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if (m_deviceList.at(index)->GetCid() == cid)
		{
			m_deviceList.at(index)->UpdateRtt(rtt, owd);
			break;
		}
	}
}

uint32_t
MeasurementManager::GetMaxRttMs()
{
	uint32_t maxRtt = 0;
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if(maxRtt < m_deviceList.at(index)->GetRtt())
		{
			maxRtt = m_deviceList.at(index)->GetRtt();
		}
	}
	//value is 0 if no measurement is available.
	return maxRtt;
}


uint32_t
MeasurementManager::GetMaxOwdMs()
{
	uint32_t maxOwd = 0;
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if(maxOwd < m_deviceList.at(index)->m_lastMaxOwd)
		{
			maxOwd = m_deviceList.at(index)->m_lastMaxOwd;
		}
	}
	//value is 0 if no measurement is available.
	return maxOwd;
}

uint32_t
MeasurementManager::GetMinOwdMs()
{
	uint32_t minOwd = UINT32_MAX;
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if(minOwd > m_deviceList.at(index)->m_lastMinOwd)
		{
			minOwd = m_deviceList.at(index)->m_lastMinOwd;
		}
	}
	//value is UINT32_MAX if no measurement is available.
	return minOwd;
}

void
MeasurementManager::UpdateOwdFromProbe(uint32_t owdMs, uint8_t cid)
{
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if (m_deviceList.at(index)->GetCid() == cid)
		{
			m_deviceList.at(index)->UpdateLastPacketOwd(owdMs, false);
			break;
		}
	}
}

void
MeasurementManager::UpdateOwdFromAck(uint32_t owdMs, uint8_t cid)
{
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if (m_deviceList.at(index)->GetCid() == cid)
		{
			m_deviceList.at(index)->UpdateLastPacketOwd(owdMs, false);
			break;
		}
	}
}

void
MeasurementManager::UpdateAllMinOwd()
{
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		m_deviceList.at(index)->m_lastMinOwd = m_deviceList.at(index)->m_currentMinOwd;
		m_deviceList.at(index)->m_currentMinOwd = UINT32_MAX;
		std::cout << " cid:" << +m_deviceList.at(index)->m_cid << " min owd:" << m_deviceList.at(index)->m_lastMinOwd << " ";
	}
	std::cout << "\n";

	//we also update max owd here.
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		m_deviceList.at(index)->m_lastMaxOwd = m_deviceList.at(index)->m_currentMaxOwd;
		m_deviceList.at(index)->m_currentMaxOwd = 0;
		std::cout << " cid:" << +m_deviceList.at(index)->m_cid << " max owd:" << m_deviceList.at(index)->m_lastMaxOwd << " ";
	}
	
	m_lastMinOwdUpdateTime = Now();

}

void
MeasurementManager::UpdateReducedMinOwd()
{
	//spacial case, if min owd reduces dramatically compared to preivous interval, e.g., 10 ms.. update immediately

	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if (m_deviceList.at(index)->m_lastMinOwd != UINT32_MAX && m_deviceList.at(index)->m_currentMinOwd != UINT32_MAX)
		{
			//last and current owd available.
			uint32_t threshold = 10;
			if(m_deviceList.at(index)->m_lastMinOwd > threshold && m_deviceList.at(index)->m_currentMinOwd < m_deviceList.at(index)->m_lastMinOwd - threshold)
			{
				//last_min_owd is grater than 5ms and current_min_owd < last_min_owd - threshol
				m_deviceList.at(index)->m_lastMinOwd = m_deviceList.at(index)->m_currentMinOwd;
				m_lastMinOwdUpdateTime = Now();//we will send a tsu
				//we will reset the currentMinOwd when the tsu is send...
			}
		}
	}

}

void
MeasurementManager::UpdateMinOwdCheck()
{
	if(m_lastMinOwdUpdateTime == Seconds(0))
	{
		//first measurement, always update
		std::cout << "+++++++++ MIN OWD UPDATE | first update";
		UpdateAllMinOwd();
	}
	else if( Now() > m_lastMinOwdUpdateTime + OWD_OFFSET_UPDATE_INTERVAL)
	{
		//more than update interval, update now.
		std::cout << "+++++++++ MIN OWD UPDATE | every OWD_OFFSET_UPDATE_INTERVAL";
		UpdateAllMinOwd();
	}
	else
	{
		//do not update all link.
		//UpdateReducedMinOwd();//check if any link's owd is reduced. if so, update only that link imdediately.
	}
		
}


void
MeasurementManager::MeasureCycleStart (uint32_t measureStartSn)
{
    //std::cout << Now().GetSeconds() <<"cycle start>>>>>>>>>>\n";

    m_measureIntervalIndex = 0;
    m_measureStartSn = measureStartSn;
    m_measurementOn = true;
    m_measureIntervalStarted = false;
	m_delayMeasurement = 0;
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		m_deviceList.at(index)->InitialMeasurementCycle();
	}
}

void
MeasurementManager::MeasureCycleStartByTsa(uint32_t measureStartSn, uint8_t delay, std::vector<uint8_t> owdVector)
{
    //std::cout << Now().GetSeconds() <<"cycle start>>>>>>>>>>\n";

    m_measureIntervalIndex = 0;
    m_measureStartSn = measureStartSn;
    m_measurementOn = true;
    m_measureIntervalStarted = false;
	m_delayMeasurement = delay;
    for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		m_deviceList.at(index)->InitialMeasurementCycle();
	}
	if (owdVector.size() > 0)//received sender delay adjustment from tsa msg.
	{
		NS_ASSERT_MSG(owdVector.size() != m_deviceList.size(), "the size of the owd vector and link is not the same!");
		for (uint8_t index = 0; index < m_deviceList.size(); index++)
		{
			m_deviceList.at(index)->m_senderOwdAdjustment = (int)owdVector.at(index)-127;
		}
	}
}

void MeasurementManager::MeasureIntervalStartCheck(Time t, uint32_t sn, uint32_t expectedSn)
{
	if(sn < expectedSn)
	{
		//not in-order packet, do nothing.
		return;
	}
	//in-order pkt
	m_lastIntervalStartSn = sn;
	if(SnDiff(sn, m_measureStartSn) > 0 && m_measureIntervalStarted == false)
	{
		//we assume the sender owd adjustment takes effects afer receives the packet after receives tsa.

		if(m_delayMeasurementEvent.IsRunning())
		{
			//no action, we are delaying the measurement.
		}
		else
		{
			if (m_delayMeasurement == 0)
			{
				MeasureIntervalStart(t);
			}
			else
			{
				m_delayMeasurementEvent = Simulator::Schedule(MilliSeconds(m_delayMeasurement), &MeasurementManager::MeasureIntervalStart, this, t+MilliSeconds(m_delayMeasurement));
				m_delayMeasurement = 0;
			}
		}
	}
}


void 
MeasurementManager::MeasureIntervalStart(Time t)
{

	//add the min owd adjustment to the measurement.
	//std::cout << "[offset, min_owd, new min_owd] ";
	for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		if (m_deviceList.at(index)->m_senderOwdAdjustment > 0)
		{
			int newDelay = (int) m_deviceList.at(index)->m_lastMinOwd  + m_deviceList.at(index)->m_senderOwdAdjustment;
			//std::cout << " [" << m_deviceList.at(index)->m_senderOwdAdjustment << ", ";
			//std::cout << m_deviceList.at(index)->m_lastMinOwd << ", ";

			if (m_deviceList.at(index)->m_lastMinOwd == UINT32_MAX)
			{
				//last owd measurement not available
				NS_ASSERT_MSG(m_deviceList.at(index)->m_senderOwdAdjustment==0, "shoule not adjust a link's owd without measurement!");
			}
			else
			{ 
				//last owd available
				if(newDelay < 0)
				{
					NS_FATAL_ERROR("new delay after offset cannot be zero");
				}

				//m_lastMinOwd and m_previousOwdValue needs to be updated at the same time.
				m_deviceList.at(index)->m_lastMinOwd = newDelay;//update last owd to new owd after adjustment
				if (m_previousOwdValue.size() > 0)
				{
					NS_ASSERT_MSG(m_previousOwdValue.size() == m_deviceList.size(), "the size of previous owd and measurement device list is different!");
					m_previousOwdValue.at(index) = newDelay;//also update the owd send in the last tsu msg. we use this check to resend tsu.
				}
				m_deviceList.at(index)->m_currentMinOwd = UINT32_MAX;//reset current min owd measurement.
				m_deviceList.at(index)->m_currentMaxOwd = 0;//reset current max owd measurement.
				m_deviceList.at(index)->m_senderOwdAdjustment = 0;//reset sender offset.
			}

			if (m_deviceList.at(index)->m_lastMaxOwd != 0)
			{
				m_deviceList.at(index)->m_lastMaxOwd = std::max(0, (int)m_deviceList.at(index)->m_senderOwdAdjustment + (int)m_deviceList.at(index)->m_lastMaxOwd);
			}
			//std::cout << m_deviceList.at(index)->m_lastMinOwd << "] ";

		}
		//std::cout << std::endl;
	}

    //initialize measurement parameters
    for (uint8_t index = 0; index < m_deviceList.size(); index++)
	{
		m_deviceList.at(index)->m_sumOwdPerInterval = 0;
		m_deviceList.at(index)->m_numOfOwdPacketsPerInterval = 0;
		m_deviceList.at(index)->m_numOfHighOwdPacketsPerInterval = 0;
		m_deviceList.at(index)->m_numOfDelayViolationDataPacketsPerInterval = 0;
		m_deviceList.at(index)->m_numOfDataPacketsPerInterval = 0;
	}
    m_measureIntervalIndex++;
    m_measureIntervalStarted = true;
    m_measureIntervalStartTime = t;
	m_splittingBurstRequirementEst = 0;
    m_measureIntervalThresh = MilliSeconds(GetMaxRttMs());
    m_measureIntervalThresh = std::max(MIN_INTERVAL_DURATION, std::min(MAX_INTERVAL_DURATION, m_measureIntervalThresh));
    //std::cout << Now().GetSeconds() <<" interval " << +m_measureIntervalIndex  << " start \n";
}

void
MeasurementManager::MeasureIntervalEndCheck(Time t)
{
	if(m_measureIntervalStarted)
	{
		uint32_t totalPackets = 0;
		for (uint8_t index = 0; index < m_deviceList.size(); index++)
		{
			totalPackets+= m_deviceList.at(index)->m_numOfDataPacketsPerInterval;
		}

		if (m_splittingBurstRequirementEst == 0 && t >= (m_measureIntervalStartTime + m_measureIntervalThresh))
		{
			//measure the number of received packets after m_measureIntervalThresh
			m_splittingBurstRequirementEst = totalPackets;
		}

		if(t > m_measureIntervalStartTime + MAX_INTERVAL_DURATION)
		{
			//interval more than max duration, check how many packets are received.
			if((int)totalPackets > m_rxControl->GetMeasurementBurstRequirement())
			{
				MeasureIntervalEnd(t);
			}
			else
			{
				//in the andorid app, if no packet is received in both links, move all traffic to Wi-Fi.
				//I am not doing that here.
				MeasureIntervalEnd(t);
			}
		}
		else
		{
			if((int)totalPackets > m_rxControl->GetMeasurementBurstRequirement() && t >= (m_measureIntervalStartTime + m_measureIntervalThresh))
			{
				//after one interval duration, we received enough packets.
				MeasureIntervalEnd(t);
			}
		}
	}
}

//menglei: a measurement interval end, collect measurement results
void
MeasurementManager::MeasureIntervalEnd (Time t)
{
    double maxDiff = 0;//max diff for all devices
    m_measureIntervalStarted = false;
    for (uint8_t index = 0; index < m_deviceList.size(); index++)
    {
    	if(m_deviceList.at(index)->m_numOfOwdPacketsPerInterval > 0)//received packets from this device(lte/wifi)
    	{
    		//compute ave owd
    		uint32_t aveOwd = m_deviceList.at(index)->m_sumOwdPerInterval/m_deviceList.at(index)->m_numOfOwdPacketsPerInterval;
    		if (m_deviceList.at(index)->m_lastIntervalOwd!= UINT32_MAX)//last interval OWD != 0
    		{
    			uint32_t rtt = m_deviceList.at(index)->GetRtt();
    			if(rtt == 0)//cannot divide by zero.
    			{
    				rtt = 1;
    			}
    			//compute OwdDiff for termination condition
            	m_deviceList.at(index)->m_lastIntervalOwdDiff =
            					 std::abs(((double)(aveOwd - m_deviceList.at(index)->m_lastIntervalOwd)) / rtt);
            }
            //store the owd for next interval computation
            m_deviceList.at(index)->m_lastIntervalOwd = aveOwd;
    	}

    	//get the max diff of Owd for all devices. It equals 0 if no packet is received from this link.
    	//When interval end, at least one device should receive packets.
   	 	if(maxDiff < m_deviceList.at(index)->m_lastIntervalOwdDiff)
   	 	{
   	 		maxDiff = m_deviceList.at(index)->m_lastIntervalOwdDiff;
   	 	}
    }

    bool endMeasurement = false;

    //end measurement cycle condition 1: if all device's measurements converges.
    if(m_measureIntervalIndex >= 2)
    {//at least measure 2 intervals
        if( maxDiff < OWD_CONVERGE_THRESHOLD){
            endMeasurement = true;
        }
    }

    //end measurement cycle condition 2: if both measurement cannot converge, but the number of intervals meets m_maxMeasureInterval
    if(m_measureIntervalIndex >= m_maxMeasureInterval) 
    {
        endMeasurement = true;
    }

    if(endMeasurement)
    {
        m_measurementOn = false; // this will end a measurement cycle
		std::vector<bool> rxPktThisInterval;
        for (uint8_t index = 0; index < m_deviceList.size(); index++)
        {
        	//for devices that do not receive any packets in this measure cycle, we set its owd equals to the last received packet
        	if (m_deviceList.at(index)->m_lastIntervalOwd == UINT32_MAX)
        	{
        		m_deviceList.at(index)->m_lastIntervalOwd = m_deviceList.at(index)->m_lastPacketOwd;
				rxPktThisInterval.push_back(false);
        	}
			else
			{
				rxPktThisInterval.push_back(true);
			}
        }

		UpdateMinOwdCheck(); //check if we need to update min owd.

		Ptr<RxMeasurement> rxMeasurement = Create <RxMeasurement> ();
		rxMeasurement->m_links = m_deviceList.size();
		rxMeasurement->m_measureIntervalThreshS = m_measureIntervalThresh.GetSeconds();
		rxMeasurement->m_measureIntervalDurationS = (t - m_measureIntervalStartTime).GetSeconds(); //we cannot use m_measureIntervalThresh directly, since we also have requirement for packet number! so the duration may be larger than m_measureIntervalThresh
		rxMeasurement->m_delayThisInterval = rxPktThisInterval;
		rxMeasurement->m_splittingBurstRequirementEst = m_splittingBurstRequirementEst;
		for (uint8_t index = 0; index < m_deviceList.size(); index++)
		{
			rxMeasurement->m_cidList.push_back(m_deviceList.at(index)->GetCid());
			if (!m_senderSideOwdAdjustment)
			{
				//enable equalize delay at the receiver side, we will normalize the delay measurement: delay_normal = delay - min_owd
				uint32_t queuingDelay = 0;
				if (m_deviceList.at(index)->m_lastMinOwd == UINT32_MAX)
				{
					//no measurement.
					queuingDelay = m_deviceList.at(index)->m_lastIntervalOwd;
				}
				else if(m_deviceList.at(index)->m_lastIntervalOwd > m_deviceList.at(index)->m_lastMinOwd)
				{
					queuingDelay = m_deviceList.at(index)->m_lastIntervalOwd - m_deviceList.at(index)->m_lastMinOwd;
				}
				//else current owd is small than last min_owd -> no queueing delay
				rxMeasurement->m_delayList.push_back(queuingDelay);
			}
			else
			{
				rxMeasurement->m_delayList.push_back(m_deviceList.at(index)->m_lastIntervalOwd);//report raw delay, sender will add delay
			};

			rxMeasurement->m_minOwdLongTerm.push_back(m_deviceList.at(index)->m_lastMinOwd);
			
			double lossRate = 0;
          	if (m_deviceList.at(index)->m_numOfMissingPacketsPerCycle > m_deviceList.at(index)->m_numOfAbnormalPacketsPerCycle)
			{
				lossRate = 1.0*(m_deviceList.at(index)->m_numOfMissingPacketsPerCycle - m_deviceList.at(index)->m_numOfAbnormalPacketsPerCycle)
							/(m_deviceList.at(index)->m_numOfMissingPacketsPerCycle + m_deviceList.at(index)->m_numOfInOrderPacketsPerCycle);
			}
			/*std::cout<< "Cycle End in Interval:" << +m_measureIntervalIndex
					 << " cid:" << +m_deviceList.at(index)->m_cid 
		         << " diff:" << m_deviceList.at(index)->m_lastIntervalOwdDiff
		         << " ave OWD:" << m_deviceList.at(index)->m_lastIntervalOwd 
		         << " last OWD:" <<  m_deviceList.at(index)->m_lastPacketOwd
		         << " loss rate:" << lossRate 
		         << " inorder:" << m_deviceList.at(index)->m_numOfInOrderPacketsPerCycle 
		         << " missing:" << m_deviceList.at(index)->m_numOfMissingPacketsPerCycle
		         << " abnormal: " << m_deviceList.at(index)->m_numOfAbnormalPacketsPerCycle
		         << " RTT:" << +m_deviceList.at(index)->GetRtt() 
		         << " \n"; */
			rxMeasurement->m_lossRateList.push_back(lossRate);

			        //log file
			/*std::ostringstream fileName;
			fileName <<"measure-" << this <<".txt";
			std::ofstream myfile;
			myfile.open (fileName.str ().c_str (), std::ios::out | std::ios::app);
			myfile << Simulator::Now ().GetSeconds () << " " << +m_deviceList.at(index)->m_cid 
			<< " " <<m_deviceList.at(index)->m_lastIntervalOwd << " " << lossRate <<"\n";
			myfile.close();*/
			rxMeasurement->m_delayViolationPktNumList.push_back(m_deviceList.at(index)->m_numOfDelayViolationDataPacketsPerInterval);
			rxMeasurement->m_totalPktNumList.push_back(m_deviceList.at(index)->m_numOfDataPacketsPerInterval);

		}

        Ptr<SplittingDecision> decision = m_rxControl->GetTrafficSplittingDecision(rxMeasurement);

		//check if we need to append min owd measurement to the tsu. Only do this for splitting mode!
		if(m_rxControl->GetSplittingBurst() > 1 && m_senderSideOwdAdjustment) //splitting mode and sender side adjustment enabled.
		{
			if (m_previousOwdValue.empty())
			{
				//no feedback yet, we can send the first feedback once the min owd measurement is ready. e.g., after OWD_OFFSET_UPDATE_INTERVAL.
				//std::cout << " min_owd: ";
				uint32_t owdMax = 0;
				for (uint8_t index = 0; index < m_deviceList.size(); index++)
				{
					if (m_deviceList.at(index)->m_lastMinOwd != UINT32_MAX && owdMax < m_deviceList.at(index)->m_lastMinOwd)
					{
						owdMax = m_deviceList.at(index)->m_lastMinOwd;
					}
					//std::cout << m_deviceList.at(index)->m_lastMinOwd << " ";
				}
				//std::cout << std::endl;

				if (owdMax > 0)
				{
					// new measurement available, send max_owd - owd to transmitter.
					std::cout << " feedback max(min_owd) - min_owd: ";
					decision->m_update=true;
					for (uint8_t index = 0; index < m_deviceList.size(); index++)
					{	
						m_previousOwdValue.push_back(m_deviceList.at(index)->m_lastMinOwd);
						//cap the owd measurement to 0~254 since we only have 1 byte.
						if(m_deviceList.at(index)->m_lastMinOwd == UINT32_MAX)
						{
							//no measurement.
							decision->m_minOwdLongTerm.push_back(UINT8_MAX);
						}
						else{
							uint32_t maxValue = UINT8_MAX - 1;
							decision->m_minOwdLongTerm.push_back(std::min(maxValue, owdMax - m_deviceList.at(index)->m_lastMinOwd));
						}
						std::cout << +decision->m_minOwdLongTerm.at(index) << " ";
						m_deviceList.at(index)->m_currentMinOwd = UINT32_MAX;//reset the measurement everytime tsu is sent.
						m_deviceList.at(index)->m_currentMaxOwd = 0;//reset the measurement everytime tsu is sent.
					}
					std::cout << std::endl;
				}
			}
			else
			{
				//check if the current min owd and previou min owd value is the same.
				if (m_deviceList.size() != m_previousOwdValue.size())
				{
					NS_FATAL_ERROR("the size of the min owd measurement and last feedback is not the same!");
				}
				bool minOwdChanged = false;
				for (uint8_t index = 0; index < m_deviceList.size(); index++)
				{	
					//be careful that, when the sender side adjust the delay (notified by tsa), we need to update both the m_lastMinOwd and m_previousOwdValue to pass the check
					if (m_deviceList.at(index)->m_lastMinOwd != m_previousOwdValue.at(index))
					{
						std::cout << " --------------------------- min owd: " << m_deviceList.at(index)->m_lastMinOwd  << " previous update: " << +m_previousOwdValue.at(index) << std::endl;
						minOwdChanged = true;
						break;
					}
				}
				
				if (minOwdChanged)
				{
					uint32_t owdMax = 0;
					std::cout << " min_owd: ";
					for (uint8_t index = 0; index < m_deviceList.size(); index++)
					{
						if (m_deviceList.at(index)->m_lastMinOwd != UINT32_MAX && owdMax < m_deviceList.at(index)->m_lastMinOwd)
						{
							owdMax = m_deviceList.at(index)->m_lastMinOwd;
						}
						std::cout << m_deviceList.at(index)->m_lastMinOwd << " ";
					}
					std::cout << std::endl;

					if (owdMax > 0)
					{
						// new measurement available, send max_owd - owd to transmitter.
						std::cout << " feedback max(min_owd) - min_owd: ";
						decision->m_update=true;
						for (uint8_t index = 0; index < m_deviceList.size(); index++)
						{	
							m_previousOwdValue.at(index) = m_deviceList.at(index)->m_lastMinOwd;
							//cap the owd measurement to 0~127 since we only have 1 byte.
							if(m_deviceList.at(index)->m_lastMinOwd == UINT32_MAX)
							{
								//no measurement.
								decision->m_minOwdLongTerm.push_back(UINT8_MAX);
							}
							else{
								uint32_t maxValue = UINT8_MAX - 1;
								decision->m_minOwdLongTerm.push_back(std::min(maxValue, owdMax - m_deviceList.at(index)->m_lastMinOwd));
							}
							std::cout << +decision->m_minOwdLongTerm.at(index) << " ";
							m_deviceList.at(index)->m_currentMinOwd = UINT32_MAX;//reset the measurement everytime tsu is sent.
							m_deviceList.at(index)->m_currentMaxOwd = 0;//reset the measurement everytime tsu is sent.

						}
						std::cout << std::endl;
					}
				}
			}
		}

        //let us start a new cycle.
        if(decision->m_update)
        {
        	//send TSU message
        	m_sendTsuCallback(decision);// MeasureCycleStartByTsa will be triggered by a tsu msg.
			//if the TSU carries a delay adjustment info, we will wait for the TSA, and appy the send delay adjustment in the m_lastMinOwd measurement. So the tx and rx is synced.
        }
        else
        {
        	MeasureCycleStart(m_lastIntervalStartSn);//restart measurement cycle without tsu/tsa.
        }
    }
    else
    {
        //start a new measure interval
        MeasureIntervalStart(t);
    }
}

void
MeasurementManager::SetSendTsuCallback(Callback<void, Ptr<SplittingDecision> > cb)
{
	m_sendTsuCallback = cb;
}

void
MeasurementManager::SetRxControlApp(Ptr<GmaRxControl> control)
{
	m_rxControl = control;
}

int
MeasurementManager::SnDiff(int x1, int x2)
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


}


