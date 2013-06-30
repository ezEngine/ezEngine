#include <PCH.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Time.h>

class ezTestTask : public ezTask
{
public:

  ezUInt32 m_uiIterations;
  ezTestTask* m_pDependency;

  ezTestTask()
  {
    m_uiIterations = 50;
    m_pDependency = NULL;
    m_bDone = false;
  }

  bool IsDone() const { return m_bDone; }

private:

  bool m_bDone;
  
  virtual void Execute (void) EZ_OVERRIDE
  {
    EZ_TEST(m_pDependency == NULL || m_pDependency->IsTaskFinished());

    for (ezUInt32 obst = 0; obst < m_uiIterations; ++obst)
    {
      ezThreadUtils::Sleep(1);
    }

    m_bDone = true;
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
  //ezStringBuilder sOutputFolder1 = BUILDSYSTEM_OUTPUT_FOLDER;
  //sOutputFolder1.AppendPath("FoundationTest");

  //ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  //EZ_TEST(ezFileSystem::AddDataDirectory(sOutputFolder1.GetData(), ezFileSystem::AllowWrites, "test") == EZ_SUCCESS);

  ezUInt32 uiWorkersShort = 4;
  ezUInt32 uiWorkersLong = 4;

  ezTaskSystem::SetWorkThreadCount(uiWorkersShort, uiWorkersLong);
  ezThreadUtils::Sleep(500);

  EZ_TEST_BLOCK(true, "Single Tasks")
  {
    ezTestTask t[3];
   
    ezTaskSystem::StartSingleTask(&t[0], ezTaskPriority::LateThisFrame);
    ezTaskSystem::StartSingleTask(&t[1], ezTaskPriority::ThisFrame);
    ezTaskSystem::StartSingleTask(&t[2], ezTaskPriority::EarlyThisFrame);

    ezTaskSystem::WaitForTask(&t[0]);
    ezTaskSystem::WaitForTask(&t[1]);
    ezTaskSystem::WaitForTask(&t[2]);

    EZ_TEST(t[0].IsDone());
    EZ_TEST(t[1].IsDone());
    EZ_TEST(t[2].IsDone());
  }

  EZ_TEST_BLOCK(true, "Single Tasks with Dependencies")
  {
    ezTestTask t[4];
    ezTaskGroupID g[4];
   
    g[0] = ezTaskSystem::StartSingleTask(&t[0], ezTaskPriority::LateThisFrame);
    g[1] = ezTaskSystem::StartSingleTask(&t[1], ezTaskPriority::ThisFrame, g[0]);
    g[2] = ezTaskSystem::StartSingleTask(&t[2], ezTaskPriority::EarlyThisFrame, g[1]);
    g[3] = ezTaskSystem::StartSingleTask(&t[3], ezTaskPriority::EarlyThisFrame, g[0]);

    ezTaskSystem::WaitForTask(&t[2]);
    ezTaskSystem::WaitForTask(&t[3]);

    EZ_TEST(t[0].IsDone());
    EZ_TEST(t[1].IsDone());
    EZ_TEST(t[2].IsDone());
    EZ_TEST(t[3].IsDone());
  }

  EZ_TEST_BLOCK(true, "Grouped Tasks / TaskFinished Callback / GroupFinished Callback")
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
      EZ_TEST(!ezTaskSystem::IsTaskGroupFinished(g[i]));

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
      EZ_TEST(!t[i].IsTaskFinished());
      EZ_TEST(!t[i].IsDone());

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

    EZ_TEST(TasksFinished == 8);
    EZ_TEST(GroupsFinished == 4);

    for (int i = 0; i < 4; ++i)
      EZ_TEST(ezTaskSystem::IsTaskGroupFinished(g[i]));

    for (int i = 0; i < 8; ++i)
    {
      EZ_TEST(t[i].IsTaskFinished());
      EZ_TEST(t[i].IsDone());
    }
  }

  EZ_TEST_BLOCK(true, "This Frame Tasks / Next Frame Tasks")
  {
    const int Tasks = 20;
    ezTestTask t[Tasks];
    //ezTestTask tm[2];

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
    EZ_TEST(iNotAllThisTasksFinished <= (ezInt32) uiWorkersShort);

    ezInt32 iNotAllNextTasksFinished = 0;

    for (int i = 0; i < Tasks; i += 2)
    {
      //ezTaskSystem::WaitForTask(&t[i]);
      //EZ_TEST(t[i].IsTaskFinished());

      if (!t[i + 1].IsTaskFinished())
        ++iNotAllNextTasksFinished;
    }

    EZ_TEST(iNotAllNextTasksFinished > 0);

    ezTaskSystem::FinishFrameTasks();

    for (int i = 0; i < Tasks; i += 2)
    {
      EZ_TEST(t[i].IsTaskFinished());

      ezTaskSystem::WaitForTask(&t[i + 1]);

      EZ_TEST(t[i + 1].IsTaskFinished());
    }
  }

  // frame tasks / next frame tasks
  // long running tasks
  // cancel task
  // cancel group
  // all task priorities

//  //// cancel all these tasks, but don't wait for them
//  //for (ezUInt32 i = g_uiTasks/2; i < g_uiTasks; ++i)
//  //  ezTaskSystem::CancelGroup(g_PathSearchGroups[i], false);
//
//  //// try again, this time wait for them
//  //for (ezUInt32 i = g_uiTasks/2; i < g_uiTasks; ++i)
//  //  ezTaskSystem::CancelTask(&g_PathSearches[i].GetStatic(), true);
//
// finishes the grid creators
//  //ezTaskSystem::FinishFrameTasks();
//
//  // finishes the path searches
//  //ezTaskSystem::FinishFrameTasks();
}
