
/* Copyright(C) 2024 Intel Corporation
*  SPDX-License-Identifier: GPL-2.0
*  https://spdx.org/licenses/GPL-2.0.html
*/


// Include a header file from your module to test.
// An essential include is test.h
#include "ns3/test.h"

// Do not put your test classes in namespace ns3.  You may find it useful
// to use the using directive to access the ns3 namespace directly
using namespace ns3;

// Add a doxygen group for tests.
// If you have more than one test, this should be in only one of them.
/**
 * \defgroup networkgym-tests Tests for networkgym
 * \ingroup networkgym
 * \ingroup tests
 */

// This is an example TestCase.
/**
 * \ingroup networkgym-tests
 * Test case for feature 1
 */
class NetworkgymTestCase1 : public TestCase
{
  public:
    NetworkgymTestCase1();
    virtual ~NetworkgymTestCase1();

  private:
    void DoRun() override;
};

// Add some help text to this case to describe what it is intended to test
NetworkgymTestCase1::NetworkgymTestCase1()
    : TestCase("Networkgym test case (does nothing)")
{
}

// This destructor does nothing but we include it as a reminder that
// the test case should clean up after itself
NetworkgymTestCase1::~NetworkgymTestCase1()
{
}

//
// This method is the pure virtual method from class TestCase that every
// TestCase must implement
//
void
NetworkgymTestCase1::DoRun()
{
    // A wide variety of test macros are available in src/core/test.h
    NS_TEST_ASSERT_MSG_EQ(true, true, "true doesn't equal true for some reason");
    // Use this one for floating point comparisons
    NS_TEST_ASSERT_MSG_EQ_TOL(0.01, 0.01, 0.001, "Numbers are not equal within tolerance");
}

// The TestSuite class names the TestSuite, identifies what type of TestSuite,
// and enables the TestCases to be run.  Typically, only the constructor for
// this class must be defined

/**
 * \ingroup networkgym-tests
 * TestSuite for module networkgym
 */
class NetworkgymTestSuite : public TestSuite
{
  public:
    NetworkgymTestSuite();
};

NetworkgymTestSuite::NetworkgymTestSuite()
    : TestSuite("networkgym", UNIT)
{
    // TestDuration for TestCase can be QUICK, EXTENSIVE or TAKES_FOREVER
    AddTestCase(new NetworkgymTestCase1, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
/**
 * \ingroup networkgym-tests
 * Static variable for test initialization
 */
static NetworkgymTestSuite snetworkgymTestSuite;
