#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Factory.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <stdarg.h>

#ifdef JPH_DEBUG_RENDERER
std::unique_ptr<ezJoltDebugRenderer> ezJoltCore::s_pDebugRenderer;
#endif

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezJoltSteppingMode, 1)
  EZ_ENUM_CONSTANTS(ezJoltSteppingMode::Variable, ezJoltSteppingMode::Fixed, ezJoltSteppingMode::SemiFixed)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_BITFLAGS(ezOnJoltContact, 1)
  //EZ_BITFLAGS_CONSTANT(ezOnJoltContact::SendReportMsg),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::ImpactReactions),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::SlideReactions),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::RollXReactions),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::RollYReactions),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::RollZReactions),
EZ_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

ezJoltMaterial* ezJoltCore::s_pDefaultMaterial = nullptr;
std::unique_ptr<JPH::JobSystem> ezJoltCore::s_pJobSystem;
ezUniquePtr<ezProxyAllocator> ezJoltCore::s_pAllocator;
ezUniquePtr<ezProxyAllocator> ezJoltCore::s_pAllocatorAligned;

ezHashTable<void*, size_t, ezHashHelper<void*>, ezStaticsAllocatorWrapper> ezJoltCore::s_AllocSizes;

ezJoltMaterial::ezJoltMaterial() = default;
ezJoltMaterial::~ezJoltMaterial() = default;

static void JoltTraceFunc(const char* szText, ...)
{
  ezStringBuilder tmp;

  va_list args;
  va_start(args, szText);
  tmp.SetPrintfArgs(szText, args);
  va_end(args);

  ezLog::Dev("Jolt: {}", tmp);
}

#ifdef JPH_ENABLE_ASSERTS

static bool JoltAssertFailed(const char* szInExpression, const char* szInMessage, const char* szInFile, uint32_t inLine)
{
  return ezFailedCheck(szInFile, inLine, "Jolt", szInExpression, szInMessage);
};

#endif // JPH_ENABLE_ASSERTS

void ezJoltCore::DebugDraw(ezWorld* pWorld)
{
#ifdef JPH_DEBUG_RENDERER
  if (s_pDebugRenderer == nullptr)
    return;

  ezDebugRenderer::DrawSolidTriangles(pWorld, s_pDebugRenderer->m_Triangles, ezColor::White);
  ezDebugRenderer::DrawLines(pWorld, s_pDebugRenderer->m_Lines, ezColor::White);

  s_pDebugRenderer->m_Triangles.Clear();
  s_pDebugRenderer->m_Lines.Clear();
#endif
}

ezMutex s_AllocSizeMutex;

void* ezJoltCore::JoltMalloc(size_t inSize)
{
  void* ptr = ezJoltCore::s_pAllocator->Allocate(inSize, 8);

  EZ_LOCK(s_AllocSizeMutex);
  s_AllocSizes[ptr] = inSize;
  return ptr;
}

void ezJoltCore::JoltFree(void* inBlock)
{
  EZ_ASSERT_DEBUG(inBlock != nullptr, "");
  ezJoltCore::s_pAllocator->Deallocate(inBlock);
}

void* ezJoltCore::JoltReallocate(void* inBlock, size_t inSize)
{
  if (inBlock == nullptr)
  {
    return JoltMalloc(inSize);
  }
  else
  {
    void* ptr = ezJoltCore::s_pAllocator->Reallocate(inBlock, s_AllocSizes[inBlock], inSize, 8);

    EZ_LOCK(s_AllocSizeMutex);
    s_AllocSizes[ptr] = inSize;
    return ptr;
  }
}

void* ezJoltCore::JoltAlignedMalloc(size_t inSize, size_t inAlignment)
{
  return ezJoltCore::s_pAllocatorAligned->Allocate(inSize, inAlignment);
}

void ezJoltCore::JoltAlignedFree(void* inBlock)
{
  ezJoltCore::s_pAllocatorAligned->Deallocate(inBlock);
}

void ezJoltCore::Startup()
{
  s_pAllocator = EZ_DEFAULT_NEW(ezProxyAllocator, "Jolt-Core", ezFoundation::GetDefaultAllocator());
  s_pAllocatorAligned = EZ_DEFAULT_NEW(ezProxyAllocator, "Jolt-Core-Aligned", ezFoundation::GetAlignedAllocator());

  JPH::Trace = JoltTraceFunc;
  JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed);
  JPH::Allocate = ezJoltCore::JoltMalloc;
  JPH::Free = ezJoltCore::JoltFree;
  JPH::Reallocate = ezJoltCore::JoltReallocate;
  JPH::AlignedAllocate = ezJoltCore::JoltAlignedMalloc;
  JPH::AlignedFree = ezJoltCore::JoltAlignedFree;

  JPH::Factory::sInstance = new JPH::Factory();

  JPH::RegisterTypes();

  ezJoltCustomShapeInfo::sRegister();

  // TODO: custom job system
  s_pJobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

  s_pDefaultMaterial = new ezJoltMaterial;
  s_pDefaultMaterial->AddRef();
  JPH::PhysicsMaterial::sDefault = s_pDefaultMaterial;

#ifdef JPH_DEBUG_RENDERER
  s_pDebugRenderer = std::make_unique<ezJoltDebugRenderer>();
#endif

  ezSurfaceResource::s_Events.AddEventHandler(&ezJoltCore::SurfaceResourceEventHandler);
}

void ezJoltCore::Shutdown()
{
#ifdef JPH_DEBUG_RENDERER
  s_pDebugRenderer = nullptr;
#endif

  JPH::PhysicsMaterial::sDefault = nullptr;

  s_pDefaultMaterial->Release();
  s_pDefaultMaterial = nullptr;

  s_pJobSystem = nullptr;

  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;

  JPH::Trace = nullptr;

  s_pAllocator.Clear();
  s_pAllocatorAligned.Clear();

  ezSurfaceResource::s_Events.RemoveEventHandler(&ezJoltCore::SurfaceResourceEventHandler);
}

void ezJoltCore::SurfaceResourceEventHandler(const ezSurfaceResourceEvent& e)
{
  if (e.m_Type == ezSurfaceResourceEvent::Type::Created)
  {
    const auto& desc = e.m_pSurface->GetDescriptor();

    auto pNewMaterial = new ezJoltMaterial;
    pNewMaterial->AddRef();
    pNewMaterial->m_pSurface = e.m_pSurface;
    pNewMaterial->m_fRestitution = desc.m_fPhysicsRestitution;
    pNewMaterial->m_fFriction = ezMath::Lerp(desc.m_fPhysicsFrictionStatic, desc.m_fPhysicsFrictionDynamic, 0.5f);

    e.m_pSurface->m_pPhysicsMaterialJolt = pNewMaterial;
  }
  else if (e.m_Type == ezSurfaceResourceEvent::Type::Destroyed)
  {
    if (e.m_pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      ezJoltMaterial* pMaterial = static_cast<ezJoltMaterial*>(e.m_pSurface->m_pPhysicsMaterialJolt);
      pMaterial->Release();

      e.m_pSurface->m_pPhysicsMaterialJolt = nullptr;
    }
  }
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltCore);
