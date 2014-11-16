#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Serialization/DocumentJSONReader.h>
#include <ToolsFoundation/Serialization/SerializedObjectBase.h>

ezDocumentJSONReader::ezDocumentJSONReader(ezStreamReaderBase* pInput) :
  ezDocumentReaderBase(pInput),
  m_pGroup(nullptr),
  m_pObject(nullptr)
{
  //m_Reader.SetInputStream(m_pInput);
  ezResult res = m_Reader.Parse(*m_pInput);
  EZ_ASSERT(res == EZ_SUCCESS, "Reading JSON file failed!");
}

ezDocumentJSONReader::~ezDocumentJSONReader()
{
}

bool ezDocumentJSONReader::OpenGroup(const char* szName)
{
  const ezVariantDictionary& root = m_Reader.GetTopLevelObject();
  ezVariant* pGroup = nullptr;
  if (root.TryGetValue(szName, pGroup))
  {
    EZ_ASSERT(pGroup->GetType() == ezVariant::Type::VariantArray, "Group value found is not an array!");
    m_pGroup = &pGroup->Get<ezVariantArray>();
    m_pObject = nullptr;
    m_uiObjextIndex = 0;
    return true;
  } 

  m_pGroup = nullptr;
  m_pObject = nullptr;
  return false;
}

bool ezDocumentJSONReader::PeekNextObject(ezUuid& out_objectGuid, ezStringBuilder& out_sType, ezUuid& out_parentGuid)
{
  EZ_ASSERT(m_pGroup, "Need to call 'OpenGroup' first before reading objects!");
  out_objectGuid = ezUuid();
  out_sType.Clear();
  out_parentGuid = ezUuid();

  if (m_uiObjextIndex >= m_pGroup->GetCount())
  {
    return false;
  }

  const ezVariant& object = (*m_pGroup)[m_uiObjextIndex];
  EZ_ASSERT(object.GetType() == ezVariant::Type::VariantDictionary, "Object value found is not a dictionary!");
  m_pObject = &object.Get<ezVariantDictionary>();

  ezVariant* pVar = nullptr;
  if (m_pObject->TryGetValue("Type", pVar))
  {
    out_sType = pVar->Get<ezString>();
  }

  if (m_pObject->TryGetValue("ID", pVar))
  {
    out_objectGuid = pVar->Get<ezUuid>();
  }

  if (m_pObject->TryGetValue("ParentID", pVar))
  {
    out_parentGuid = pVar->Get<ezUuid>();
  }

  m_uiObjextIndex++;
  return true;
}

void ezDocumentJSONReader::ReadObject(ezSerializedObjectReaderBase& object)
{
  EZ_ASSERT(m_pObject, "No object to read, call OpenGroup and then PeekNextObject!");
  ezVariant* pType = nullptr;
  ezHybridArray<ezString, 8> stack;

  for (auto it = m_pObject->GetIterator(); it.IsValid(); ++it)
  {
    if (it.Key().Compare("ID") == 0)
    {
      object.SetGuid(it.Value().Get<ezUuid>());
    }
    else if (it.Key().Compare("ParentID") == 0)
    {
      object.SetParentGuid(it.Value().Get<ezUuid>());
    }
    else if (it.Key().Compare("Type") == 0)
    {
      // Ignore as this field is obsolete now.
    }
    else
    {
      ReadObjectRecursive(it, object, stack);
    }
  }
}

void ezDocumentJSONReader::ReadObjectRecursive(const ezHashTable<ezString, ezVariant>::ConstIterator& it, ezSerializedObjectReaderBase& object, ezHybridArray<ezString, 8>& stack)
{
  const ezVariant& value = it.Value();
  switch (value.GetType())
  {
  case ezVariant::Type::VariantDictionary:
    {
      stack.PushBack(it.Key());
      object.SubGroupChanged(stack);
      const ezVariantDictionary& vd = value.Get<ezVariantDictionary>();

      for (auto it2 = vd.GetIterator(); it2.IsValid(); ++it2)
      {
        ReadObjectRecursive(it2, object, stack);
      }

      stack.PopBack();
      object.SubGroupChanged(stack);
    }
    break;

  default:
    {
      object.SetProperty(it.Key(), it.Value());
    }
    break;
  }
}