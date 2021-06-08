#include "SimpleJobSystem.h"
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>

#ifdef _MSC_VER
#    pragma warning(disable : 4100)
#endif

// Scoped mutex lock
using lock_guard = std::lock_guard<std::mutex>;

namespace simplejobsystem
{


class SimpleJob
{
public:
    SimpleJob(void* userPtr, SJS_jobCallback callback) : mUserPointer(userPtr), mCallback(callback)
    {
    }

    void execute(void)
    {
        (mCallback)(mUserPointer);
    }

    void* mUserPointer{ nullptr };
    SJS_jobCallback mCallback;
};

typedef std::queue<SimpleJob*> SimpleJobQueue;

class FindJobs
{
public:
    virtual SimpleJob* getSimpleJob(void) = 0;
    virtual void simpleJobComplete(SimpleJob* sj) = 0;
};

class SimpleJobThread
{
public:
    SimpleJobThread(void)
    {
    }

    ~SimpleJobThread(void)
    {
        stopThread();
    }

    void stopThread(void)
    {
        if (mTaskRunnerThread)
        {
            mExit = true;
            mHaveWork.notify_one();
            mTaskRunner->JoinTask(mTaskRunnerThread);
            mTaskRunnerThread = nullptr;
        }
        else if (mThread)
        {
            mExit = true;
            mHaveWork.notify_one();
            mThread->join();
            delete mThread;
            mThread = nullptr;
        }
    }

    void start(FindJobs* fj, VHACD::IVHACD::IUserTaskRunner* taskRunner)
    {
        mTaskRunner = taskRunner;
        mFindJobs = fj;
        if (mTaskRunner)
        {
            if (mTaskRunnerThread == nullptr)
            {
                mExit = false;
                mTaskRunnerThread = mTaskRunner->StartTask([this]() { runThread(); });
            }
        }
        else
        {
            if (mThread == nullptr)
            {
                mThread = new std::thread([this]() { runThread(); });
            }
        }
        mHaveWork.notify_all();
    }

    void runThread(void)
    {
        while (!mExit)
        {
            // Process jobs while jobs are available.
            // If no more jobs are available, go to sleep until fresh work is provided
            SimpleJob* sj = mFindJobs->getSimpleJob();
            while (sj)
            {
                sj->execute();
                mFindJobs->simpleJobComplete(sj);
                sj = mFindJobs->getSimpleJob();
            }
            {
                std::unique_lock<std::mutex> lock(mWorkMutex);
                mHaveWork.wait(lock);
            }
        }
    }


    FindJobs* mFindJobs{ nullptr };
    std::atomic<bool> mExit{ false };
    void* mTaskRunnerThread{ nullptr };
    std::thread* mThread{ nullptr };
    std::mutex mWorkMutex;
    std::condition_variable mHaveWork;
    VHACD::IVHACD::IUserTaskRunner* mTaskRunner{ nullptr };
};

class SimpleJobSystemImpl : public SimpleJobSystem, public FindJobs
{
public:
    SimpleJobSystemImpl(uint32_t maxThreads, VHACD::IVHACD::IUserTaskRunner* taskRunner)
        : mMaxThreads(maxThreads), mTaskRunner(taskRunner)
    {
        mThreads = new SimpleJobThread[mMaxThreads];
    }

    virtual ~SimpleJobSystemImpl(void)
    {
        waitForJobsToComplete();
        delete[] mThreads;
    }

    // Add a job to the queue of jobs to be performed, does not actually start the job yet.
    virtual void addJob(void* userPtr, SJS_jobCallback callback) final
    {
        lock_guard _lock(mSimpleJobMutex);
        SimpleJob* sj = new SimpleJob(userPtr, callback);
        mPendingJobs.push(sj);
    }

    // Start the jobs that have been posted, returns how many jobs are pending.
    virtual uint32_t startJobs(void) final
    {
        lock_guard _lock(mSimpleJobMutex);
        mPendingJobCount += uint32_t(mPendingJobs.size()); // Set the number of pending jobs counter.
        uint32_t ret = mPendingJobCount;
        // Start all threads working on the available jobs
        for (uint32_t i = 0; i < mMaxThreads; i++)
        {
            mThreads[i].start(this, mTaskRunner);
        }
        return ret;
    }

    // Sleeps until all of the pending jobs have completed.
    virtual void waitForJobsToComplete(void) final
    {
        if (mPendingJobCount == 0)
        {
            return;
        }
        std::unique_lock<std::mutex> lock(mWorkCompleteMutex);
        mWorkComplete.wait(lock);
    }

    // Releases the SimpleJobSystem instance
    virtual void release(void) final
    {
        delete this;
    }

    virtual SimpleJob* getSimpleJob(void) final
    {
        SimpleJob* ret = nullptr;
        lock_guard _lock(mSimpleJobMutex);
        if (!mPendingJobs.empty())
        {
            ret = mPendingJobs.front();
            mPendingJobs.pop();
        }
        return ret;
    }

    // This job is complete, delete the pointer and decrement
    // the total pending job count. If it goes to zero, then raise the 'mHaveWork'
    virtual void simpleJobComplete(SimpleJob* sj) final
    {
        delete sj;
        mPendingJobCount--;
        if (mPendingJobCount == 0)
        {
            mWorkComplete.notify_all(); // wake up when job count goes to zero
        }
    }

    std::atomic<uint32_t> mPendingJobCount{ 0 };
    std::mutex mWorkCompleteMutex;
    std::condition_variable mWorkComplete;
    uint32_t mMaxThreads{ 8 };
    std::mutex mSimpleJobMutex;
    SimpleJobQueue mPendingJobs;
    SimpleJobThread* mThreads{ nullptr };
    VHACD::IVHACD::IUserTaskRunner* mTaskRunner{ nullptr };
};

// Create in instance of the SimpleJobSystem with the number of threads specified.
// More threads than available cores is not particularly beneficial.
SimpleJobSystem* SimpleJobSystem::create(uint32_t maxThreads, VHACD::IVHACD::IUserTaskRunner* taskRunner)
{
    auto ret = new SimpleJobSystemImpl(maxThreads, taskRunner);
    return static_cast<SimpleJobSystem*>(ret);
}


} // namespace simplejobsystem
