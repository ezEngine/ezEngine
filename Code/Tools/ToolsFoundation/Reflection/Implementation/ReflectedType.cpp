#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeHandle functions
////////////////////////////////////////////////////////////////////////

const ezReflectedType* ezReflectedTypeHandle::GetType() const
{
  return ezReflectedTypeManager::GetType(*this);
}


////////////////////////////////////////////////////////////////////////
// ezReflectedPropertyDescriptor functions
////////////////////////////////////////////////////////////////////////

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(const char* szName, const char* szType, ezBitflags<PropertyFlags> flags)
  : m_sName(szName), m_sType(szType), m_Type(ezVariant::Type::Invalid), m_Flags(flags)
{
}

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(const char* szName, ezVariant::Type::Enum type, ezBitflags<PropertyFlags> flags)
  : m_sName(szName), m_sType(), m_Type(type), m_Flags(flags)
{
}

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(const char* szName, ezVariant::Type::Enum type, const ezVariant& constantValue)
  : m_sName(szName), m_sType(), m_Type(type), m_Flags(PropertyFlags::IsPOD | PropertyFlags::IsReadOnly | PropertyFlags::IsConstant)
  , m_ConstantValue(constantValue)
{
}


////////////////////////////////////////////////////////////////////////
// ezReflectedProperty functions
////////////////////////////////////////////////////////////////////////

ezReflectedProperty::ezReflectedProperty(const char* szName, ezVariant::Type::Enum type, ezBitflags<PropertyFlags> flags)
  : m_hTypeHandle(), m_Type(type), m_Flags(flags)
{
  m_sPropertyName.Assign(szName);
}

ezReflectedProperty::ezReflectedProperty(const char* szName, ezReflectedTypeHandle m_hType, ezBitflags<PropertyFlags> flags)
  : m_hTypeHandle(m_hType), m_Type(ezVariant::Type::Invalid), m_Flags(flags)
{
  m_sPropertyName.Assign(szName);
}


////////////////////////////////////////////////////////////////////////
// ezReflectedConstant functions
////////////////////////////////////////////////////////////////////////

ezReflectedConstant::ezReflectedConstant(const char* szName, const ezVariant& constantValue)
  : m_ConstantValue(constantValue)
{
  m_sPropertyName.Assign(szName);
}


////////////////////////////////////////////////////////////////////////
// ezReflectedType public functions
////////////////////////////////////////////////////////////////////////

bool ezReflectedType::IsDerivedFrom(ezReflectedTypeHandle hType) const
{
  ezReflectedTypeHandle type = m_hType;
  while (!type.IsInvalidated())
  {
    if (hType == type)
      return true;

    type = type.GetType()->GetParentTypeHandle();
  }
  return false;
}

const ezReflectedProperty* ezReflectedType::GetPropertyByIndex(ezUInt32 uiIndex) const
{
  if (uiIndex < m_Properties.GetCount())
  {
    return &m_Properties[uiIndex];
  }

  return nullptr;
}

const ezReflectedProperty* ezReflectedType::GetPropertyByName(const char* szPropertyName) const
{
  ezUInt32 uiIndex = 0;
  if (m_PropertyNameToIndex.TryGetValue(szPropertyName, uiIndex))
  {
    return &m_Properties[uiIndex];
  }

  if (GetParentTypeHandle().GetType() != nullptr)
  {
    return GetParentTypeHandle().GetType()->GetPropertyByName(szPropertyName);

  }

  return nullptr;
}

const ezReflectedProperty* ezReflectedType::GetPropertyByPath(const ezPropertyPath& path) const
{
  const ezReflectedType* pCurrentType = this;
  const ezReflectedProperty* pCurrentProperty = nullptr;
  for (auto szPropertyName : path)
  {
    if (pCurrentType == nullptr)
      return nullptr;

    pCurrentProperty = pCurrentType->GetPropertyByName(szPropertyName);
    if (pCurrentProperty == nullptr)
    {
      return nullptr;
    }
    pCurrentType = pCurrentProperty->m_hTypeHandle.GetType();
  }
  return pCurrentProperty;
}

const ezReflectedConstant* ezReflectedType::GetConstantByIndex(ezUInt32 uiIndex) const
{
  if (uiIndex < m_Constants.GetCount())
  {
    return &m_Constants[uiIndex];
  }

  return nullptr;
}

const ezReflectedConstant* ezReflectedType::GetConstantByName(const char* szPropertyName) const
{
  ezUInt32 uiIndex = 0;
  if (m_ConstantNameToIndex.TryGetValue(szPropertyName, uiIndex))
  {
    return &m_Constants[uiIndex];
  }
  
  return nullptr;
}

void ezReflectedType::GetDependencies(ezSet<ezReflectedTypeHandle>& out_dependencies, bool bTransitive) const
{
  out_dependencies.Clear();
  out_dependencies = m_Dependencies;

  if (!bTransitive)
    return;

  ezDeque<ezReflectedTypeHandle> queue;

  for (auto it = m_Dependencies.GetIterator(); it.IsValid(); ++it)
  {
    queue.PushBack(it.Key());
  }

  while (!queue.IsEmpty())
  {
    ezReflectedTypeHandle handle = queue.PeekFront();
    queue.PopFront();

    const ezReflectedType* pType = handle.GetType();
    EZ_ASSERT_DEV(pType != nullptr, "A dependency could not be resolved to an actual type!");
    for (auto it = pType->m_Dependencies.GetIterator(); it.IsValid(); ++it)
    {
      if (!out_dependencies.Find(it.Key()).IsValid())
      {
        // Dependency is new, add to both queue and set.
        queue.PushBack(it.Key());
        out_dependencies.Insert(it.Key());
      }
    }
  }
  
}


////////////////////////////////////////////////////////////////////////
// ezReflectedType public functions
////////////////////////////////////////////////////////////////////////

ezReflectedType::ezReflectedType(const char* szTypeName, const char* szPluginName, ezReflectedTypeHandle hParentType)
  : m_hParentType(hParentType)
{
  m_sTypeName.Assign(szTypeName);
  m_sPluginName.Assign(szPluginName);
}

void ezReflectedType::RegisterProperties()
{
  const ezUInt32 uiPropertyCount = m_Properties.GetCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const char* szPropertyName = m_Properties[i].m_sPropertyName.GetString().GetData();
    EZ_ASSERT_DEV(!m_PropertyNameToIndex.Contains(szPropertyName), "A property with the name '%s' already exists!", szPropertyName);
    m_PropertyNameToIndex.Insert(szPropertyName, i);
  }
}

void ezReflectedType::RegisterConstants()
{
  const ezUInt32 uiPropertyCount = m_Constants.GetCount();
  for (ezUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const char* szPropertyName = m_Constants[i].m_sPropertyName.GetString().GetData();
    EZ_ASSERT_DEV(!m_ConstantNameToIndex.Contains(szPropertyName), "A constant with the name '%s' already exists!", szPropertyName);
    m_ConstantNameToIndex.Insert(szPropertyName, i);
  }
}