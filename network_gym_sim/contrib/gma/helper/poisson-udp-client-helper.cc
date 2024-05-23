/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/
/*
 * Copyright (c) 2008 INRIA
 *
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 */
#include "poisson-udp-client-helper.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

PoissonUdpClientHelper::PoissonUdpClientHelper ()
{
  m_factory.SetTypeId (PoissonUdpClient::GetTypeId ());
}

PoissonUdpClientHelper::PoissonUdpClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (PoissonUdpClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

PoissonUdpClientHelper::PoissonUdpClientHelper (Address address)
{
  m_factory.SetTypeId (PoissonUdpClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void
PoissonUdpClientHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
PoissonUdpClientHelper::Install (NodeContainer c)
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node = *i;
      Ptr<PoissonUdpClient> client = m_factory.Create<PoissonUdpClient> ();
      node->AddApplication (client);
      apps.Add (client);
    }
  return apps;
}

} // namespace ns3