#include <PCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezTypeVersionInfo, ezNoBase, 1, ezRTTIDefaultAllocator<ezTypeVersionInfo>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("TypeName", GetTypeName, SetTypeName),
    EZ_ACCESSOR_PROPERTY("ParentTypeName", GetParentTypeName, SetParentTypeName),
    EZ_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

const char* ezTypeVersionInfo::GetTypeName() const
{
  return m_sTypeName.GetData();
}

void ezTypeVersionInfo::SetTypeName(const char* szName)
{
  m_sTypeName.Assign(szName);
}

const char* ezTypeVersionInfo::GetParentTypeName() const
{
  return m_sParentTypeName.GetData();
}

void ezTypeVersionInfo::SetParentTypeName(const char* szName)
{
  m_sParentTypeName.Assign(szName);
}

void ezGraphPatchContext::PatchBaseClass(const char* szType, ezUInt32 uiTypeVersion, bool bForcePatch)
{
  ezHashedString sType;
  sType.Assign(szType);
  for (ezUInt32 uiBaseClassIndex = m_uiBaseClassIndex; uiBaseClassIndex < m_BaseClasses.GetCount(); ++uiBaseClassIndex)
  {
    if (m_BaseClasses[uiBaseClassIndex].m_sType == sType)
    {
      Patch(uiBaseClassIndex, uiTypeVersion, bForcePatch);
      return;
    }
  }
  EZ_REPORT_FAILURE("Base class of name '{0}' not found in parent types of '{1}'", sType.GetData(), m_pNode->GetType());
}

void ezGraphPatchContext::RenameClass(const char* szTypeName)
{
  m_pNode->SetType(m_pGraph->RegisterString(szTypeName));
  m_BaseClasses[m_uiBaseClassIndex].m_sType.Assign(szTypeName);
}


void ezGraphPatchContext::ChangeBaseClass(ezArrayPtr<ezVersionKey> baseClasses)
{
  m_BaseClasses.SetCount(m_uiBaseClassIndex + 1 + baseClasses.GetCount());
  for (ezUInt32 i = 0; i < baseClasses.GetCount(); i++)
  {
    m_BaseClasses[m_uiBaseClassIndex + 1 + i] = baseClasses[i];
  }
}

//////////////////////////////////////////////////////////////////////////

ezGraphPatchContext::ezGraphPatchContext(ezGraphVersioning* pParent, ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph)
{
  EZ_PROFILE("ezGraphPatchContext");
  m_pParent = pParent;
  m_pGraph = pGraph;
  if (pTypesGraph)
  {
    ezRttiConverterContext context;
    ezRttiConverterReader rttiConverter(pTypesGraph, &context);
    ezString sDescTypeName = "ezReflectedTypeDescriptor";
    auto& nodes = pTypesGraph->GetAllNodes();
    m_TypeToInfo.Reserve(nodes.GetCount());
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->GetType() == sDescTypeName)
      {
        ezTypeVersionInfo info;
        rttiConverter.ApplyPropertiesToObject(it.Value(), ezGetStaticRTTI<ezTypeVersionInfo>(), &info);
        m_TypeToInfo.Insert(info.m_sTypeName, info);
      }
    }
  }
}

void ezGraphPatchContext::Patch(ezAbstractObjectNode* pNode)
{
  m_pNode = pNode;
  // Build version hierarchy.
  m_BaseClasses.Clear();
  ezVersionKey key;
  key.m_sType.Assign(m_pNode->GetType());
  key.m_uiTypeVersion = m_pNode->GetTypeVersion();

  m_BaseClasses.PushBack(key);
  UpdateBaseClasses();

  // Patch
  bool bPatched = false;
  for (m_uiBaseClassIndex = 0; m_uiBaseClassIndex < m_BaseClasses.GetCount(); ++m_uiBaseClassIndex)
  {
    const ezUInt32 uiMaxVersion = m_pParent->GetMaxPatchVersion(m_BaseClasses[m_uiBaseClassIndex].m_sType);
    Patch(m_uiBaseClassIndex, uiMaxVersion, false);
  }
  m_pNode->SetTypeVersion(m_BaseClasses[0].m_uiTypeVersion);
}


void ezGraphPatchContext::Patch(ezUInt32 uiBaseClassIndex, ezUInt32 uiTypeVersion, bool bForcePatch)
{
  if (bForcePatch)
  {
    m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion = ezMath::Min(m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion, uiTypeVersion - 1);
  }
  while (m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion < uiTypeVersion)
  {
    // Don't move this out of the loop, needed to support renaming a class which will change the key.
    ezVersionKey key = m_BaseClasses[uiBaseClassIndex];
    key.m_uiTypeVersion += 1;
    const ezGraphPatch* pPatch = nullptr;
    if (m_pParent->m_NodePatches.TryGetValue(key, pPatch))
    {
      pPatch->Patch(*this, m_pGraph, m_pNode);
      uiTypeVersion = m_pParent->GetMaxPatchVersion(m_BaseClasses[m_uiBaseClassIndex].m_sType);
    }
    // Don't use a ref to the key as the array might get resized during patching.
    m_BaseClasses[m_uiBaseClassIndex].m_uiTypeVersion = key.m_uiTypeVersion;
  }
}

