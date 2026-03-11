#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <Jolt/Jolt.h>

#include <Jolt/Core/FixedSizeFreeList.h>
#include <Jolt/Core/JobSystemWithBarrier.h>

class ezJoltJobSystem final : public JPH::JobSystemWithBarrier
{
public:
  ezJoltJobSystem(ezUInt32 uiMaxJobs, ezUInt32 uiMaxBarriers);

  virtual int GetMaxConcurrency() const override;
  virtual JPH::JobHandle CreateJob(const char* szName, JPH::ColorArg color, const JobFunction& jobFunction, ezUInt32 uiNumDependencies = 0) override;

  virtual void QueueJob(Job* pJob) override;
  virtual void QueueJobs(Job** pJobs, ezUInt32 uiNumJobs) override;
  virtual void FreeJob(Job* pJob) override;

private:
  static void OnTaskFinished(const ezSharedPtr<ezTask>& task);

  class CustomJob : public JPH::JobSystem::Job
  {
  public:
    CustomJob(const char* szJobName, JPH::ColorArg color, JPH::JobSystem* pJobSystem, const JobFunction& jobFunction, ezUInt32 uiNumDependencies)
      : Job(szJobName, color, pJobSystem, jobFunction, uiNumDependencies)
    {
    }

    ezUInt32 m_uiJobIndex = ezInvalidIndex;
  };

  class ezJoltTask : public ezTask
  {
  public:
    CustomJob* m_pJob = nullptr;

    virtual void Execute() override;
  };


  using AvailableJobs = JPH::FixedSizeFreeList<CustomJob>;
  AvailableJobs m_Jobs;

  ezDynamicArray<ezSharedPtr<ezJoltTask>> m_Tasks;
};
