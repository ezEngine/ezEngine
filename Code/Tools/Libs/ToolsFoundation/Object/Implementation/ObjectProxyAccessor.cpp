#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezObjectProxyAccessor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezObjectProxyAccessor::ezObjectProxyAccessor(ezObjectAccessorBase* pSource)
  : ezObjectAccessorBase(pSource->GetObjectManager())
  , m_pSource(pSource)
{
}

ezObjectProxyAccessor::~ezObjectProxyAccessor() = default;

void ezObjectProxyAccessor::StartTransaction(ezStringView sDisplayString)
{
  m_pSource->StartTransaction(sDisplayString);
}

void ezObjectProxyAccessor::CancelTransaction()
{
  m_pSource->CancelTransaction();
}

void ezObjectProxyAccessor::FinishTransaction()
{
  m_pSource->FinishTransaction();
}

void ezObjectProxyAccessor::BeginTemporaryCommands(ezStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{
  m_pSource->BeginTemporaryCommands(sDisplayString, bFireEventsWhenUndoingTempCommands);
}

void ezObjectProxyAccessor::CancelTemporaryCommands()
{
  m_pSource->CancelTemporaryCommands();
}

void ezObjectProxyAccessor::FinishTemporaryCommands()
{
  m_pSource->FinishTemporaryCommands();
}

const ezDocumentObject* ezObjectProxyAccessor::GetObject(const ezUuid& object)
{
  return m_pSource->GetObject(object);
}

ezStatus ezObjectProxyAccessor::GetValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index /*= ezVariant()*/)
{
  return m_pSource->GetValue(pObject, pProp, out_value, index);
}

ezStatus ezObjectProxyAccessor::SetValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  return m_pSource->SetValue(pObject, pProp, newValue, index);
}

ezStatus ezObjectProxyAccessor::InsertValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  return m_pSource->InsertValue(pObject, pProp, newValue, index);
}

ezStatus ezObjectProxyAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
{
  return m_pSource->RemoveValue(pObject, pProp, index);
}

ezStatus ezObjectProxyAccessor::MoveValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  return m_pSource->MoveValue(pObject, pProp, oldIndex, newIndex);
}

ezStatus ezObjectProxyAccessor::GetCount(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezInt32& out_iCount)
{
  return m_pSource->GetCount(pObject, pProp, out_iCount);
}

ezStatus ezObjectProxyAccessor::AddObject(
  const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
{
  return m_pSource->AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
}

ezStatus ezObjectProxyAccessor::RemoveObject(const ezDocumentObject* pObject)
{
  return m_pSource->RemoveObject(pObject);
}

ezStatus ezObjectProxyAccessor::MoveObject(
  const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index)
{
  return m_pSource->MoveObject(pObject, pNewParent, pParentProp, index);
}

ezStatus ezObjectProxyAccessor::GetKeys(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_keys)
{
  return m_pSource->GetKeys(pObject, pProp, out_keys);
}

ezStatus ezObjectProxyAccessor::GetValues(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezDynamicArray<ezVariant>& out_values)
{
  return m_pSource->GetValues(pObject, pProp, out_values);
}
