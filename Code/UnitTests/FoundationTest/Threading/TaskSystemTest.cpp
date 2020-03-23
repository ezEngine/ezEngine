#include <FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Utilities/DGMLWriter.h>

class ezTestTask : public ezTask
{
public:
  ezUInt32 m_uiIterations;
  ezTestTask* m_pDependency;
  bool m_bSupportCancel;
  ezInt32 m_iTaskID;

  ezTestTask()
    : ezTask("TestTask")
  {
    m_uiIterations = 50;
    m_pDependency = nullptr;
    m_bStarted = false;
    m_bDone = false;
    m_bSupportCancel = false;
    m_iTaskID = -1;
  }

  bool IsStarted() const { return m_bStarted; }
  bool IsDone() const { return m_bDone; }
  bool IsMultiplicityDone() const { return m_MultiplicityCount == (int)GetMultiplicity(); }

private:
  bool m_bStarted;
  bool m_bDone;
  mutable ezAtomicInteger32 m_MultiplicityCount;

  virtual void ExecuteWithMultiplicity(ezUInt32 uiInvocation) const override { m_MultiplicityCount.Increment(); }

  virtual void Execute() override
  {
    if (m_iTaskID >= 0)
      printf("Starting Task %i at %.4f\n", m_iTaskID, ezTime::Now().GetSeconds());

    m_bStarted = true;

    EZ_TEST_BOOL(m_pDependency == nullptr || m_pDependency->IsTaskFinished());

    for (ezUInt32 obst = 0; obst < m_uiIterations; ++obst)
    {
      ezThreadUtils::Sleep(ezTime::Milliseconds(1));
      ezTime::Now();

      if (HasBeenCanceled() && m_bSupportCancel)
      {
        if (m_iTaskID >= 0)
          printf("Canceling Task %i at %.4f\n", m_iTaskID, ezTime::Now().GetSeconds());
        return;
      }
    }

    m_bDone = true;

    if (m_iTaskID >= 0)
      printf("Finishing Task %i at %.4f\n", m_iTaskID, ezTime::Now().GetSeconds());
  }
};

class TaskCallbacks
{
public:
  void TaskFinished(ezTask* pTask) { m_pInt->Increment(); }

  void TaskGroupFinished() { m_pInt->Increment(); }

  ezAtomicInteger32* m_pInt;
};

