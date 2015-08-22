#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

////////////////////////////////////////////////////////////////////////
// ezPropertyPath functions
////////////////////////////////////////////////////////////////////////

ezPropertyPath::ezPropertyPath()
{
}

ezPropertyPath::ezPropertyPath(const char* szPath)
{
  ezStringBuilder temp = szPath;
  temp.Split(false, *this, "/");
}

ezStringBuilder ezPropertyPath::GetPathString() const
{
  ezStringBuilder sPath;
  EZ_ASSERT_DEV(GetCount() > 0, "Path must not be empty");

  sPath = (*this)[0];

  const ezUInt32 uiPathLength = GetCount();
  for (ezUInt32 i = 1; i < uiPathLength; ++i)
  {
    sPath.Append("/", (*this)[i]);
  }
  return sPath;
}



////////////////////////////////////////////////////////////////////////
// ezReflectedPropertyDescriptor
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezReflectedPropertyDescriptor, ezNoBase, 1, ezRTTIDefaultAllocator<ezReflectedPropertyDescriptor>);
EZ_BEGIN_PROPERTIES
  EZ_ENUM_MEMBER_PROPERTY("Category", ezPropertyCategory, m_Category),
  EZ_MEMBER_PROPERTY("Name", m_sName),
  EZ_MEMBER_PROPERTY("Type", m_sType),
  EZ_BITFLAGS_MEMBER_PROPERTY("Flags", ezPropertyFlags, m_Flags),
  EZ_MEMBER_PROPERTY("ConstantValue", m_ConstantValue),
  EZ_ARRAY_ACCESSOR_PROPERTY("Attributes", GetCount, GetValue, SetValue, Insert, Remove)->AddFlags(ezPropertyFlags::PointerOwner),
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(ezPropertyCategory::Enum category, const char* szName, const char* szType, ezVariant::Type::Enum type, ezBitflags<ezPropertyFlags> flags, const ezArrayPtr<ezPropertyAttribute* const> attributes)
  : m_Category(category), m_sName(szName), m_sType(szType), m_Flags(flags)
{
  m_ReferenceAttributes = attributes;
}

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(const char* szName, ezVariant::Type::Enum type, const ezVariant& constantValue, const ezArrayPtr<ezPropertyAttribute* const> attributes)
  : m_Category(ezPropertyCategory::Constant), m_sName(szName), m_sType(), m_Flags(ezPropertyFlags::StandardType | ezPropertyFlags::ReadOnly | ezPropertyFlags::Constant)
  , m_ConstantValue(constantValue)
{
  m_ReferenceAttributes = attributes;
}

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(const ezReflectedPropertyDescriptor& rhs)
{
  operator= (rhs);
}

void ezReflectedPropertyDescriptor::operator=(const ezReflectedPropertyDescriptor& rhs)
{
  m_Category = rhs.m_Category;
  m_sName = rhs.m_sName;

  m_sType = rhs.m_sType;

  m_Flags = rhs.m_Flags;
  m_ConstantValue = rhs.m_ConstantValue;

  m_Attributes = rhs.m_Attributes; 
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

ezReflectedPropertyDescriptor::~ezReflectedPropertyDescriptor()
{
  for (auto pAttr : m_Attributes)
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
}

ezUInt32 ezReflectedPropertyDescriptor::GetCount() const
{
  return ezMath::Max(m_ReferenceAttributes.GetCount(), m_Attributes.GetCount());
}

ezPropertyAttribute* ezReflectedPropertyDescriptor::GetValue(ezUInt32 uiIndex) const
{
  if (!m_ReferenceAttributes.IsEmpty())
    return m_ReferenceAttributes[uiIndex];

  return m_Attributes[uiIndex];
}

void ezReflectedPropertyDescriptor::SetValue(ezUInt32 uiIndex, ezPropertyAttribute* value)
{
  m_Attributes[uiIndex] = value;
}

void ezReflectedPropertyDescriptor::Insert(ezUInt32 uiIndex, ezPropertyAttribute* value)
{
  m_Attributes.Insert(value, uiIndex);
}

void ezReflectedPropertyDescriptor::Remove(ezUInt32 uiIndex)
{
  m_Attributes.RemoveAt(uiIndex);
}

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeDescriptor
////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezReflectedTypeDescriptor, ezNoBase, 1, ezRTTIDefaultAllocator<ezReflectedTypeDescriptor>);
EZ_BEGIN_PROPERTIES
  EZ_MEMBER_PROPERTY("TypeName", m_sTypeName),
  EZ_MEMBER_PROPERTY("PluginName", m_sPluginName),
  EZ_MEMBER_PROPERTY("ParentTypeName", m_sParentTypeName),
  EZ_MEMBER_PROPERTY("DefaultInitialization", m_sDefaultInitialization),
  EZ_BITFLAGS_MEMBER_PROPERTY("Flags", ezTypeFlags, m_Flags),
  EZ_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
  EZ_MEMBER_PROPERTY("TypeSize", m_uiTypeSize),
  EZ_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

