#include "DefaultAllocatorPolicyTest.h"
#include "core/DefaultAllocatorPolicy.h"

DefaultAllocatorPolicyTest::DefaultAllocatorPolicyTest()
  : singleTracker("single"),
    chainTracker1("chainA"),
    chainTracker2("chainB"),
    chainTracker3("chainC"),
    context100(0, 100, false),
    context200(0, 200, false),
    context300(0, 300, false),
    context1000(0, 1000, false),
    NULL_REQUEST(NULL) {

  // Construct a graph for a single tracker.
  singleTrackerSet.addTracker(&singleTracker);

  // Construct a graph for a chain of 3 trackers.
  chainTracker1.addDownstreamTracker(&chainTracker2);
  chainTracker2.addDownstreamTracker(&chainTracker3);
  chainTrackerSet.addTracker(&chainTracker1);
  chainTrackerSet.addTracker(&chainTracker2);
  chainTrackerSet.addTracker(&chainTracker3);
}

void DefaultAllocatorPolicyTest::testSingleAllocation() {
  DefaultAllocatorPolicy* policy = new DefaultAllocatorPolicy(singleTrackerSet);

  // Create a request for 100 bytes of memory.
  MemoryAllocatorPolicy::Request request(context100, 100);

  // Nothing should be schedulable right now, even with 1000 bytes.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Should not be able to schedule anything.",
                               NULL_REQUEST,
                               policy->nextSchedulableRequest(1000));

  // Try to add the request to the policy.
  CPPUNIT_ASSERT_NO_THROW(policy->addRequest(request, "single"));

  // After adding, this request should be schedulable with sufficient memory
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Should be able to schedule the request.",
                               &request, policy->nextSchedulableRequest(1000));

  // Make sure we can schedule this request with 100 bytes of memory available.
  CPPUNIT_ASSERT_MESSAGE("Should be able to allocate 100 bytes of memory",
                         policy->canScheduleRequest(100, request));

  // Try to remove the request.
  CPPUNIT_ASSERT_NO_THROW(policy->removeRequest(request, "single"));

  // Nothing should be schedulable after removing the request.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Should not be able to schedule anything after "
                               "removing the request.", NULL_REQUEST,
                               policy->nextSchedulableRequest(1000));

  delete policy;
}

void DefaultAllocatorPolicyTest::testBlockWhenOutOfMemory() {
  DefaultAllocatorPolicy* policy = new DefaultAllocatorPolicy(singleTrackerSet);

  // Create 2 requests for 100 bytes of memory.
 MemoryAllocatorPolicy::Request request1(context100, 100);
 MemoryAllocatorPolicy::Request request2(context100, 100);

  // Add both requests.
  CPPUNIT_ASSERT_NO_THROW(policy->addRequest(request1, "single"));
  CPPUNIT_ASSERT_NO_THROW(policy->addRequest(request2, "single"));

  // Given 150 bytes of memory, the first request should be schedulable.
  uint64_t memory = 150;
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Next schedulable request should be request1.",
                               &request1,
                               policy->nextSchedulableRequest(memory));
  CPPUNIT_ASSERT_MESSAGE("Should be able to schedule request1.",
                         policy->canScheduleRequest(memory, request1));

  // Schedule request 1.
  CPPUNIT_ASSERT_NO_THROW(policy->removeRequest(request1, "single"));
  memory -= request1.size;

  // Should NOT be able to schedule request2 with only 50 bytes left.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Next schedulable request should be NULL.",
                               NULL_REQUEST,
                               policy->nextSchedulableRequest(memory));
  CPPUNIT_ASSERT_MESSAGE("Should NOT be able to schedule request2.",
                         !policy->canScheduleRequest(memory, request2));

  // Cleanup (remove request2 so destructor doesn't throw)
  CPPUNIT_ASSERT_NO_THROW(policy->removeRequest(request2, "single"));

  delete policy;
}

