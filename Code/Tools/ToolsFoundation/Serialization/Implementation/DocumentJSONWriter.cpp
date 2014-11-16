#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Serialization/DocumentJSONWriter.h>
#include <ToolsFoundation/Serialization/SerializedObjectBase.h>
#include <ToolsFoundation/Serialization/ObjectSerializationContext.h>

ezDocumentJSONWriter::ezDocumentJSONWriter(ezStreamWriterBase* pOutput) :
  ezDocumentWriterBase(pOutput),
  m_bInGroup(false)
{
  m_Writer.SetOutputStream(pOutput);
  m_Writer.BeginObject();
}

ezDocumentJSONWriter::~ezDocumentJSONWriter()
{
}

void ezDocumentJSONWriter::StartGroup(const char* szName)
{
  EZ_ASSERT(!m_bInGroup, "Can't start another group while already in a group!");
  m_Writer.BeginArray(szName);
  m_bInGroup = true;
}

void ezDocumentJSONWriter::EndGroup()
{
  EZ_ASSERT(m_bInGroup, "Can't end a group if non was started!");
  m_Writer.EndArray();
  m_bInGroup = false;
}

void ezDocumentJSONWriter::WriteObject(const ezSerializedObjectWriterBase& object)
{
  m_Writer.BeginObject();
  {
    ezStringBuilder temp;
    m_Writer.AddVariableString("Type", object.GetType(temp));
    m_Writer.AddVariableVariant("ID", object.GetGuid());
    m_Writer.AddVariableVariant("ParentID", object.GetParentGuid());
    ezObjectSerializationContext context(*this);
    object.GatherProperties(context);
  }
  m_Writer.EndObject();
}

void ezDocumentJSONWriter::EndDocument()
{
  EZ_ASSERT(!m_bInGroup, "Can't end document if a group is still open!");
  m_Writer.EndObject();
}

void ezDocumentJSONWriter::PushSubGroup(const char* szGroupName)
{
  m_Writer.BeginObject(szGroupName);
  m_SubGroupStack.PushBack(szGroupName);
}

void ezDocumentJSONWriter::PopSubGroup()
{
  m_Writer.EndObject();
  m_SubGroupStack.PopBack();
}

void ezDocumentJSONWriter::AddProperty(const char* szName, const ezVariant& value)
{
  m_Writer.AddVariableVariant(szName, value);
}
