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
#include <JoltPlugin/System/JoltJobSystem.h>
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
  // EZ_BITFLAGS_CONSTANT(ezOnJoltContact::SendContactMsg), // do not expose in the UI
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::ImpactReactions),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::SlideReactions),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::RollXReactions),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::RollYReactions),
  EZ_BITFLAGS_CONSTANT(ezOnJoltContact::RollZReactions),
EZ_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

ezJoltMaterial* ezJoltCore::s_pDefaultMaterial = nullptr;
ezUniquePtr<JPH::JobSystem> ezJoltCore::s_pJobSystemEZ;
std::unique_ptr<JPH::JobSystem> ezJoltCore::s_pJobSystemJolt;
ezUniquePtr<ezProxyAllocator> ezJoltCore::s_pAllocator;
ezUniquePtr<ezProxyAllocator> ezJoltCore::s_pAllocatorAligned;
ezCollisionFilterConfig ezJoltCore::s_CollisionFilterConfig;
ezWeightCategoryConfig ezJoltCore::s_WeightCategoryConfig;
ezImpulseTypeConfig ezJoltCore::s_ImpulseTypeConfig;

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

ezCVarBool cvar_JoltUseEzTaskSystem("Jolt.UseEzTaskSystem", true, ezCVarFlags::Save, "Use the EZ TaskSystem for Jolt updates");

JPH::JobSystem* ezJoltCore::GetJoltJobSystem()
{
  if (cvar_JoltUseEzTaskSystem)
  {
    if (s_pJobSystemEZ == nullptr)
    {
      s_pJobSystemEZ = EZ_NEW(ezFoundation::GetAlignedAllocator(), ezJoltJobSystem, JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers);
    }

    return s_pJobSystemEZ.Borrow();
  }
  else
  {
    if (s_pJobSystemJolt == nullptr)
    {
      s_pJobSystemJolt = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);
    }

    return s_pJobSystemJolt.get();
  }
}

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

void* ezJoltCore::JoltMalloc(size_t inSize)
{
  return ezJoltCore::s_pAllocator->Allocate(inSize, 8);
}

void ezJoltCore::JoltFree(void* inBlock)
{
  ezJoltCore::s_pAllocator->Deallocate(inBlock);
}

void* ezJoltCore::JoltReallocate(void* inBlock, size_t inOldSize, size_t inNewSize)
{
  if (inBlock == nullptr)
  {
    return JoltMalloc(inNewSize);
  }
  else
  {
    return ezJoltCore::s_pAllocator->Reallocate(inBlock, inOldSize, inNewSize, 8);
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

const ezCollisionFilterConfig& ezJoltCore::GetCollisionFilterConfig()
{
  return s_CollisionFilterConfig;
}

const ezWeightCategoryConfig& ezJoltCore::GetWeightCategoryConfig()
{
  return s_WeightCategoryConfig;
}

const ezImpulseTypeConfig& ezJoltCore::GetImpulseTypeConfig()
{
  return s_ImpulseTypeConfig;
}

void ezJoltCore::ReloadConfigs()
{
  LoadCollisionFilters();
  LoadWeightCategories();
  LoadImpulseTypes();
}

void ezJoltCore::LoadCollisionFilters()
{
  EZ_LOG_BLOCK("ezJoltCore::LoadCollisionFilters");

  if (s_CollisionFilterConfig.Load().Failed())
  {
    ezLog::Info("Collision filter config file could not be found ('{}'). Using default values.", ezCollisionFilterConfig::s_sConfigFile);

    // setup some default config

    s_CollisionFilterConfig.SetGroupName(0, "Default");
    s_CollisionFilterConfig.EnableCollision(0, 0);
  }
}

void ezJoltCore::LoadWeightCategories()
{
  EZ_LOG_BLOCK("ezJoltCore::LoadWeightCategories");

  if (s_WeightCategoryConfig.Load().Failed())
  {
    ezLog::Info("Weight category config file could not be found ('{}').", ezWeightCategoryConfig::s_sConfigFile);
  }
}

void ezJoltCore::LoadImpulseTypes()
{
  EZ_LOG_BLOCK("ezJoltCore::LoadImpulseTypes");

  if (s_ImpulseTypeConfig.Load().Failed())
  {
    ezLog::Info("Impulse Types config file could not be found ('{}').", ezImpulseTypeConfig::s_sConfigFile);
  }
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

  s_pJobSystemEZ = nullptr;
  s_pJobSystemJolt = nullptr;

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

    ezJoltMaterial* pJoltMat = static_cast<ezJoltMaterial*>(e.m_pSurface->m_pPhysicsMaterialJolt);

    if (pJoltMat == nullptr)
    {
      pJoltMat = new ezJoltMaterial;
      pJoltMat->AddRef();
      pJoltMat->m_pSurface = e.m_pSurface;
    }
    else
    {
      EZ_ASSERT_DEV(pJoltMat->m_pSurface == e.m_pSurface, "Invalid surface");
    }

    pJoltMat->m_fRestitution = desc.m_fPhysicsRestitution;
    pJoltMat->m_fFriction = ezMath::Lerp(desc.m_fPhysicsFrictionStatic, desc.m_fPhysicsFrictionDynamic, 0.5f);

    e.m_pSurface->m_pPhysicsMaterialJolt = pJoltMat;
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