EZ_CREATE_SIMPLE_TEST(Threading, TaskSystem)
{
  ezInt8 iWorkersShort = 4;
  ezInt8 iWorkersLong = 4;

  ezTaskSystem::SetWorkerThreadCount(iWorkersShort, iWorkersLong);
  ezThreadUtils::Sleep(ezTime::Milliseconds(500));

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single Tasks")
  {
    ezTestTask t[3];

    t[0].SetTaskName("Task 0");
    t[1].SetTaskName("Task 1");
    t[2].SetTaskName("Task 2");

    auto tg0 = ezTaskSystem::StartSingleTask(&t[0], ezTaskPriority::LateThisFrame);
    auto tg1 = ezTaskSystem::StartSingleTask(&t[1], ezTaskPriority::ThisFrame);
    auto tg2 = ezTaskSystem::StartSingleTask(&t[2], ezTaskPriority::EarlyThisFrame);

    ezTaskSystem::WaitForGroup(tg0);
    ezTaskSystem::WaitForGroup(tg1);
    ezTaskSystem::WaitForGroup(tg2);

    EZ_TEST_BOOL(t[0].IsDone());
    EZ_TEST_BOOL(t[1].IsDone());
    EZ_TEST_BOOL(t[2].IsDone());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single Tasks with Dependencies")
  {
    ezTestTask t[4];
    ezTaskGroupID g[4];

    g[0] = ezTaskSystem::StartSingleTask(&t[0], ezTaskPriority::LateThisFrame);
    g[1] = ezTaskSystem::StartSingleTask(&t[1], ezTaskPriority::ThisFrame, g[0]);
    g[2] = ezTaskSystem::StartSingleTask(&t[2], ezTaskPriority::EarlyThisFrame, g[1]);
    g[3] = ezTaskSystem::StartSingleTask(&t[3], ezTaskPriority::EarlyThisFrame, g[0]);

    ezTaskSystem::WaitForGroup(g[2]);
    ezTaskSystem::WaitForGroup(g[3]);

    EZ_TEST_BOOL(t[0].IsDone());
    EZ_TEST_BOOL(t[1].IsDone());
    EZ_TEST_BOOL(t[2].IsDone());
    EZ_TEST_BOOL(t[3].IsDone());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Grouped Tasks / TaskFinished Callback / GroupFinished Callback")
  {
    ezTestTask t[8];
    ezTaskGroupID g[4];
    ezAtomicInteger32 GroupsFinished;
    ezAtomicInteger32 TasksFinished;

    TaskCallbacks callbackGroup;
    callbackGroup.m_pInt = &GroupsFinished;

    TaskCallbacks callbackTask;
    callbackTask.m_pInt = &TasksFinished;

    g[0] = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame, ezMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[1] = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame, ezMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[2] = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame, ezMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));
    g[3] = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame, ezMakeDelegate(&TaskCallbacks::TaskGroupFinished, &callbackGroup));

    for (int i = 0; i < 4; ++i)
      EZ_TEST_BOOL(!ezTaskSystem::IsTaskGroupFinished(g[i]));

    ezTaskSystem::AddTaskGroupDependency(g[1], g[0]);
    ezTaskSystem::AddTaskGroupDependency(g[2], g[0]);
    ezTaskSystem::AddTaskGroupDependency(g[3], g[1]);

    ezTaskSystem::AddTaskToGroup(g[0], &t[0]);
    ezTaskSystem::AddTaskToGroup(g[1], &t[1]);
    ezTaskSystem::AddTaskToGroup(g[1], &t[2]);
    ezTaskSystem::AddTaskToGroup(g[2], &t[3]);
    ezTaskSystem::AddTaskToGroup(g[2], &t[4]);
    ezTaskSystem::AddTaskToGroup(g[2], &t[5]);
    ezTaskSystem::AddTaskToGroup(g[3], &t[6]);
    ezTaskSystem::AddTaskToGroup(g[3], &t[7]);

    for (int i = 0; i < 8; ++i)
    {
      EZ_TEST_BOOL(!t[i].IsTaskFinished());
      EZ_TEST_BOOL(!t[i].IsDone());

      t[i].SetOnTaskFinished(ezMakeDelegate(&TaskCallbacks::TaskFinished, &callbackTask));
    }

    // do a snapshot
    // we don't validate it, just make sure it doesn't crash
    ezDGMLGraph graph;
    ezTaskSystem::WriteStateSnapshotToDGML(graph);

    ezTaskSystem::StartTaskGroup(g[3]);
    ezTaskSystem::StartTaskGroup(g[2]);
    ezTaskSystem::StartTaskGroup(g[1]);
    ezTaskSystem::StartTaskGroup(g[0]);

    ezTaskSystem::WaitForGroup(g[3]);
    ezTaskSystem::WaitForGroup(g[2]);
    ezTaskSystem::WaitForGroup(g[1]);
    ezTaskSystem::WaitForGroup(g[0]);

    EZ_TEST_INT(TasksFinished, 8);

    // It is not guaranteed that group finished callback is called after WaitForGroup returned so we need to wait a bit here.
    for (int i = 0; i < 10; i++)
    {
      if (GroupsFinished == 4)
      {
        break;
      }
      ezThreadUtils::Sleep(ezTime::Milliseconds(10));
    }
    EZ_TEST_INT(GroupsFinished, 4);

    for (int i = 0; i < 4; ++i)
      EZ_TEST_BOOL(ezTaskSystem::IsTaskGroupFinished(g[i]));

    for (int i = 0; i < 8; ++i)
    {
      EZ_TEST_BOOL(t[i].IsTaskFinished());
      EZ_TEST_BOOL(t[i].IsDone());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "This Frame Tasks / Next Frame Tasks")
  {
    const ezUInt32 uiNumTasks = 20;
    ezTestTask t[uiNumTasks];
    ezTaskGroupID tg[uiNumTasks];
    bool finished[uiNumTasks];

    for (ezUInt32 i = 0; i < uiNumTasks; i += 2)
    {
      finished[i] = false;
      finished[i + 1] = false;

      t[i].m_uiIterations = 10;
      t[i + 1].m_uiIterations = 20;

      tg[i] = ezTaskSystem::StartSingleTask(&t[i], ezTaskPriority::ThisFrame);
      tg[i + 1] = ezTaskSystem::StartSingleTask(&t[i + 1], ezTaskPriority::NextFrame);
    }

    // 'finish' the first frame
    ezTaskSystem::FinishFrameTasks();

    {
      ezUInt32 uiNotAllThisTasksFinished = 0;
      ezUInt32 uiNotAllNextTasksFinished = 0;

      for (ezUInt32 i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i].IsTaskFinished())
        {
          EZ_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1].IsTaskFinished())
        {
          EZ_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      // up to the number of worker threads tasks can still be active
      EZ_TEST_BOOL(uiNotAllThisTasksFinished <= ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::ShortTasks));
      EZ_TEST_BOOL(uiNotAllNextTasksFinished <= uiNumTasks);
    }


    // 'finish' the second frame
    ezTaskSystem::FinishFrameTasks();

    {
      ezUInt32 uiNotAllThisTasksFinished = 0;
      ezUInt32 uiNotAllNextTasksFinished = 0;

      for (int i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i].IsTaskFinished())
        {
          EZ_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1].IsTaskFinished())
        {
          EZ_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      EZ_TEST_BOOL(uiNotAllThisTasksFinished + uiNotAllNextTasksFinished <= ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::ShortTasks));
    }

    // 'finish' all frames
    ezTaskSystem::FinishFrameTasks();

    {
      ezUInt32 uiNotAllThisTasksFinished = 0;
      ezUInt32 uiNotAllNextTasksFinished = 0;

      for (ezUInt32 i = 0; i < uiNumTasks; i += 2)
      {
        if (!t[i].IsTaskFinished())
        {
          EZ_TEST_BOOL(!finished[i]);
          ++uiNotAllThisTasksFinished;
        }
        else
        {
          finished[i] = true;
        }

        if (!t[i + 1].IsTaskFinished())
        {
          EZ_TEST_BOOL(!finished[i + 1]);
          ++uiNotAllNextTasksFinished;
        }
        else
        {
          finished[i + 1] = true;
        }
      }

      // even after finishing multiple frames, the previous frame tasks may still be in execution
      // since no N+x tasks enforce their completion in this test
      EZ_TEST_BOOL(uiNotAllThisTasksFinished + uiNotAllNextTasksFinished <= ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::ShortTasks));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Main Thread Tasks")
  {
    const ezUInt32 uiNumTasks = 20;
    ezTestTask t[uiNumTasks];

    for (ezUInt32 i = 0; i < uiNumTasks; ++i)
    {
      t[i].m_uiIterations = 10;

      ezTaskSystem::StartSingleTask(&t[i], ezTaskPriority::ThisFrameMainThread);
    }

    ezTaskSystem::FinishFrameTasks();

    for (ezUInt32 i = 0; i < uiNumTasks; ++i)
    {
      EZ_TEST_BOOL(t[i].IsTaskFinished());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Canceling Tasks")
  {
    const ezUInt32 uiNumTasks = 20;
    ezTestTask t[uiNumTasks];
    ezTaskGroupID tg[uiNumTasks];

    for (int i = 0; i < uiNumTasks; ++i)
    {
      t[i].m_uiIterations = 50;

      tg[i] = ezTaskSystem::StartSingleTask(&t[i], ezTaskPriority::ThisFrame);
    }

    ezThreadUtils::Sleep(ezTime::Milliseconds(1));

    ezUInt32 uiCanceled = 0;

    for (ezUInt32 i0 = uiNumTasks; i0 > 0; --i0)
    {
      const ezUInt32 i = i0 - 1;

      if (ezTaskSystem::CancelTask(&t[i], ezOnTaskRunning::ReturnWithoutBlocking) == EZ_SUCCESS)
        ++uiCanceled;
    }

    ezUInt32 uiDone = 0;
    ezUInt32 uiStarted = 0;

    for (int i = 0; i < uiNumTasks; ++i)
    {
      ezTaskSystem::WaitForGroup(tg[i]);
      EZ_TEST_BOOL(t[i].IsTaskFinished());

      if (t[i].IsDone())
        ++uiDone;
      if (t[i].IsStarted())
        ++uiStarted;
    }

    // at least one task should have run and thus be 'done'
    EZ_TEST_BOOL(uiDone > 0);
    EZ_TEST_BOOL(uiDone < uiNumTasks);

    EZ_TEST_BOOL(uiStarted > 0);
    EZ_TEST_BOOL_MSG(uiStarted <= ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::ShortTasks), "This test can fail when the PC is under heavy load."); // should not have managed to start more tasks than there are threads
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Canceling Tasks (forcefully)")
  {
    const ezUInt32 uiNumTasks = 20;
    ezTestTask t[uiNumTasks];
    ezTaskGroupID tg[uiNumTasks];

    for (int i = 0; i < uiNumTasks; ++i)
    {
      t[i].m_uiIterations = 50;
      t[i].m_bSupportCancel = true;

      tg[i] = ezTaskSystem::StartSingleTask(&t[i], ezTaskPriority::ThisFrame);
    }

    ezThreadUtils::Sleep(ezTime::Milliseconds(1));

    ezUInt32 uiCanceled = 0;

    for (int i = uiNumTasks - 1; i >= 0; --i)
    {
      if (ezTaskSystem::CancelTask(&t[i], ezOnTaskRunning::ReturnWithoutBlocking) == EZ_SUCCESS)
        ++uiCanceled;
    }

    ezUInt32 uiDone = 0;
    ezUInt32 uiStarted = 0;

    for (int i = 0; i < uiNumTasks; ++i)
    {
      ezTaskSystem::WaitForGroup(tg[i]);
      EZ_TEST_BOOL(t[i].IsTaskFinished());

      if (t[i].IsDone())
        ++uiDone;
      if (t[i].IsStarted())
        ++uiStarted;
    }

    // not a single thread should have finished the execution
    if (EZ_TEST_BOOL_MSG(uiDone == 0, "This test can fail when the PC is under heavy load.").Succeeded())
    {
      EZ_TEST_BOOL(uiStarted > 0);
      EZ_TEST_BOOL(uiStarted <= ezTaskSystem::GetNumAllocatedWorkerThreads(ezWorkerThreadType::ShortTasks)); // should not have managed to start more tasks than there are threads
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Canceling Group")
  {
    const ezUInt32 uiNumTasks = 4;
    ezTestTask t1[uiNumTasks];
    ezTestTask t2[uiNumTasks];

    ezTaskGroupID g1, g2;
    g1 = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame);
    g2 = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame);

    ezTaskSystem::AddTaskGroupDependency(g2, g1);

    for (ezUInt32 i = 0; i < uiNumTasks; ++i)
    {
      ezTaskSystem::AddTaskToGroup(g1, &t1[i]);
      ezTaskSystem::AddTaskToGroup(g2, &t2[i]);
    }

    ezTaskSystem::StartTaskGroup(g2);
    ezTaskSystem::StartTaskGroup(g1);

    ezThreadUtils::Sleep(ezTime::Milliseconds(10));

    EZ_TEST_BOOL(ezTaskSystem::CancelGroup(g2, ezOnTaskRunning::WaitTillFinished) == EZ_SUCCESS);

    for (int i = 0; i < uiNumTasks; ++i)
    {
      EZ_TEST_BOOL(!t2[i].IsDone());
      EZ_TEST_BOOL(t2[i].IsTaskFinished());
    }

    ezThreadUtils::Sleep(ezTime::Milliseconds(1));

    EZ_TEST_BOOL(ezTaskSystem::CancelGroup(g1, ezOnTaskRunning::WaitTillFinished) == EZ_FAILURE);

    for (int i = 0; i < uiNumTasks; ++i)
    {
      EZ_TEST_BOOL(!t2[i].IsDone());

      EZ_TEST_BOOL(t1[i].IsTaskFinished());
      EZ_TEST_BOOL(t2[i].IsTaskFinished());
    }

    ezThreadUtils::Sleep(ezTime::Milliseconds(100));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Tasks with Multiplicity")
  {
    ezTestTask t[3];
    ezTaskGroupID tg[3];

    t[0].SetTaskName("Task 0");
    t[1].SetTaskName("Task 1");
    t[2].SetTaskName("Task 2");

    t[0].SetMultiplicity(1);
    t[1].SetMultiplicity(100);
    t[2].SetMultiplicity(1000);

    tg[0] = ezTaskSystem::StartSingleTask(&t[0], ezTaskPriority::LateThisFrame);
    tg[1] = ezTaskSystem::StartSingleTask(&t[1], ezTaskPriority::ThisFrame);
    tg[2] = ezTaskSystem::StartSingleTask(&t[2], ezTaskPriority::EarlyThisFrame);

    ezTaskSystem::WaitForGroup(tg[0]);
    ezTaskSystem::WaitForGroup(tg[1]);
    ezTaskSystem::WaitForGroup(tg[2]);

    EZ_TEST_BOOL(t[0].IsMultiplicityDone());
    EZ_TEST_BOOL(t[1].IsMultiplicityDone());
    EZ_TEST_BOOL(t[2].IsMultiplicityDone());
  }

  // capture profiling info for testing
  /*ezStringBuilder sOutputPath = ezTestFramework::GetInstance()->GetAbsOutputPath();

  ezFileSystem::AddDataDirectory(sOutputPath.GetData());

  ezFileWriter fileWriter;
  if (fileWriter.Open("profiling.json") == EZ_SUCCESS)
  {
  ezProfilingSystem::Capture(fileWriter);
  }*/
}
