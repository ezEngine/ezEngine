#include "VHACD.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>
#include <float.h>

#define ENABLE_ASYNC 1

#define HACD_ALLOC(x) malloc(x)
#define HACD_FREE(x) free(x)
#define HACD_ASSERT(x) assert(x)

namespace VHACD
{

class MyHACD_API : public VHACD::IVHACD,
                   public VHACD::IVHACD::IUserCallback,
                   VHACD::IVHACD::IUserLogger,
                   VHACD::IVHACD::IUserTaskRunner
{
public:
    MyHACD_API(void)
    {
        mVHACD = VHACD::CreateVHACD();
    }

    virtual ~MyHACD_API(void)
    {
        releaseHACD();
        Cancel();
        mVHACD->Release();
    }

    virtual void* StartTask(std::function<void()> func) override
    {
        return new std::thread(func);
    }

    virtual void JoinTask(void* Task) override
    {
        std::thread* t = (std::thread*)Task;
        t->join();
        delete t;
    }

    virtual bool Compute(const double* const _points,
                         const uint32_t countPoints,
                         const uint32_t* const _triangles,
                         const uint32_t countTriangles,
                         const Parameters& _desc) final
    {
#if ENABLE_ASYNC
        VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(_desc.m_profiler, MyHACD_API::Compute)

        Cancel(); // if we previously had a solution running; cancel it.
        releaseHACD();

        Parameters desc = _desc;
        mTaskRunner = _desc.m_taskRunner ? _desc.m_taskRunner : this;
        desc.m_taskRunner = mTaskRunner;

        // We need to copy the input vertices and triangles into our own buffers so we can operate
        // on them safely from the background thread.
        mVertices = (double*)HACD_ALLOC(sizeof(double) * countPoints * 3);
        mIndices = (uint32_t*)HACD_ALLOC(sizeof(uint32_t) * countTriangles * 3);
        memcpy(mVertices, _points, sizeof(double) * countPoints * 3);
        memcpy(mIndices, _triangles, sizeof(uint32_t) * countTriangles * 3);
        mRunning = true;
        mTask = mTaskRunner->StartTask([this, countPoints, countTriangles, desc]() {
            // printf("Computing V-HACD in background task.\n");
            ComputeNow(mVertices, countPoints, mIndices, countTriangles, desc);
            // printf("V-HACD computation complete.\n");
            // If we have a user provided callback and the user did *not* call 'cancel' we notify him that the
            // task is completed. However..if the user selected 'cancel' we do not send a completed notification event.
            if (desc.m_callback && !mCancel)
            {
                // printf("Doing callback notification\n");
                desc.m_callback->NotifyVHACDComplete();
                // printf("Notify callback complete.\n");
            }
            mRunning = false;
            // printf("Exiting compute V-HACD task.\n");
        });
#else
        releaseHACD();
        ComputeNow(_points, countPoints, _triangles, countTriangles, desc);
#endif
        return true;
    }

    bool ComputeNow(const double* const points,
                    const uint32_t countPoints,
                    const uint32_t* const triangles,
                    const uint32_t countTriangles,
                    const Parameters& _desc)
    {
        VHACD_TRACE_CPUPROFILER_EVENT_SCOPE(_desc.m_profiler, MyHACD_API::ComputeNow);
        uint32_t ret = 0;

        Parameters desc;
        mHullCount = 0;
        mCallback = _desc.m_callback;
        mLogger = _desc.m_logger;

        desc = _desc;
        // Set our intercepting callback interfaces if non-null
        desc.m_callback = _desc.m_callback ? this : nullptr;
        desc.m_logger = _desc.m_logger ? this : nullptr;

        // If not task runner provided, then use the default one
        if (desc.m_taskRunner == nullptr)
        {
            desc.m_taskRunner = this;
        }

        if (countPoints)
        {
            bool ok = mVHACD->Compute(points, countPoints, triangles, countTriangles, desc);
            if (ok)
            {
                ret = mVHACD->GetNConvexHulls();
                mHulls = new IVHACD::ConvexHull[ret];
                for (uint32_t i = 0; i < ret; i++)
                {
                    VHACD::IVHACD::ConvexHull vhull;
                    mVHACD->GetConvexHull(i, vhull);
                    VHACD::IVHACD::ConvexHull h;
                    h.m_nPoints = vhull.m_nPoints;
                    h.m_points = (double*)HACD_ALLOC(sizeof(double) * 3 * h.m_nPoints);
                    memcpy(h.m_points, vhull.m_points, sizeof(double) * 3 * h.m_nPoints);
                    h.m_nTriangles = vhull.m_nTriangles;
                    h.m_triangles = (uint32_t*)HACD_ALLOC(sizeof(uint32_t) * 3 * h.m_nTriangles);
                    memcpy(h.m_triangles, vhull.m_triangles, sizeof(uint32_t) * 3 * h.m_nTriangles);
                    h.m_volume = vhull.m_volume;
                    h.m_center[0] = vhull.m_center[0];
                    h.m_center[1] = vhull.m_center[1];
                    h.m_center[2] = vhull.m_center[2];
                    mHulls[i] = h;
                    if (mCancel)
                    {
                        ret = 0;
                        break;
                    }
                }
            }
        }

        mHullCount = ret;


        return ret ? true : false;
    }

    void releaseHull(VHACD::IVHACD::ConvexHull& h)
    {
        HACD_FREE((void*)h.m_triangles);
        HACD_FREE((void*)h.m_points);
        h.m_triangles = nullptr;
        h.m_points = nullptr;
    }

    virtual void GetConvexHull(const uint32_t index, VHACD::IVHACD::ConvexHull& ch) const final
    {
        if (index < mHullCount)
        {
            ch = mHulls[index];
        }
    }

    void releaseHACD(void) // release memory associated with the last HACD request
    {
        for (uint32_t i = 0; i < mHullCount; i++)
        {
            releaseHull(mHulls[i]);
        }
        delete[] mHulls;
        mHulls = nullptr;
        mHullCount = 0;
        HACD_FREE(mVertices);
        mVertices = nullptr;
        HACD_FREE(mIndices);
        mIndices = nullptr;
    }


    virtual void release(void) // release the HACD_API interface
    {
        delete this;
    }

    virtual uint32_t getHullCount(void)
    {
        return mHullCount;
    }

    virtual void Cancel() final
    {
        // printf("Entered Cancel\n");
        mCancel = true;
        if (mVHACD)
        {
            // printf("Setting cancel on the V-HACD instance\n");
            mVHACD->Cancel(); // Set the cancel signal to the base VHACD
        }
        if (mTask)
        {
            // printf("JoinTask waiting for task to complete.\n");
            mTaskRunner->JoinTask(mTask); // Wait for the thread to fully exit before we delete the instance
            mTask = nullptr;
        }
        // printf("Leaving Cancel routine\n");
        mCancel = false; // clear the cancel semaphore
    }

    virtual bool Compute(const float* const points,
                         const uint32_t countPoints,
                         const uint32_t* const triangles,
                         const uint32_t countTriangles,
                         const Parameters& params) final
    {

        double* vertices = (double*)HACD_ALLOC(sizeof(double) * countPoints * 3);
        const float* source = points;
        double* dest = vertices;
        for (uint32_t i = 0; i < countPoints; i++)
        {
            dest[0] = source[0];
            dest[1] = source[1];
            dest[2] = source[2];
            dest += 3;
            source += 3;
        }

        bool ret = Compute(vertices, countPoints, triangles, countTriangles, params);
        HACD_FREE(vertices);
        return ret;
    }

    virtual uint32_t GetNConvexHulls() const final
    {
        processPendingMessages();
        return mHullCount;
    }

    virtual void Clean(void) final // release internally allocated memory
    {
        Cancel();
        releaseHACD();
        mVHACD->Clean();
    }

    virtual void Release(void) final // release IVHACD
    {
        delete this;
    }

    virtual void Update(const double overallProgress,
                        const double stageProgress,
                        const double operationProgress,
                        const char* const stage,
                        const char* const operation) final
    {
        mMessageMutex.lock();
        mHaveUpdateMessage = true;
        mOverallProgress = overallProgress;
        mStageProgress = stageProgress;
        mOperationProgress = operationProgress;
        mStage = std::string(stage);
        mOperation = std::string(operation);
        mMessageMutex.unlock();
    }

    virtual void Log(const char* const msg) final
    {
        mMessageMutex.lock();
        mHaveLogMessage = true;
        mMessage = std::string(msg);
        mMessageMutex.unlock();
    }

    virtual bool IsReady(void) const final
    {
        processPendingMessages();
        return !mRunning;
    }

    // As a convenience for the calling application we only send it update and log messages from it's own main
    // thread.  This reduces the complexity burden on the caller by making sure it only has to deal with log
    // messages in it's main application thread.
    void processPendingMessages(void) const
    {
        if (mCancel)
        {
            return;
        }
        // If we have a new update message and the user has specified a callback we send the message and clear the
        // semaphore
        if (mHaveUpdateMessage && mCallback)
        {
            mMessageMutex.lock();
            mCallback->Update(mOverallProgress, mStageProgress, mOperationProgress, mStage.c_str(), mOperation.c_str());
            mHaveUpdateMessage = false;
            mMessageMutex.unlock();
        }
        // If we have a new log message and the user has specified a callback we send the message and clear the
        // semaphore
        if (mHaveLogMessage && mLogger)
        {
            mMessageMutex.lock();
            mLogger->Log(mMessage.c_str());
            mHaveLogMessage = false;
            mMessageMutex.unlock();
        }
    }

    // Will compute the center of mass of the convex hull decomposition results and return it
    // in 'centerOfMass'.  Returns false if the center of mass could not be computed.
    virtual bool ComputeCenterOfMass(double centerOfMass[3]) const override
    {
        bool ret = false;

        centerOfMass[0] = 0;
        centerOfMass[1] = 0;
        centerOfMass[2] = 0;

        if (mVHACD && IsReady())
        {
            ret = mVHACD->ComputeCenterOfMass(centerOfMass);
        }
        return ret;
    }

private:
    double* mVertices{ nullptr };
    uint32_t* mIndices{ nullptr };
    std::atomic<uint32_t> mHullCount{ 0 };
    VHACD::IVHACD::ConvexHull* mHulls{ nullptr };
    VHACD::IVHACD::IUserCallback* mCallback{ nullptr };
    VHACD::IVHACD::IUserLogger* mLogger{ nullptr };
    VHACD::IVHACD* mVHACD{ nullptr };
    VHACD::IVHACD::IUserTaskRunner* mTaskRunner{ nullptr };
    void* mTask{ nullptr };
    std::atomic<bool> mRunning{ false };
    std::atomic<bool> mCancel{ false };

    // Thread safe caching mechanism for messages and update status.
    // This is so that caller always gets messages in his own thread
    // Member variables are marked as 'mutable' since the message dispatch function
    // is called from const query methods.
    mutable std::mutex mMessageMutex;
    mutable std::atomic<bool> mHaveUpdateMessage{ false };
    mutable std::atomic<bool> mHaveLogMessage{ false };
    mutable double mOverallProgress{ 0 };
    mutable double mStageProgress{ 0 };
    mutable double mOperationProgress{ 0 };
    mutable std::string mStage;
    mutable std::string mOperation;
    mutable std::string mMessage;
};

IVHACD* CreateVHACD_ASYNC(void)
{
    MyHACD_API* m = new MyHACD_API;
    return static_cast<IVHACD*>(m);
}


}; // namespace VHACD
