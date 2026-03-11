#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Types/SharedPtr.h>
#include <JoltPlugin/System/JoltJobSystem.h>

ezJoltJobSystem::ezJoltJobSystem(ezUInt32 uiMaxJobs, ezUInt32 uiMaxBarriers)
{
  JobSystemWithBarrier::Init(uiMaxBarriers);

  m_Jobs.Init(uiMaxJobs, uiMaxJobs);

  m_Tasks.SetCount(uiMaxJobs);
  for (ezUInt32 i = 0; i < m_Tasks.GetCount(); ++i)
  {
    m_Tasks[i] = EZ_DEFAULT_NEW(ezJoltTask);
    m_Tasks[i]->ConfigureTask("Jolt", ezTaskNesting::Never, &ezJoltJobSystem::OnTaskFinished);
  }
}

int ezJoltJobSystem::GetMaxConcurrency() const
{
  return ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::ShortTasks);
}

JPH::JobHandle ezJoltJobSystem::CreateJob(const char* szName, JPH::ColorArg color, const JobFunction& jobFunction, ezUInt32 uiNumDependencies)
{
  // Loop until we can get a job from the free list
  ezUInt32 index;
  for (;;)
  {
    index = m_Jobs.ConstructObject(szName, color, this, jobFunction, uiNumDependencies);
    if (index != AvailableJobs::cInvalidObjectIndex)
      break;

    EZ_ASSERT_DEBUG(false, "No Jolt jobs available!");
    ezThreadUtils::YieldTimeSlice();
  }

  CustomJob* job = &m_Jobs.Get(index);
  job->m_uiJobIndex = index;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  {
    ezStringBuilder name("Jolt-", szName);
    m_Tasks[index]->ConfigureTask(name, ezTaskNesting::Never, &ezJoltJobSystem::OnTaskFinished);
  }
#endif

  // Construct handle to keep a reference, the job is queued below and may immediately complete
  JobHandle handle(job);

  // If there are no dependencies, queue the job now
  if (uiNumDependencies == 0)
    QueueJob(job);

  // Return the handle
  return handle;
}

void ezJoltJobSystem::FreeJob(Job* pJob)
{
  m_Jobs.DestructObject(static_cast<CustomJob*>(pJob));
}

void ezJoltJobSystem::OnTaskFinished(const ezSharedPtr<ezTask>& task)
{
  ezJoltTask* pTask = static_cast<ezJoltTask*>(task.Borrow());

  auto* pJob = static_cast<JPH::JobSystem::Job*>(pTask->m_pJob);
  pTask->m_pJob = nullptr;

  // doing this here prevens a race condition in reusing tasks
  pJob->Release();
}

void ezJoltJobSystem::QueueJob(Job* pJob)
{
  auto* pMyJob = static_cast<CustomJob*>(pJob);
  pMyJob->AddRef();

  const auto& pTask = m_Tasks[pMyJob->m_uiJobIndex];

  pTask->m_pJob = pMyJob;

  ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::EarlyThisFrame);
}

void ezJoltJobSystem::QueueJobs(Job** pJob, ezUInt32 uiNum_Jobs)
{
  for (ezUInt32 i = 0; i < uiNum_Jobs; ++i)
  {
    QueueJob(pJob[i]);
  }
}

void ezJoltJobSystem::ezJoltTask::Execute()
{
  m_pJob->Execute();
}
