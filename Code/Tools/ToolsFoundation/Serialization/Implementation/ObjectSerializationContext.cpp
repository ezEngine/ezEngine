#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Serialization/ObjectSerializationContext.h>
#include <ToolsFoundation/Serialization/DocumentWriterBase.h>

ezPropertySerializationContext::ezPropertySerializationContext(ezDocumentWriterBase& writer) : m_pWriter(&writer)
{
}

void ezPropertySerializationContext::PushSubGroup(const char* szGroupName)
{
  m_pWriter->PushSubGroup(szGroupName);
}

void ezPropertySerializationContext::PopSubGroup()
{
  m_pWriter->PopSubGroup();
}

void ezPropertySerializationContext::AddProperty(const char* szName, const ezVariant& value)
{
  m_pWriter->AddProperty(szName, value);
}

void ezPropertySerializationContext::AddProperty(ezPropertyPath& path, const ezVariant& value)
{
  ezStringBuilder sPath;
  EZ_ASSERT_DEV(!path.IsEmpty(), "Can't call AddProperty with an empty path!");
  sPath = path[0];
  const ezUInt32 uiCount = path.GetCount();
  for (ezUInt32 i = 1; i < uiCount; i++)
  {
    sPath.AppendPath(path[i]);
  }

  AddProperty(sPath.GetData(), value);
}
