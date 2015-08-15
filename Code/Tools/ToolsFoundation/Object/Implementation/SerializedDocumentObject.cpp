#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/SerializedDocumentObject.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Serialization/ObjectSerializationContext.h>
#include <ToolsFoundation/Serialization/ObjectSerializationHelper.h>

ezSerializedDocumentObjectWriter::ezSerializedDocumentObjectWriter(const ezDocumentObjectBase* pObject) : m_pObject(pObject)
{
  EZ_ASSERT_DEV(pObject != nullptr, "Object passed to ezSerializedDocumentObjectWriter must not be nullptr!");
}

ezSerializedDocumentObjectWriter::~ezSerializedDocumentObjectWriter()
{
}

ezUuid ezSerializedDocumentObjectWriter::GetGuid() const
{
  return m_pObject->GetGuid();
}

ezUuid ezSerializedDocumentObjectWriter::GetParentGuid() const
{
  auto pParent = m_pObject->GetParent();
  return pParent ? pParent->GetGuid() : ezUuid();
}

const char* ezSerializedDocumentObjectWriter::GetType(ezStringBuilder& builder) const
{
  auto pType = m_pObject->GetTypeAccessor().GetType();
  builder = pType->GetTypeName();
  return builder.GetData();
}

void ezSerializedDocumentObjectWriter::GatherProperties(ezPropertySerializationContext& context) const
{
  context.PushSubGroup("TypeData");
  {
    ezObjectSerializationHelper::WriteTypeAccessorToContext(m_pObject->GetTypeAccessor(), context);
  }
  context.PopSubGroup();
}


ezSerializedDocumentObjectReader::ezSerializedDocumentObjectReader(ezDocumentObjectBase* pObject) :
  m_pObject(pObject),
  m_pAccessor(nullptr)
{
}

ezSerializedDocumentObjectReader::~ezSerializedDocumentObjectReader()
{
}

void ezSerializedDocumentObjectReader::SetGuid(const ezUuid& guid)
{
  // No need to set it as we already did so in the object manager.
}

void ezSerializedDocumentObjectReader::SetParentGuid(const ezUuid& guid)
{
  m_ParentGuid = guid;
}

void ezSerializedDocumentObjectReader::SetProperty(const char* szName, const ezVariant& value)
{
  if (m_pAccessor)
  {
    m_pAccessor->SetValue(szName, value);
  }
}

void ezSerializedDocumentObjectReader::SubGroupChanged(const ezHybridArray<ezString, 8>& stack)
{
  if (stack.GetCount() == 1)
  {
    if (stack[0].Compare("TypeData") == 0)
    {
      m_pAccessor = &m_pObject->GetTypeAccessor();
    }
  }
  else
  {
    m_pAccessor = nullptr;
  }
}


