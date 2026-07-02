/*******************************************************************************
 * <unittest/lt/core/UnitTestLTCoreReleaseClientData.c>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, you can obtain one at
 * https://mozilla.org/MPL/2.0/.
 *
 * Copyright 2026, Roku, Inc.  All rights reserved.
 ******************************************************************************/

#include <lt/core/LTCore.h>
#include <tilt/JiltEngine.h>

/******************
  local #defines */
#define THREAD_NAME                             "REASONER"
#define THREAD_SPECIFIC_DATA_KEY                "REASON"
#define TiltHarness                             Tilt

/******************
  local typedefs */
typedef struct Reasoning {
      LTAtomic                  threadReleaseCount;
      LTAtomic                  timerReleaseCount;
      LTAtomic                  eventReleaseCount;
      LTAtomic                  queueTaskProcReleaseCount;
      LTThread_ReleaseReason    threadReleaseReason;
      LTThread_ReleaseReason    timerReleaseReason;
      LTThread_ReleaseReason    eventReleaseReason;
      LTThread_ReleaseReason    queueTaskProcReleaseReason;
      u32                       threadReleaseClientData;
      u32                       timerReleaseClientData;
      u32                       eventReleaseClientData;
      u32                       queueTaskProcReleaseClientData;

} Reasoning;

/********************
  static constants */
static const LTArgsDescriptor s_eventArgs = {1, { kLTArgType_u32 }};

/********************
  static variables */
static Reasoning        s_reasoning;

/********************
  static functions */
static void ResetReasoning(void) {
    lt_memset(&s_reasoning, 0, sizeof(s_reasoning));
}

// The following 4 functions are noops for now
static void MyEventDispatchProc(LTEvent hEvent, void *eventProc, LTArgs *eventArgs, void *eventProcClientData)                              { LT_UNUSED(hEvent); LT_UNUSED(eventProc); LT_UNUSED(eventArgs); LT_UNUSED(eventProcClientData); }
static void MyEventDispatchCompleteProc(LTEvent hEvent, LTArgs * eventArgs)                                                                 { LT_UNUSED(hEvent); LT_UNUSED(eventArgs); }
static void MyNotifyImmediateEventStateProc(LTEvent hEvent, void * notifyImmediateClientData, void * eventProc, void * eventProcClientData) { LT_UNUSED(hEvent); LT_UNUSED(notifyImmediateClientData); LT_UNUSED(eventProc); LT_UNUSED(eventProcClientData); }
static void MyEventProc(LTEvent hEvent, void *clientData)                                                                                   { LT_UNUSED(hEvent); LT_UNUSED(clientData); }

static void MyEventClientDataReleaseProc(LTThread_ReleaseReason reason, void *clientData) {
    LTAtomic_FetchAdd(&s_reasoning.eventReleaseCount, 1);
    s_reasoning.eventReleaseReason = reason;
    s_reasoning.eventReleaseClientData = (u32)(LT_SIZE)clientData;
}

static void MyThreadSpecificReleaseProc(LTThread_ReleaseReason reason, void *clientData) {
    LTAtomic_FetchAdd(&s_reasoning.threadReleaseCount, 1);
    s_reasoning.threadReleaseReason = reason;
    s_reasoning.threadReleaseClientData = (u32)(LT_SIZE)clientData;
}

static void MyTimerClientDataReleaseProc(LTThread_ReleaseReason reason, void *clientData) {
    LTAtomic_FetchAdd(&s_reasoning.timerReleaseCount, 1);
    s_reasoning.timerReleaseReason = reason;
    s_reasoning.timerReleaseClientData = (u32)(LT_SIZE)clientData;
}

static void MyQueueTaskProcReleaseProc(LTThread_ReleaseReason reason, void *clientData) {
    LTAtomic_FetchAdd(&s_reasoning.queueTaskProcReleaseCount, 1);
    s_reasoning.queueTaskProcReleaseReason = reason;
    s_reasoning.queueTaskProcReleaseClientData = (u32)(LT_SIZE)clientData;
}

static bool MyFalsifyingThreadInitProc(void) {
    return false;
}

/************************
 TestReleaseClientData */
