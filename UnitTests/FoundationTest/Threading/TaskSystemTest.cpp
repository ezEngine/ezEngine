#include <PCH.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>

class ezTestTask : public ezTask
{
public:

  ezUInt32 m_uiIterations;
  ezTestTask* m_pDependency;
  bool m_bSupportCancel;
  ezInt32 m_iTaskID;

  ezTestTask()
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

private:

  bool m_bStarted;
  bool m_bDone;
  
  virtual void Execute() override
  {
    if (m_iTaskID >= 0)
      printf("Starting Task %i at %.4f\n", m_iTaskID, ezTime::Now().GetSeconds());

    m_bStarted = true;

    EZ_TEST_BOOL(m_pDependency == nullptr || m_pDependency->IsTaskFinished());

    for (ezUInt32 obst = 0; obst < m_uiIterations; ++obst)
    {
      ezThreadUtils::Sleep(1);
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

void TaskFinished(ezTask* pTask, void* pPassThrough)
{
  ezAtomicInteger32* pInt = (ezAtomicInteger32*) pPassThrough;
  pInt->Increment();
}

void TaskGroupFinished(void* pPassThrough)
{
  ezAtomicInteger32* pInt = (ezAtomicInteger32*) pPassThrough;
  pInt->Increment();
}

EZ_CREATE_SIMPLE_TEST(Threading, TaskSystem)
{
  ezUInt32 uiWorkersShort = 4;
  ezUInt32 uiWorkersLong = 4;

  ezTaskSystem::SetWorkThreadCount(uiWorkersShort, uiWorkersLong);
  ezThreadUtils::Sleep(500);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Single Tasks")
  {
    ezTestTask t[3];

    t[0].SetTaskName("Task 0");
    t[1].SetTaskName("Task 1");
    t[2].SetTaskName("Task 2");
   
    ezTaskSystem::StartSingleTask(&t[0], ezTaskPriority::LateThisFrame);
    ezTaskSystem::StartSingleTask(&t[1], ezTaskPriority::ThisFrame);
    ezTaskSystem::StartSingleTask(&t[2], ezTaskPriority::EarlyThisFrame);

    ezTaskSystem::WaitForTask(&t[0]);
    ezTaskSystem::WaitForTask(&t[1]);
    ezTaskSystem::WaitForTask(&t[2]);

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

    ezTaskSystem::WaitForTask(&t[2]);
    ezTaskSystem::WaitForTask(&t[3]);

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

    g[0] = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame, TaskGroupFinished, &GroupsFinished);
    g[1] = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame, TaskGroupFinished, &GroupsFinished);
    g[2] = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame, TaskGroupFinished, &GroupsFinished);
    g[3] = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame, TaskGroupFinished, &GroupsFinished);

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

