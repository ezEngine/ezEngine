#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Serialization/SerializedTypeAccessorObject.h>
#include <ToolsFoundation/Serialization/ObjectSerializationContext.h>
#include <ToolsFoundation/Serialization/ObjectSerializationHelper.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

ezSerializedTypeAccessorObjectWriter::ezSerializedTypeAccessorObjectWriter(const ezIReflectedTypeAccessor* pObject) : m_pObject(pObject)
{
  EZ_ASSERT(pObject, "Object passed to ezSerializedTypeAccessorObjectWriter must not be nullptr!");
}

ezSerializedTypeAccessorObjectWriter::~ezSerializedTypeAccessorObjectWriter()
{
}

ezUuid ezSerializedTypeAccessorObjectWriter::GetGuid() const
{
  return ezUuid();
}

ezUuid ezSerializedTypeAccessorObjectWriter::GetParentGuid() const
{
  return ezUuid();
}

const char* ezSerializedTypeAccessorObjectWriter::GetType(ezStringBuilder& builder) const
{
  auto pType = m_pObject->GetReflectedTypeHandle().GetType();
  EZ_ASSERT(pType, "ezIReflectedTypeAccessor used for serialization has an unknown type!");
  builder = pType->GetTypeName().GetData();
  return builder.GetData();
}

void ezSerializedTypeAccessorObjectWriter::GatherProperties(ezObjectSerializationContext& context) const
{
  context.PushSubGroup("TypeData");
  {
    ezObjectSerializationHelper::WriteTypeAccessorToContext(*m_pObject, context);
  }
  context.PopSubGroup();
}


ezSerializedTypeAccessorObjectReader::ezSerializedTypeAccessorObjectReader(ezIReflectedTypeAccessor* pObject) :
  m_pObject(pObject),
  m_bInTypeDataGroup(false)
{
}

ezSerializedTypeAccessorObjectReader::~ezSerializedTypeAccessorObjectReader()
{
}

void ezSerializedTypeAccessorObjectReader::SetGuid(const ezUuid& guid)
{
}

void ezSerializedTypeAccessorObjectReader::SetParentGuid(const ezUuid& guid)
{
}

void ezSerializedTypeAccessorObjectReader::SetProperty(const char* szName, const ezVariant& value)
{
  if (m_bInTypeDataGroup)
    m_pObject->SetValue(szName, value);
}

void ezSerializedTypeAccessorObjectReader::SubGroupChanged(const ezHybridArray<ezString, 8>& stack)
{
  if (stack.GetCount() == 1)
  {
    if (stack[0].Compare("TypeData") == 0)
    {
      m_bInTypeDataGroup = true;
    }
  }
  else
  {
    m_bInTypeDataGroup = false;
  }
}


