/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef GMA_HELPER_H
#define GMA_HELPER_H

#include "ns3/gma-protocol.h"

namespace ns3 {

class GmaHelper : public Object
{
public:
  GmaHelper ();
	/*
  This function setup a tunnel between server and client. The server with physical address serverAddr, client with two phsical address clientAddr and clientAddrSec.
  The function will encapulate the packets forwarding to the virtual address 7.0.1.2, and route these packets over either clientAddr or clientAddrSec.
  TODO: split the sender and receiver functionality into 2 sub-classes.
  */
  virtual ~GmaHelper ();
  static TypeId GetTypeId (void);
  
private:

};

}

#endif /* GMA_HELPER_H */

