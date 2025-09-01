#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezAttributeHolder, ezNoBase, 1, ezRTTINoAllocator)
{
  flags.Add(ezTypeFlags::Abstract);
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_ACCESSOR_PROPERTY("Attributes", GetCount, GetValue, SetValue, Insert, Remove)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezAttributeHolder::ezAttributeHolder() = default;

ezAttributeHolder::ezAttributeHolder(const ezAttributeHolder& rhs)
{
  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

ezAttributeHolder::~ezAttributeHolder()
{
  for (auto pAttr : m_Attributes)
  {
    if (pAttr)
      pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<ezPropertyAttribute*>(pAttr));
  }
}

void ezAttributeHolder::operator=(const ezAttributeHolder& rhs)
{
  if (this == &rhs)
    return;

  m_Attributes = rhs.m_Attributes;
  rhs.m_Attributes.Clear();

  m_ReferenceAttributes = rhs.m_ReferenceAttributes;
}

ezUInt32 ezAttributeHolder::GetCount() const
{
  return ezMath::Max(m_ReferenceAttributes.GetCount(), m_Attributes.GetCount());
}

const ezPropertyAttribute* ezAttributeHolder::GetValue(ezUInt32 uiIndex) const
{
  if (!m_ReferenceAttributes.IsEmpty())
    return m_ReferenceAttributes[uiIndex];

  return m_Attributes[uiIndex];
}

void ezAttributeHolder::SetValue(ezUInt32 uiIndex, const ezPropertyAttribute* value)
{
  m_Attributes[uiIndex] = value;
}

void ezAttributeHolder::Insert(ezUInt32 uiIndex, const ezPropertyAttribute* value)
{
  m_Attributes.InsertAt(uiIndex, value);
}

void ezAttributeHolder::Remove(ezUInt32 uiIndex)
{
  m_Attributes.RemoveAtAndCopy(uiIndex);
}

////////////////////////////////////////////////////////////////////////
// ezReflectedPropertyDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezReflectedPropertyDescriptor, ezAttributeHolder, 2, ezRTTIDefaultAllocator<ezReflectedPropertyDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("Category", ezPropertyCategory, m_Category),
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Type", m_sType),
    EZ_BITFLAGS_MEMBER_PROPERTY("Flags", ezPropertyFlags, m_Flags),
    EZ_MEMBER_PROPERTY("ConstantValue", m_ConstantValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

class ezReflectedPropertyDescriptorPatch_1_2 : public ezGraphPatch
{
public:
  ezReflectedPropertyDescriptorPatch_1_2()
    : ezGraphPatch("ezReflectedPropertyDescriptor", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    if (ezAbstractObjectNode::Property* pProp = pNode->FindProperty("Flags"))
    {
      ezStringBuilder sValue = pProp->m_Value.Get<ezString>();
      ezHybridArray<ezStringView, 32> values;
      sValue.Split(false, values, "|");

      ezStringBuilder sNewValue;
      for (ezInt32 i = (ezInt32)values.GetCount() - 1; i >= 0; i--)
      {
        if (values[i].IsEqual("ezPropertyFlags::Constant"))
        {
          values.RemoveAtAndCopy(i);
        }
        else if (values[i].IsEqual("ezPropertyFlags::EmbeddedClass"))
        {
          values[i] = ezStringView("ezPropertyFlags::Class");
        }
        else if (values[i].IsEqual("ezPropertyFlags::Pointer"))
        {
          values.PushBack(ezStringView("ezPropertyFlags::Class"));
        }
      }
      for (ezUInt32 i = 0; i < values.GetCount(); ++i)
      {
        if (i != 0)
          sNewValue.Append("|");
        sNewValue.Append(values[i]);
      }
      pProp->m_Value = sNewValue.GetData();
    }
  }
};

ezReflectedPropertyDescriptorPatch_1_2 g_ezReflectedPropertyDescriptorPatch_1_2;


ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(ezPropertyCategory::Enum category, ezStringView sName, ezStringView sType, ezBitflags<ezPropertyFlags> flags)
  : m_Category(category)
  , m_sName(sName)
  , m_sType(sType)
  , m_Flags(flags)
{
}

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(ezPropertyCategory::Enum category, ezStringView sName, ezStringView sType,
  ezBitflags<ezPropertyFlags> flags, ezArrayPtr<const ezPropertyAttribute* const> attributes)
  : m_Category(category)
  , m_sName(sName)
  , m_sType(sType)
  , m_Flags(flags)
{
  m_ReferenceAttributes = attributes;
}

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(
  ezStringView sName, const ezVariant& constantValue, ezArrayPtr<const ezPropertyAttribute* const> attributes)
  : m_Category(ezPropertyCategory::Constant)
  , m_sName(sName)
  , m_sType()
  , m_Flags(ezPropertyFlags::StandardType | ezPropertyFlags::ReadOnly)
  , m_ConstantValue(constantValue)
{
  m_ReferenceAttributes = attributes;
  const ezRTTI* pType = ezReflectionUtils::GetTypeFromVariant(constantValue);
  if (pType)
    m_sType = pType->GetTypeName();
}

ezReflectedPropertyDescriptor::ezReflectedPropertyDescriptor(const ezReflectedPropertyDescriptor& rhs)
{
  operator=(rhs);
}

void ezReflectedPropertyDescriptor::operator=(const ezReflectedPropertyDescriptor& rhs)
{
  m_Category = rhs.m_Category;
  m_sName = rhs.m_sName;

  m_sType = rhs.m_sType;

  m_Flags = rhs.m_Flags;
  m_ConstantValue = rhs.m_ConstantValue;

  ezAttributeHolder::operator=(rhs);
}

ezReflectedPropertyDescriptor::~ezReflectedPropertyDescriptor() = default;


////////////////////////////////////////////////////////////////////////
// ezFunctionParameterDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezFunctionArgumentDescriptor, ezNoBase, 1, ezRTTIDefaultAllocator<ezFunctionArgumentDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_sType),
    EZ_BITFLAGS_MEMBER_PROPERTY("Flags", ezPropertyFlags, m_Flags),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezFunctionArgumentDescriptor::ezFunctionArgumentDescriptor() = default;

ezFunctionArgumentDescriptor::ezFunctionArgumentDescriptor(ezStringView sType, ezBitflags<ezPropertyFlags> flags)
  : m_sType(sType)
  , m_Flags(flags)
{
}


////////////////////////////////////////////////////////////////////////
// ezReflectedFunctionDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezReflectedFunctionDescriptor, ezAttributeHolder, 1, ezRTTIDefaultAllocator<ezReflectedFunctionDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_BITFLAGS_MEMBER_PROPERTY("Flags", ezPropertyFlags, m_Flags),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezFunctionType, m_Type),
    EZ_MEMBER_PROPERTY("ReturnValue", m_ReturnValue),
    EZ_ARRAY_MEMBER_PROPERTY("Arguments", m_Arguments),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezReflectedFunctionDescriptor::ezReflectedFunctionDescriptor() = default;

ezReflectedFunctionDescriptor::ezReflectedFunctionDescriptor(ezStringView sName, ezBitflags<ezPropertyFlags> flags, ezEnum<ezFunctionType> type, ezArrayPtr<const ezPropertyAttribute* const> attributes)
  : m_sName(sName)
  , m_Flags(flags)
  , m_Type(type)
{
  m_ReferenceAttributes = attributes;
}

ezReflectedFunctionDescriptor::ezReflectedFunctionDescriptor(const ezReflectedFunctionDescriptor& rhs)
{
  operator=(rhs);
}

ezReflectedFunctionDescriptor::~ezReflectedFunctionDescriptor() = default;

void ezReflectedFunctionDescriptor::operator=(const ezReflectedFunctionDescriptor& rhs)
{
  m_sName = rhs.m_sName;
  m_Flags = rhs.m_Flags;
  m_Type = rhs.m_Type;
  m_ReturnValue = rhs.m_ReturnValue;
  m_Arguments = rhs.m_Arguments;
  ezAttributeHolder::operator=(rhs);
}

////////////////////////////////////////////////////////////////////////
// ezReflectedTypeDescriptor
////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezReflectedTypeDescriptor, ezAttributeHolder, 1, ezRTTIDefaultAllocator<ezReflectedTypeDescriptor>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("TypeName", m_sTypeName),
    EZ_MEMBER_PROPERTY("PluginName", m_sPluginName),
    EZ_MEMBER_PROPERTY("ParentTypeName", m_sParentTypeName),
    EZ_BITFLAGS_MEMBER_PROPERTY("Flags", ezTypeFlags, m_Flags),
    EZ_ARRAY_MEMBER_PROPERTY("Properties", m_Properties),
    EZ_ARRAY_MEMBER_PROPERTY("Functions", m_Functions),
    EZ_MEMBER_PROPERTY("TypeVersion", m_uiTypeVersion),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezReflectedTypeDescriptor::~ezReflectedTypeDescriptor() = default;