void ezGraphPatchContext::UpdateBaseClasses()
{
  for (;;)
  {
    ezHashedString sParentType;
    if (ezTypeVersionInfo* pInfo = m_TypeToInfo.GetValue(m_BaseClasses.PeekBack().m_sType))
    {
      m_BaseClasses.PeekBack().m_uiTypeVersion = pInfo->m_uiTypeVersion;
      sParentType = pInfo->m_sParentTypeName;
    }
    else if (const ezRTTI* pType = ezRTTI::FindTypeByName(m_BaseClasses.PeekBack().m_sType.GetData()))
    {
      m_BaseClasses.PeekBack().m_uiTypeVersion = pType->GetTypeVersion();
      if (pType->GetParentType())
      {
        sParentType.Assign(pType->GetParentType()->GetTypeName());
      }
      else
        sParentType = ezHashedString();
    }
    else
    {
      ezLog::Error("Can't patch base class, parent type of '{0}' unknown.", m_BaseClasses.PeekBack().m_sType.GetData());
      break;
    }

    if (sParentType.IsEmpty())
      break;

    ezVersionKey key;
    key.m_sType = sParentType;
    key.m_uiTypeVersion = 0;
    m_BaseClasses.PushBack(key);
  }
}

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SINGLETON(ezGraphVersioning);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, GraphVersioning)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Reflection"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezGraphVersioning);
  }

  ON_CORE_SHUTDOWN
  {
    ezGraphVersioning* pDummy = ezGraphVersioning::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezGraphVersioning::ezGraphVersioning()
    : m_SingletonRegistrar(this)
{
  ezPlugin::s_PluginEvents.AddEventHandler(ezMakeDelegate(&ezGraphVersioning::PluginEventHandler, this));

  UpdatePatches();
}

ezGraphVersioning::~ezGraphVersioning()
{
  ezPlugin::s_PluginEvents.RemoveEventHandler(ezMakeDelegate(&ezGraphVersioning::PluginEventHandler, this));
}

void ezGraphVersioning::PatchGraph(ezAbstractObjectGraph* pGraph, ezAbstractObjectGraph* pTypesGraph)
{
  EZ_PROFILE("PatchGraph");

  ezGraphPatchContext context(this, pGraph, pTypesGraph);
  for (const ezGraphPatch* pPatch : m_GraphPatches)
  {
    pPatch->Patch(context, pGraph, nullptr);
  }

  auto& nodes = pGraph->GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    ezAbstractObjectNode* pNode = it.Value();
    context.Patch(pNode);
  }
}

void ezGraphVersioning::PluginEventHandler(const ezPlugin::PluginEvent& EventData)
{
  UpdatePatches();
}

void ezGraphVersioning::UpdatePatches()
{
  m_GraphPatches.Clear();
  m_NodePatches.Clear();
  m_MaxPatchVersion.Clear();

  ezVersionKey key;
  ezGraphPatch* pInstance = ezGraphPatch::GetFirstInstance();

  while (pInstance)
  {
    switch (pInstance->GetPatchType())
    {
      case ezGraphPatch::PatchType::NodePatch:
      {
        key.m_sType = pInstance->GetType();
        key.m_uiTypeVersion = pInstance->GetTypeVersion();
        m_NodePatches.Insert(key, pInstance);

        if (ezUInt32* pMax = m_MaxPatchVersion.GetValue(key.m_sType))
        {
          *pMax = ezMath::Max(*pMax, key.m_uiTypeVersion);
        }
        else
        {
          m_MaxPatchVersion[key.m_sType] = key.m_uiTypeVersion;
        }
      }
      break;
      case ezGraphPatch::PatchType::GraphPatch:
      {
        m_GraphPatches.PushBack(pInstance);
      }
      break;
    }
    pInstance = pInstance->GetNextInstance();
  }

  m_GraphPatches.Sort([](const ezGraphPatch* a, const ezGraphPatch* b) -> bool { return a->GetTypeVersion() < b->GetTypeVersion(); });
}

ezUInt32 ezGraphVersioning::GetMaxPatchVersion(const ezHashedString& sType) const
{
  if (const ezUInt32* pMax = m_MaxPatchVersion.GetValue(sType))
  {
    return *pMax;
  }
  return 0;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_GraphVersioning);
