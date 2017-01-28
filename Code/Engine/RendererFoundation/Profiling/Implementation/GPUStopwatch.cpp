#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Profiling/GPUStopwatch.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Context/Context.h>

ezGPUStopwatch::ezGPUStopwatch(ezGALDevice& device, unsigned int numFramesDelay)
  : m_nextQuery(0)
  , m_running(false)
  , m_device(device)
  , m_lastResult(ezTime::Zero())
{
  m_queries.SetCount(numFramesDelay);

  ezGALQueryCreationDescription queryDesc;
  queryDesc.m_type = ezGALQueryType::Timestamp;
  for (auto& queryGroup : m_queries)
  {
    queryGroup.beginQuery = device.CreateQuery(queryDesc);
    queryGroup.endQuery = device.CreateQuery(queryDesc);
  }
}

ezGPUStopwatch::~ezGPUStopwatch()
{
  for (auto& queryGroup : m_queries)
  {
    m_device.DestroyQuery(queryGroup.beginQuery);
    m_device.DestroyQuery(queryGroup.endQuery);
  }
}

void ezGPUStopwatch::Begin(ezGALContext& context)
{
  EZ_ASSERT_DEV(!m_running, "Can't begin gpu stopwatch since it is already running!");
  m_running = true;

  context.EndQuery(m_queries[m_nextQuery].beginQuery);
}

ezResult ezGPUStopwatch::End(ezGALContext& context, ezTime* outTimeDifference)
{
  EZ_ASSERT_DEV(m_running, "Can't stop gpu stopwatch since it is not running!");
  m_running = false;

  context.EndQuery(m_queries[m_nextQuery].endQuery);
  m_nextQuery = (m_nextQuery + 1) % m_queries.GetCount();

  if(outTimeDifference)
    *outTimeDifference = ezTime::Zero();

  // Retrieve oldest query data and convert to time.
  // Each call may fail, but check only at the end to avoid DX giving us warnings about pointless queries.
  ezUInt64 timestampBegin, timestampEnd;
  ezResult beginResult = context.GetQueryResult(m_queries[m_nextQuery].beginQuery, timestampBegin);
  ezResult endResult = context.GetQueryResult(m_queries[m_nextQuery].endQuery, timestampEnd);
  if (beginResult.Failed() || endResult.Failed())
    return EZ_FAILURE;

  // Frequency might change and retrieval might even fail.
  ezUInt64 ticksPerSecond = m_device.GetTimestampTicksPerSecond();
  if (ticksPerSecond == 0)
    return EZ_FAILURE;

  m_lastResult = ezTime::Seconds(static_cast<double>(timestampEnd - timestampBegin) / ticksPerSecond);
  if (outTimeDifference)
    *outTimeDifference = m_lastResult;

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Profiling_Implementation_GPUStopwatch);

