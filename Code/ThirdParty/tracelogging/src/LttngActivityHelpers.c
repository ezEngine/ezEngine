// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <lttngh/LttngHelpers.h>
#include <byteswap.h> // bswap_32, BYTE_ORDER
#include <time.h>   // clock_gettime
#include <unistd.h> // getpid


#if BYTE_ORDER == LITTLE_ENDIAN
#define HostToBigEndian32(x) bswap_32(x)
#define HostToLittleEndian32(x) (x)
#else
#define HostToBigEndian32(x) (x)
#define HostToLittleEndian32(x) bswap_32(x)
#endif

/*
This is a 64-bit value that is a (hopefully) unique identifier for the activity
ID generator, i.e. no other activity ID generator on the system will have the
same root. Uniqueness applies within the current machine's boot session -- you
might hit the same ID on another machine or on the same machine after
rebooting.
*/
struct lttngh_ActivityRoot {
  uint32_t Root0;
  uint32_t Root1;
};

/*
The thread-local data needed by the activity ID system.
*/
struct lttngh_ThreadInfo {
  uint32_t *ThreadActivityPtr;    // ActivityIdForEvent
  uint32_t ThreadActivityId[4];   // ActivityIdGet/ActivityIdSet
  uint32_t ThreadActivityNext[4]; // ActivityIdCreate
};

static __thread struct lttngh_ThreadInfo ThreadInfo;

/*
Objective: generate a 64-bit "activity generator ID" that is unlikely to be
used by any other activity ID generator writing to the same trace.
TODO: This could probably be improved.
*/
static void
ActivityRootCreate(struct lttngh_ActivityRoot *pNewRoot) lttng_ust_notrace;
static void ActivityRootCreate(struct lttngh_ActivityRoot *pNewRoot) {
  uint32_t timeBits;
  struct timespec tv;

  clock_gettime(CLOCK_MONOTONIC, &tv);
  timeBits = ((uint32_t)tv.tv_sec << 8) |  // Low 24 bits of seconds.
             ((uint32_t)tv.tv_nsec >> 22); // Top 8 bits of nanoseconds.

  // Distinguish by timestamp in case PID rolls over.
  pNewRoot->Root0 = timeBits;

  // Distinguish by address of lttngh_ActivityIdCreate in case multiple
  // generators exist in the process (i.e. from different .so libraries).
  // TODO: Remove this when LttngHelpers is made into a .so library.
  pNewRoot->Root0 ^= (uint32_t)(uintptr_t)&lttngh_ActivityIdCreate;

  // Distinguish by PID so different processes get different activity IDs.
  // Use little-endian to reduce conflict with version field.
  pNewRoot->Root1 = HostToLittleEndian32((uint32_t)getpid());

  // Since this isn't really a GUID, set the GUID version bits to 0
  // (invalid version).
  pNewRoot->Root1 &= HostToBigEndian32(0xffff0fff);
}

void lttngh_ActivityIdCreate(void *pNewActivityId) {
  static pthread_mutex_t StaticMutex = PTHREAD_MUTEX_INITIALIZER;
  static struct lttngh_ActivityRoot StaticRoot; // Generator ID.
  static uint32_t StaticPool; // Incremented as threads allocate pools.

  // Return a 16-byte value that is unlikely to conflict with other values
  // in the same trace. A UUID would work, but would be more expensive than
  // we want. Instead, generate a 3-part locally-unique id:
  // - The top 64 bits distinguish our generator from other generators that
  //   might also be writing to the same trace.
  // - The next 32 bits are a pool number. This distinguishes between
  //   threads using our generator (each thread allocates a pool as needed).
  //   If we run out of pools, we reseed the top 64 bits.
  // - The next 32 bits are assigned by incrementing a thread-local value.
  //   If this overflows, the thread allocates a new pool.
  struct lttngh_ThreadInfo *pInfo = &ThreadInfo;
  if (caa_unlikely(pInfo->ThreadActivityNext[3] == 0)) {
    // Allocate a new pool for our thread.

    struct lttngh_ActivityRoot localRoot;
    uint32_t localPool;
    char localRootIsValid;

    localPool = __atomic_load_n(&StaticPool, __ATOMIC_RELAXED);
    if (localPool != 0) {
      // Optimistically assume that we won't need to generate a root.
      localRootIsValid = 0;
    } else {
      // Assume that we'll need a new root.
      ActivityRootCreate(&localRoot);
      localRootIsValid = 1;
    }

    pthread_mutex_lock(&StaticMutex);

    localPool = StaticPool;
    if (caa_unlikely(localPool == 0 && !localRootIsValid)) {
      // StaticPool became 0, so we need to generate a root after all.
      pthread_mutex_unlock(&StaticMutex);
      ActivityRootCreate(&localRoot);
      pthread_mutex_lock(&StaticMutex);
      localPool = StaticPool;
    }

    StaticPool = (localPool + 1) & 0x3fffffff;
    if (localPool == 0) {
      // Update the global root with our newly-generated root.
      StaticRoot = localRoot;
    } else {
      // Update our root from the global root.
      localRoot = StaticRoot;
    }

    pthread_mutex_unlock(&StaticMutex);

    assert(0 == (localPool & 0xc0000000)); // Memory corruption?
    pInfo->ThreadActivityNext[0] = localRoot.Root0;
    pInfo->ThreadActivityNext[1] = localRoot.Root1;

    // Set the GUID variant bits to "RFC4122", even though we aren't really a
    // GUID.
    pInfo->ThreadActivityNext[2] = HostToBigEndian32(localPool | 0x80000000);
  }

  memcpy(pNewActivityId, pInfo->ThreadActivityNext,
         sizeof(pInfo->ThreadActivityNext));
  pInfo->ThreadActivityNext[3] += 1;
}

void lttngh_ActivityIdGet(void *pCurrentThreadActivityId) {
  memcpy(pCurrentThreadActivityId, ThreadInfo.ThreadActivityId,
         sizeof(ThreadInfo.ThreadActivityId));
}

void lttngh_ActivityIdSet(void const *pNewThreadActivityId) {
  struct lttngh_ThreadInfo *pInfo = &ThreadInfo;
  memcpy(pInfo->ThreadActivityId, pNewThreadActivityId,
         sizeof(pInfo->ThreadActivityId));
  if (pInfo->ThreadActivityId[0] | pInfo->ThreadActivityId[1] |
      pInfo->ThreadActivityId[2] | pInfo->ThreadActivityId[3]) {
    pInfo->ThreadActivityPtr = pInfo->ThreadActivityId;
  } else {
    pInfo->ThreadActivityPtr = NULL;
  }
}

void const *lttngh_ActivityIdPeek(void) { return ThreadInfo.ThreadActivityPtr; }
