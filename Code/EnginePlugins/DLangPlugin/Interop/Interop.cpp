#include <DLangPlugin/DLangPluginPCH.h>

#include <DLangPlugin/Interop/DComponentInterfaces.h>
#include <DLangPlugin/Interop/Interop.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/Map.h>

struct ezDComponentType
{
  ComponentCreationFunc m_Creator = nullptr;
  ComponentDestructionFunc m_Destructor = nullptr;
};

struct LiveComponent
{
  ezDLangBaseComponent* m_pComponent = nullptr;
  ComponentDestructionFunc m_pDestructor = nullptr;
};

ezMap<ezString, ezDComponentType> m_DComponentTypes;
ezArrayMap<ezDLangBaseComponent*, ComponentDestructionFunc> m_LiveComponents;

void ezDLangInterop::RegisterComponentType(const char* szName, ComponentCreationFunc creator, ComponentDestructionFunc destructor)
{
  ezStringBuilder name = szName;
  if (const char* dot = ezStringUtils::FindLastSubString(szName, "."))
  {
    name = dot + 1;
  }

  ezLog::Info("Registered {}", name);

  auto& type = m_DComponentTypes[name];
  type.m_Creator = creator;
  type.m_Destructor = destructor;
}

ezDLangBaseComponent* ezDLangInterop::CreateComponentType(const char* szName, ezGameObject* pOwner, ezComponent* pOwnerComponent)
{
  auto it = m_DComponentTypes.Find(szName);
  if (!it.IsValid())
    return nullptr;

  if (it.Value().m_Creator == nullptr)
    return nullptr;

  auto pComp = it.Value().m_Creator(pOwner, pOwnerComponent);
  m_LiveComponents[pComp] = it.Value().m_Destructor;
  return pComp;
}

void ezDLangInterop::DestroyComponentType(ezDLangBaseComponent* pComp)
{
  ezUInt32 idx = m_LiveComponents.Find(pComp);
  EZ_ASSERT_DEV(idx != ezInvalidIndex, "Unknown D component to destroy");

  m_LiveComponents.GetValue(idx)(pComp);
  m_LiveComponents.RemoveAtAndCopy(idx);
}
