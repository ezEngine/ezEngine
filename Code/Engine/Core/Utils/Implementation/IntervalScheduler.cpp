#include <Core/CorePCH.h>

#include <Core/Utils/IntervalScheduler.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezUpdateRate, 1)
  EZ_ENUM_CONSTANTS(ezUpdateRate::EveryFrame)
  EZ_ENUM_CONSTANTS(ezUpdateRate::Max30fps, ezUpdateRate::Max20fps, ezUpdateRate::Max10fps)
  EZ_ENUM_CONSTANTS(ezUpdateRate::Max5fps, ezUpdateRate::Max2fps, ezUpdateRate::Max1fps)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

static ezTime s_Intervals[] = {
  ezTime::Zero(),              // EveryFrame
  ezTime::Seconds(1.0 / 30.0), // Max30fps
  ezTime::Seconds(1.0 / 20.0), // Max20fps
  ezTime::Seconds(1.0 / 10.0), // Max10fps
  ezTime::Seconds(1.0 / 5.0),  // Max5fps
  ezTime::Seconds(1.0 / 2.0),  // Max2fps
  ezTime::Seconds(1.0 / 1.0),  // Max1fps
};

static_assert(EZ_ARRAY_SIZE(s_Intervals) == ezUpdateRate::Max1fps + 1);

ezTime ezUpdateRate::GetInterval(Enum updateRate)
{
  return s_Intervals[updateRate];
}

//////////////////////////////////////////////////////////////////////////

ezIntervalSchedulerBase::ezIntervalSchedulerBase(ezTime minInterval, ezTime maxInterval)
  : m_MinInterval(minInterval)
  , m_MaxInterval(maxInterval)
{
  m_fInvIntervalRange = 1.0 / (m_MaxInterval - m_MinInterval).GetSeconds();

  for (ezUInt32 i = 0; i < HistogramSize; ++i)
  {
    m_HistogramSlotValues[i] = GetHistogramSlotValue(i);
  }
}

ezIntervalSchedulerBase::~ezIntervalSchedulerBase() = default;


EZ_STATICLINK_FILE(Core, Core_Utils_Implementation_IntervalScheduler);
