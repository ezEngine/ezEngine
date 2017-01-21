#pragma once

#include <RendererCore/Basics.h>
#include <RendererFoundation/Basics.h>

class ezGALDevice;

/// Debugging utility class for easy and stalling free GPU timing measures.
///
/// The timer is not suitable to be reused for different events since it will return timing result from n frames past.
/// Keep in mind that this is unlike a CPU stopwatch a non-trivial gpu object and should not be recreated too often.
class EZ_RENDERERCORE_DLL ezGPUStopwatch
{
public:
  ezGPUStopwatch(ezGALDevice& device, ezUInt32 numFramesDelay = 3);
  ~ezGPUStopwatch();

  /// Starts the timer. 
  void Begin(ezGALContext& context);

  /// Ends the timer and queries the result from the oldest queries in the queue.
  /// \param outTimeDifference
  ///   Optional argument. Writes out GPU time difference between Begin and End from numFramesDelay frames before.
  /// \return
  ///   Success if querying the timer worked. Otherwise the passed time will be set to null.
  ezResult End(ezGALContext& context, ezTime* outTimeDifference);

  /// Gets result from the last successful call to End.
  ezTime GetLastResult() const { return m_lastResult; }


  /// Scope object that calls Begin on construction and End on destruction;
  struct Scope
  {
    Scope(ezGPUStopwatch& stopwatch, ezGALContext& context) : m_stopwatch(stopwatch), m_context(context) { m_stopwatch.Begin(m_context); }
    ~Scope() { m_stopwatch.End(m_context, nullptr); }

  private:
    ezGPUStopwatch& m_stopwatch;
    ezGALContext& m_context;
  };


private:

  struct QueryPair
  {
    ezGALQueryHandle beginQuery;
    ezGALQueryHandle endQuery;
  };

  ezDynamicArray<QueryPair> m_queries;
  ezUInt32 m_nextQuery;
  bool m_running;
  ezGALDevice& m_device;
  ezTime m_lastResult;
};