      t[i].SetOnTaskFinished(TaskFinished, &TasksFinished);
    }

    ezTaskSystem::StartTaskGroup(g[3]);
    ezTaskSystem::StartTaskGroup(g[2]);
    ezTaskSystem::StartTaskGroup(g[1]);
    ezTaskSystem::StartTaskGroup(g[0]);

    ezTaskSystem::WaitForGroup(g[3]);
    ezTaskSystem::WaitForGroup(g[2]);
    ezTaskSystem::WaitForGroup(g[1]);
    ezTaskSystem::WaitForGroup(g[0]);

    EZ_TEST_INT(TasksFinished, 8);
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
    const int Tasks = 20;
    ezTestTask t[Tasks];

    for (int i = 0; i < Tasks; i += 2)
    {
      t[i].m_uiIterations = 10;
      t[i + 1].m_uiIterations = 20;

      ezTaskSystem::StartSingleTask(&t[i], ezTaskPriority::ThisFrame);
      ezTaskSystem::StartSingleTask(&t[i + 1], ezTaskPriority::NextFrame);
    }

    ezTaskSystem::FinishFrameTasks();

    ezInt32 iNotAllThisTasksFinished = 0;

    for (int i = 0; i < Tasks; i += 2)
    {
      if (!t[i].IsTaskFinished())
        ++iNotAllThisTasksFinished;
    }

    // up to the number of worker threads tasks can be still active
    EZ_TEST_BOOL(iNotAllThisTasksFinished <= (ezInt32) uiWorkersShort);

    ezInt32 iNotAllNextTasksFinished = 0;

    for (int i = 0; i < Tasks; i += 2)
    {
      //ezTaskSystem::WaitForTask(&t[i]);
      //EZ_TEST_BOOL(t[i].IsTaskFinished());

      if (!t[i + 1].IsTaskFinished())
        ++iNotAllNextTasksFinished;
    }

    EZ_TEST_BOOL_MSG(iNotAllNextTasksFinished > 0, "This test CAN fail, if the PC is blocked just right. It does not matter though, it is not really a failure.");

    ezTaskSystem::FinishFrameTasks();

    for (int i = 0; i < Tasks; i += 2)
    {
      EZ_TEST_BOOL(t[i].IsTaskFinished());

      ezTaskSystem::WaitForTask(&t[i + 1]);

      EZ_TEST_BOOL(t[i + 1].IsTaskFinished());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Main Thread Tasks")
  {
    const int Tasks = 20;
    ezTestTask t[Tasks];

    for (int i = 0; i < Tasks; ++i)
    {
      t[i].m_uiIterations = 10;

      ezTaskSystem::StartSingleTask(&t[i], ezTaskPriority::ThisFrameMainThread);
    }

    ezTaskSystem::FinishFrameTasks();

    for (int i = 0; i < Tasks; ++i)
    {
      EZ_TEST_BOOL(t[i].IsTaskFinished());
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Canceling Tasks")
  {
    const int Tasks = 20;
    ezTestTask t[Tasks];

    for (int i = 0; i < Tasks; ++i)
    {
      t[i].m_uiIterations = 50;

      ezTaskSystem::StartSingleTask(&t[i], ezTaskPriority::ThisFrame);
    }

    ezThreadUtils::Sleep(1);

    ezInt32 iCanceled = 0;

    for (int i = Tasks - 1; i >= 0; --i)
    {
      if (ezTaskSystem::CancelTask(&t[i], ezOnTaskRunning::ReturnWithoutBlocking) == EZ_SUCCESS)
        ++iCanceled;
    }

    ezInt32 iDone = 0;
    ezInt32 iStarted = 0;

    for (int i = 0; i < Tasks; ++i)
    {
      ezTaskSystem::WaitForTask(&t[i]);
      EZ_TEST_BOOL(t[i].IsTaskFinished());

      if (t[i].IsDone())
        ++iDone;
      if (t[i].IsStarted())
        ++iStarted;
    }

    // at least one task should have run and thus be 'done'
    EZ_TEST_BOOL(iDone > 0);
    EZ_TEST_BOOL(iDone < Tasks);

    EZ_TEST_BOOL(iStarted > 0);
    EZ_TEST_BOOL_MSG(iStarted <= 4, "This test can fail when the PC is under heavy load."); // should not have managed to start more tasks than there are threads
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Canceling Tasks (forcefully)")
  {
    const int Tasks = 20;
    ezTestTask t[Tasks];

    for (int i = 0; i < Tasks; ++i)
    {
      t[i].m_uiIterations = 50;
      t[i].m_bSupportCancel = true;

      ezTaskSystem::StartSingleTask(&t[i], ezTaskPriority::ThisFrame);
    }

    ezThreadUtils::Sleep(1);

    ezInt32 iCanceled = 0;

    for (int i = Tasks - 1; i >= 0; --i)
    {
      if (ezTaskSystem::CancelTask(&t[i], ezOnTaskRunning::ReturnWithoutBlocking) == EZ_SUCCESS)
        ++iCanceled;
    }

    ezInt32 iDone = 0;
    ezInt32 iStarted = 0;

    for (int i = 0; i < Tasks; ++i)
    {
      ezTaskSystem::WaitForTask(&t[i]);
      EZ_TEST_BOOL(t[i].IsTaskFinished());

      if (t[i].IsDone())
        ++iDone;
      if (t[i].IsStarted())
        ++iStarted;
    }

    // not a single thread should have finished the execution
    EZ_TEST_BOOL(iDone == 0);
    EZ_TEST_BOOL(iStarted > 0);
    EZ_TEST_BOOL(iStarted <= 4); // should not have managed to start more tasks than there are threads
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Canceling Group")
  {
    const int Tasks = 4;
    ezTestTask t1[Tasks];
    ezTestTask t2[Tasks];

    ezTaskGroupID g1, g2;
    g1 = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame);
    g2 = ezTaskSystem::CreateTaskGroup(ezTaskPriority::ThisFrame);

    ezTaskSystem::AddTaskGroupDependency(g2, g1);

    for (int i = 0; i < Tasks; ++i)
    {
      //t1[i].m_iTaskID = i;
      //t2[i].m_iTaskID = Tasks + i;

      ezTaskSystem::AddTaskToGroup(g1, &t1[i]);
      ezTaskSystem::AddTaskToGroup(g2, &t2[i]);
    }

    ezTaskSystem::StartTaskGroup(g2);
    ezTaskSystem::StartTaskGroup(g1);

    ezThreadUtils::Sleep(10);

    EZ_TEST_BOOL(ezTaskSystem::CancelGroup(g2, ezOnTaskRunning::WaitTillFinished) == EZ_SUCCESS);

    for (int i = 0; i < Tasks; ++i)
    {
      EZ_TEST_BOOL(!t2[i].IsDone());
      EZ_TEST_BOOL(t2[i].IsTaskFinished());
    }

    ezThreadUtils::Sleep(1);

    EZ_TEST_BOOL(ezTaskSystem::CancelGroup(g1, ezOnTaskRunning::WaitTillFinished) == EZ_FAILURE);

    for (int i = 0; i < Tasks; ++i)
    {
      EZ_TEST_BOOL(!t2[i].IsDone());

      EZ_TEST_BOOL(t1[i].IsTaskFinished());
      EZ_TEST_BOOL(t2[i].IsTaskFinished());
    }

    ezThreadUtils::Sleep(100);
  }

  // capture profiling info for testing
  /*ezStringBuilder sOutputPath = BUILDSYSTEM_OUTPUT_FOLDER;
  sOutputPath.AppendPath("FoundationTest");

  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory(sOutputPath.GetData());

  ezFileWriter fileWriter;
  if (fileWriter.Open("profiling.json") == EZ_SUCCESS)
  {
    ezProfilingSystem::Capture(fileWriter);
  }*/
}