void DefaultAllocatorPolicyTest::testPriorityQueues() {
  DefaultAllocatorPolicy* policy = new DefaultAllocatorPolicy(chainTrackerSet);

  // Create a bunch of requests of various sizes
  MemoryAllocatorPolicy::Request request100_1(context100, 100);
  MemoryAllocatorPolicy::Request request100_2(context100, 100);
  MemoryAllocatorPolicy::Request request200(context200, 200);
  MemoryAllocatorPolicy::Request request300(context300, 300);
  MemoryAllocatorPolicy::Request request1000(context1000, 1000);

  // Chain has 3 trackers, A, B, C. Give trackers the following requests:
  // A: 200
  // B: 100_2, 300
  // C: 100_1, 1000
  CPPUNIT_ASSERT_NO_THROW(policy->addRequest(request100_1, "chainC"));
  CPPUNIT_ASSERT_NO_THROW(policy->addRequest(request100_2, "chainB"));
  CPPUNIT_ASSERT_NO_THROW(policy->addRequest(request200, "chainA"));
  CPPUNIT_ASSERT_NO_THROW(policy->addRequest(request300, "chainB"));
  CPPUNIT_ASSERT_NO_THROW(policy->addRequest(request1000, "chainC"));

  // Make sure we have enough memory to satisfy all requests.
  uint64_t memory = 100 * 2 + 200 + 300 + 1000;


  // The first request to be scheduled should be 100_1.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Next schedulable request should be 100_1.",
                               &request100_1,
                               policy->nextSchedulableRequest(memory));
  CPPUNIT_ASSERT_MESSAGE("Should be able to schedule request100_1.",
                         policy->canScheduleRequest(memory, request100_1));

  // Schedule 100_1.
  CPPUNIT_ASSERT_NO_THROW(policy->removeRequest(request100_1, "chainC"));
  memory -= request100_1.size;


  // The next request to be scheduled should be 1000.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Next schedulable request should be 1000.",
                               &request1000,
                               policy->nextSchedulableRequest(memory));
  CPPUNIT_ASSERT_MESSAGE("Should be able to schedule request1000.",
                         policy->canScheduleRequest(memory, request1000));

  // Schedule 1000.
  CPPUNIT_ASSERT_NO_THROW(policy->removeRequest(request1000, "chainC"));
  memory -= request1000.size;


  // The next request to be scheduled should be 100_2.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Next schedulable request should be 100_2.",
                               &request100_2,
                               policy->nextSchedulableRequest(memory));
  CPPUNIT_ASSERT_MESSAGE("Should be able to schedule request100_2.",
                         policy->canScheduleRequest(memory, request100_2));

  // Schedule 100_2.
  CPPUNIT_ASSERT_NO_THROW(policy->removeRequest(request100_2, "chainB"));
  memory -= request100_2.size;


  // The next request to be scheduled should be 300.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Next schedulable request should be 300.",
                               &request300,
                               policy->nextSchedulableRequest(memory));
  CPPUNIT_ASSERT_MESSAGE("Should be able to schedule request300.",
                         policy->canScheduleRequest(memory, request300));

  // Schedule 300.
  CPPUNIT_ASSERT_NO_THROW(policy->removeRequest(request300, "chainB"));
  memory -= request300.size;


  // The last request to be scheduled should be 200.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Next schedulable request should be 200.",
                               &request200,
                               policy->nextSchedulableRequest(memory));
  CPPUNIT_ASSERT_MESSAGE("Should be able to schedule request200.",
                         policy->canScheduleRequest(memory, request200));

  // Schedule 200.
  CPPUNIT_ASSERT_NO_THROW(policy->removeRequest(request200, "chainA"));
  memory -= request200.size;


  // We should be out of memory with nothing left to schedule.
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong amount of remaining memory.",
                               (uint64_t) 0, memory);
  CPPUNIT_ASSERT_EQUAL_MESSAGE("Next schedulable request should be NULL.",
                               NULL_REQUEST,
                               policy->nextSchedulableRequest(
                                 std::numeric_limits<uint64_t>::max()));

  delete policy;
}
