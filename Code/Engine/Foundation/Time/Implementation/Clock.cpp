#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Clock.h>

ezClock::Event ezClock::s_TimeEvents;
ezClock* ezClock::s_pGlobalClock = nullptr;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Clock)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_BASESYSTEMS_STARTUP
  {
    ezClock::s_pGlobalClock = new ezClock("Global");
  }

EZ_END_SUBSYSTEM_DECLARATION;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezClock, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Paused", GetPaused, SetPaused),
    EZ_ACCESSOR_PROPERTY("Speed", GetSpeed, SetSpeed),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(GetGlobalClock),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetAccumulatedTime),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetTimeDiff)
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezClock::ezClock(ezStringView sName)
{
  SetClockName(sName);

  Reset(true);
}

void ezClock::Reset(bool bEverything)
{
  if (bEverything)
  {
    m_pTimeStepSmoother = nullptr;
    m_MinTimeStep = ezTime::MakeFromSeconds(0.001); // 1000 FPS
    m_MaxTimeStep = ezTime::MakeFromSeconds(0.1);   //   10 FPS, many simulations will be instable at that rate already
    m_FixedTimeStep = ezTime::MakeFromSeconds(0.0);
  }

  m_AccumulatedTime = ezTime::MakeFromSeconds(0.0);
  m_fSpeed = 1.0;
  m_bPaused = false;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = ezTime::Now() - m_MinTimeStep;
  m_LastTimeDiff = m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}

void ezClock::Update()
{
  const ezTime tNow = ezTime::Now();
  const ezTime tDiff = tNow - m_LastTimeUpdate;
  m_LastTimeUpdate = tNow;

  if (m_bPaused)
  {
    // no change during pause
    m_LastTimeDiff = ezTime::MakeFromSeconds(0.0);
  }
  else if (m_FixedTimeStep > ezTime::MakeFromSeconds(0.0))
  {
    // scale the time step by the speed factor
    m_LastTimeDiff = m_FixedTimeStep * m_fSpeed;
  }
  else
  {
    // in variable time step mode, apply the time step smoother, if available
    if (m_pTimeStepSmoother)
      m_LastTimeDiff = m_pTimeStepSmoother->GetSmoothedTimeStep(tDiff, this);
    else
    {
      // scale the time step by the speed factor
      // and make sure the time step does not leave the predetermined bounds
      m_LastTimeDiff = ezMath::Clamp(tDiff * m_fSpeed, m_MinTimeStep, m_MaxTimeStep);
    }
  }

  m_AccumulatedTime += m_LastTimeDiff;

  EventData ed;
  ed.m_sClockName = m_sName;
  ed.m_RawTimeStep = tDiff;
  ed.m_SmoothedTimeStep = m_LastTimeDiff;

  s_TimeEvents.Broadcast(ed);
}

void ezClock::SetAccumulatedTime(ezTime t)
{
  m_AccumulatedTime = t;

  // this is to prevent having a time difference of zero (which might not work with some code)
  // in case the next Update() call is done right after this
  m_LastTimeUpdate = ezTime::Now() - ezTime::MakeFromSeconds(0.01);
  m_LastTimeDiff = ezTime::MakeFromSeconds(0.01);
}

void ezClock::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;

  inout_stream << uiVersion;
  inout_stream << m_AccumulatedTime;
  inout_stream << m_LastTimeDiff;
  inout_stream << m_FixedTimeStep;
  inout_stream << m_MinTimeStep;
  inout_stream << m_MaxTimeStep;
  inout_stream << m_fSpeed;
  inout_stream << m_bPaused;
}

void ezClock::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Wrong version for ezClock: {0}", uiVersion);

  inout_stream >> m_AccumulatedTime;
  inout_stream >> m_LastTimeDiff;
  inout_stream >> m_FixedTimeStep;
  inout_stream >> m_MinTimeStep;
  inout_stream >> m_MaxTimeStep;
  inout_stream >> m_fSpeed;
  inout_stream >> m_bPaused;

  // make sure we continue properly
  m_LastTimeUpdate = ezTime::Now() - m_MinTimeStep;

  if (m_pTimeStepSmoother)
    m_pTimeStepSmoother->Reset(this);
}



EZ_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Clock);
