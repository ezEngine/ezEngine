module ez.foundation.time.clock;

public import ez.foundation.time.time;

extern(C++, class) struct ezClock
{
  // CODEGEN-BEGIN: Struct("ezClock")
  static ezClock* GetGlobalClock();
  this(const(char)* szName);
  void Reset(bool bEverything);
  void Update();
  // method 'SetTimeStepSmoothing' - unsupported argument 'pSmoother'
  // method 'GetTimeStepSmoothing' - unsupported return type
  void SetPaused(bool bPaused);
  bool GetPaused() const;
  void SetFixedTimeStep(ezTime tDiff /* = ezTime() */);
  ezTime GetFixedTimeStep() const;
  void SetAccumulatedTime(ezTime t);
  ezTime GetAccumulatedTime() const;
  ezTime GetTimeDiff() const;
  void SetSpeed(double fFactor);
  double GetSpeed() const;
  void SetMinimumTimeStep(ezTime tMin);
  void SetMaximumTimeStep(ezTime tMax);
  ezTime GetMinimumTimeStep() const;
  ezTime GetMaximumTimeStep() const;
  // method 'Save' - unsupported argument 'Stream'
  // method 'Load' - unsupported argument 'Stream'
  void SetClockName(const(char)* szName);
  const(char)* GetClockName() const;
  struct EventData
  {
    const(char)* m_szClockName;
    ezTime m_RawTimeStep;
    ezTime m_SmoothedTimeStep;
    // Operator: =
  }
  // Typedef 'Event' has unsupported type
  // method 'AddEventHandler' - unsupported argument 'handler'
  // method 'RemoveEventHandler' - unsupported argument 'handler'
  // Operator: =
  // CODEGEN-END
}
