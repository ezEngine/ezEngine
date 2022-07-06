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

ezJoltMaterial::ezJoltMaterial() = default;
ezJoltMaterial::~ezJoltMaterial() = default;

//#define EZ_Jolt_DETAILED_MEMORY_STATS EZ_ON
//#define EZ_Jolt_DETAILED_MEMORY_STATS EZ_OFF
//
// ezJoltAllocatorCallback::ezJoltAllocatorCallback()
//  : m_Allocator("Jolt", ezFoundation::GetAlignedAllocator())
//{
//}
//
// void* ezJoltAllocatorCallback::allocate(size_t size, const char* typeName, const char* filename, int line)
//{
//  void* pPtr = m_Allocator.Allocate(size, 16);
//
//#if EZ_ENABLED(EZ_Jolt_DETAILED_MEMORY_STATS)
//  ezStringBuilder s;
//  s.Set(typeName, " - ", filename);
//  m_Allocations[pPtr] = s;
//#endif
//
//  return pPtr;
//}
//
// void ezJoltAllocatorCallback::deallocate(void* ptr)
//{
//  if (ptr == nullptr)
//    return;
//
//#if EZ_ENABLED(EZ_Jolt_DETAILED_MEMORY_STATS)
//  m_Allocations.Remove(ptr);
//#endif
//
//  m_Allocator.Deallocate(ptr);
//}
//
// void ezJoltAllocatorCallback::VerifyAllocations()
//{
//#if EZ_ENABLED(EZ_Jolt_DETAILED_MEMORY_STATS)
//  EZ_ASSERT_DEV(m_Allocations.IsEmpty(), "There are {0} unfreed allocations", m_Allocations.GetCount());
//
//  for (auto it = m_Allocations.GetIterator(); it.IsValid(); ++it)
//  {
//    const char* s = it.Value().GetData();
//    ezLog::Info(s);
//  }
//#endif
//}

static void JoltTraceFunc(const char* szText, ...)
{
  ezStringBuilder tmp;

  va_list args;
  va_start(args, szText);
  tmp.PrintfArgs(szText, args);
  va_end(args);

  ezLog::Dev("Jolt: {}", tmp);
}

#ifdef JPH_ENABLE_ASSERTS

static bool JoltAssertFailed(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine)
{
  return ezFailedCheck(inFile, inLine, "Jolt", inExpression, inMessage);
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

void ezJoltCore::Startup()
{
  JPH::Trace = JoltTraceFunc;
  JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed;)

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

#ifdef JPH_ENABLE_ASSERTS
  JPH::AssertFailed = JPH::DummyAssertFailed;
#endif

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
