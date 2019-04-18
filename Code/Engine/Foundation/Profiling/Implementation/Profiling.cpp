#include <FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Communication/DataTransfer.h>
#include <Types/UniquePtr.h>

#if EZ_ENABLED(EZ_USE_PROFILING)

ezStaticRingBuffer<ezTime, ezProfilingSystem::RING_BUFFER_SIZE_FRAMES> ezProfilingSystem::s_FrameStartTimes;
ezUInt64 ezProfilingSystem::s_uiFrameCount = 0;

class ezProfileCaptureDataTransfer : public ezDataTransfer
{
private:
  virtual void OnTransferRequest() override
  {
    ezDataTransferObject dto(*this, "Capture", "application/json", "json");

    ezProfilingSystem::Capture(dto.GetWriter());

    dto.Transmit();
  }

};

static ezProfileCaptureDataTransfer s_ProfileCaptureDataTransfer;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ProfilingSystem)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    ezProfilingSystem::Initialize();
    s_ProfileCaptureDataTransfer.EnableDataTransfer("Profiling Capture");
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_ProfileCaptureDataTransfer.DisableDataTransfer();
    ezProfilingSystem::Reset();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

// Include inline file
#include <Foundation/Profiling/Implementation/Profiling_EZ_inl.h>


// static
void ezProfilingSystem::Initialize()
{
  SetThreadName("Main Thread");
}

// static
void ezProfilingSystem::StartNewFrame()
{
  ++s_uiFrameCount;

  if (!s_FrameStartTimes.CanAppend())
  {
    s_FrameStartTimes.PopFront();
  }

  s_FrameStartTimes.PushBack(ezTime::Now());
}

#else

// static
void ezProfilingSystem::SetThreadName(const char* szThreadName) {}

void ezProfilingSystem::Initialize() {}

void ezProfilingSystem::Reset() {}

void ezProfilingSystem::Capture(ezStreamWriter& outputStream) {}

void ezProfilingSystem::RemoveThread() {}

void ezProfilingSystem::StartNewFrame() {}

void ezProfilingSystem::InitializeGPUData() {}

static ezProfilingSystem::GPUData s_Dummy;
ezProfilingSystem::GPUData& ezProfilingSystem::AllocateGPUData()
{
  return s_Dummy;
}


#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Profiling_Implementation_Profiling);