void TestReleaseClientData(TiltHarness *harness) {
    LTCore    *core    = LT_GetCore();
    TILT_EXPECT_TRUE(harness, core, "LT_GetCore() failed");

    LTThread  hThread  = core->CreateThread(THREAD_NAME);
    LTEvent   hEvent   = core->CreateEvent(&s_eventArgs, &MyEventDispatchProc, &MyEventDispatchCompleteProc, &MyNotifyImmediateEventStateProc, NULL);
    ILTThread *iThread = lt_gethandleinterface(ILTThread, hThread);
    ILTEvent  *iEvent  = lt_gethandleinterface(ILTEvent, hEvent);
    TILT_EXPECT_TRUE(harness, hThread, "Couldn't create thread");
    TILT_EXPECT_TRUE(harness, hEvent, "Couldn't create event");
    TILT_EXPECT_TRUE(harness, iThread, "Failed to get iThread interface");
    TILT_EXPECT_TRUE(harness, iEvent, "Failed to get iEvent interface");

    // test that the clientdata release proc gets called when unregistering from the event
    ResetReasoning();
    iEvent->RegisterForEvent(hEvent, &MyEventProc, &MyEventClientDataReleaseProc, (void *)1, false);
    iEvent->UnregisterFromEvent(hEvent, &MyEventProc);
    TILT_EXPECT_TRUE(harness, LTAtomic_Load(&s_reasoning.eventReleaseCount)  == 1, "LTEvent->UnregisterFromEvent did not release the clientdata");
    TILT_EXPECT_TRUE(harness, s_reasoning.eventReleaseReason == kLTThread_ReleaseReason_EventUnregistered, "LTEvent->UnregisterFromEvent gave the wrong reason for releasing clientData");
    TILT_EXPECT_TRUE(harness, s_reasoning.eventReleaseClientData  == 1, "LTEvent->UnregisterFromEvent released wrong clientdata");

    // test that the clientdata release proc gets called when destroying the event while registered
    ResetReasoning();
    iEvent->RegisterForEvent(hEvent, &MyEventProc, &MyEventClientDataReleaseProc, (void *)2, false);
    iEvent->Destroy(hEvent);
    TILT_EXPECT_TRUE(harness, LTAtomic_Load(&s_reasoning.eventReleaseCount)  == 1, "LTEvent->Destroy did not release the clientdata");
    TILT_EXPECT_TRUE(harness, s_reasoning.eventReleaseReason == kLTThread_ReleaseReason_EventDestroyed, "LTEvent->Destroy gave the wrong reason for releasing clientData");
    TILT_EXPECT_TRUE(harness, s_reasoning.eventReleaseClientData    == 2, "LTEvent->Destroy released wrong clientdata");

    // test setting thread specific client data and destroying the thread without starting it
    ResetReasoning();
    iThread->SetThreadSpecificClientData(hThread, THREAD_SPECIFIC_DATA_KEY, &MyThreadSpecificReleaseProc, (void *)3); // set
    iThread->Destroy(hThread);
    TILT_EXPECT_TRUE(harness, LTAtomic_Load(&s_reasoning.threadReleaseCount)  == 1, "iThread->Destroy on non-started thread did not release the clientdata");
    TILT_EXPECT_TRUE(harness, s_reasoning.threadReleaseReason == kLTThread_ReleaseReason_ThreadSpecificPurge, "iThread->Destroy on non-started thread gave the wrong reason for releasing clientData");
    TILT_EXPECT_TRUE(harness, s_reasoning.threadReleaseClientData == 3, "iThread->Destroy on non-started thread released wrong clientdata");
    hThread  = core->CreateThread(THREAD_NAME);
    TILT_EXPECT_TRUE(harness, hThread, "Couldn't create thread after destroying it");

    // test re-setting and clearing thread specific client data
    ResetReasoning();
    iThread->SetThreadSpecificClientData(hThread, THREAD_SPECIFIC_DATA_KEY, &MyThreadSpecificReleaseProc, (void *)1); // set
    iThread->SetThreadSpecificClientData(hThread, THREAD_SPECIFIC_DATA_KEY, &MyThreadSpecificReleaseProc, (void *)2); // re-set
    TILT_EXPECT_TRUE(harness, LTAtomic_Load(&s_reasoning.threadReleaseCount)  == 1, "iThread->SetThreadSpecificClientData did not release existing clientdata when new clientdata set");
    TILT_EXPECT_TRUE(harness, s_reasoning.threadReleaseReason == kLTThread_ReleaseReason_ThreadSpecificReset, "iThread->SetThreadSpecificClientData gave the wrong reason for releasing clientData");
    TILT_EXPECT_TRUE(harness, s_reasoning.threadReleaseClientData == 1, "iThread->SetThreadSpecificClientData released wrong clientdata");
    s_reasoning.threadReleaseReason = kLTThread_ReleaseReason_NullReason;
    iThread->SetThreadSpecificClientData(hThread, THREAD_SPECIFIC_DATA_KEY, &MyThreadSpecificReleaseProc, NULL);      // clear
    TILT_EXPECT_TRUE(harness, LTAtomic_Load(&s_reasoning.threadReleaseCount)  == 2, "iThread->SetThreadSpecificClientData did not release clientdata when cleared");
    TILT_EXPECT_TRUE(harness, s_reasoning.threadReleaseReason == kLTThread_ReleaseReason_ThreadSpecificReset, "iThread->SetThreadSpecificClientData, when clearing, gave the wrong reason for releasing clientData");
    TILT_EXPECT_TRUE(harness, s_reasoning.threadReleaseClientData  == 2, "Thread->SetThreadSpecificClientData, when clearing, released the wrong clientdata");

    // test returning false from ThreadInitProc
    ResetReasoning();
    iThread->SetThreadSpecificClientData(hThread, THREAD_SPECIFIC_DATA_KEY, &MyThreadSpecificReleaseProc, (void *)1);
    iThread->Start(hThread, &MyFalsifyingThreadInitProc, NULL);
    iThread->WaitUntilFinished(hThread, LTTime_Infinite());
    TILT_EXPECT_TRUE(harness, LTAtomic_Load(&s_reasoning.threadReleaseCount)  == 1, "LTThread did not release thread specific clientdata after ThreadInit returned false");
    TILT_EXPECT_TRUE(harness, s_reasoning.threadReleaseReason == kLTThread_ReleaseReason_ThreadSpecificPurge, "LTThread gave the wrong reason for releasing thread specific clientData after ThreadInit returned false");
    TILT_EXPECT_TRUE(harness, s_reasoning.threadReleaseClientData == 1, "LTThread released the wrong clientdata after ThreadInit returned false");
    iThread->Destroy(hThread);

    // _________________________
    // TESTS STILL TO BE WRITTEN
    // test cleanup from destroy with thread not yet running
    // test loaded cleanup from destroy with thread running
    // test loaded cleanup from terminate
    // test cleanup after QueueTaskProc
    // test cleanup from KillTimer


    // _________________________________________________________________
    // These LT_UNUSED GO AWAY WHEN THE TESTS TO BE WRITTEN ARE WRITTEN!
    LT_UNUSED(MyQueueTaskProcReleaseProc);
    LT_UNUSED(MyTimerClientDataReleaseProc);


}

/*******************************************************************************
 *  LOG
 *******************************************************************************
 *  26-Oct-22   augustus    created
 */
