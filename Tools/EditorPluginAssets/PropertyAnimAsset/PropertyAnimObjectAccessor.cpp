#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

ezPropertyAnimObjectAccessor::ezPropertyAnimObjectAccessor(ezPropertyAnimAssetDocument* pDoc, ezCommandHistory* pHistory)
  : ezObjectCommandAccessor(pHistory)
  , m_ObjAccessor(pHistory)
  , m_pDocument(pDoc)
  , m_pObjectManager(static_cast<ezPropertyAnimObjectManager*>(pDoc->GetObjectManager()))
{
}

ezStatus ezPropertyAnimObjectAccessor::GetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index /*= ezVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return ezObjectCommandAccessor::GetValue(pObject, pProp, out_value, index);
  }
  else
  {
    return ezObjectCommandAccessor::GetValue(pObject, pProp, out_value, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index)
{
  if (IsTemporary(pObject))
  {
    ezVariantType::Enum type = pProp->GetSpecificType()->GetVariantType();
    if (type >= ezVariantType::Bool && type <= ezVariantType::Double)
    {
      return SetCurveCp(pObject, pProp, index, ezPropertyAnimTarget::Number, newValue.ConvertTo<double>());
    }
    else if (type >= ezVariantType::Vector2 && type <= ezVariantType::Vector4U)
    {
      ezVariant oldValue;
      EZ_VERIFY(m_ObjAccessor.GetValue(pObject, pProp, oldValue, index).Succeeded(), "Property does not exist, can't animate");
      const ezUInt32 uiComponents = ezReflectionUtils::GetComponentCount(type);
      for (ezUInt32 c = 0; c < uiComponents; c++)
      {
        double fOldValue = ezReflectionUtils::GetComponent(oldValue, c);
        double fValue = ezReflectionUtils::GetComponent(newValue, c);
        if (ezMath::IsEqual(fOldValue, fValue, ezMath::BasicType<double>::SmallEpsilon()))
          continue;
        ezStatus res = SetCurveCp(pObject, pProp, index, static_cast<ezPropertyAnimTarget::Enum>((int)ezPropertyAnimTarget::VectorX + c), fValue);
        if (res.Failed())
        {
          return res;
        }
      }
      return ezStatus(EZ_SUCCESS);
    }
    else if (type == ezVariantType::Color || type == ezVariantType::ColorGamma)
    {
      //#TODO: case ezPropertyAnimTarget::Color:
    }
    return ezStatus(ezFmt("The property '{0}' cannot be animated.", pProp->GetPropertyName()));
  }
  else
  {
    return ezObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

ezStatus ezPropertyAnimObjectAccessor::AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
{
  if (IsTemporary(pParent, pParentProp))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

ezStatus ezPropertyAnimObjectAccessor::RemoveObject(const ezDocumentObject* pObject)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::RemoveObject(pObject);
  }
}

ezStatus ezPropertyAnimObjectAccessor::MoveObject(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index)
{
  if (IsTemporary(pObject))
  {
    return ezStatus("The structure of the context cannot be animated.");
  }
  else
  {
    return ezObjectCommandAccessor::MoveObject(pObject, pNewParent, pParentProp, index);
  }
}

bool ezPropertyAnimObjectAccessor::IsTemporary(const ezDocumentObject* pObject) const
{
  return m_pObjectManager->IsTemporary(pObject);
}

bool ezPropertyAnimObjectAccessor::IsTemporary(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp) const
{
  return m_pObjectManager->IsTemporary(pParent, pParentProp->GetPropertyName());
}


ezStatus ezPropertyAnimObjectAccessor::SetCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target, double fValue)
{
  ezUuid track = FindOrAddTrack(pObject, pProp, index, target);
  SetOrInsertCurveCp(track, fValue);
  return ezStatus(EZ_SUCCESS);
}

ezUuid ezPropertyAnimObjectAccessor::FindOrAddTrack(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target)
{
  ezUuid track = m_pDocument->FindTrack(pObject, pProp, index, target);
  if (!track.IsValid())
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    track = m_pDocument->CreateTrack(pObject, pProp, index, target);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  EZ_ASSERT_DEBUG(track.IsValid(), "Creating track failed.");
  return track;
}

ezStatus ezPropertyAnimObjectAccessor::SetOrInsertCurveCp(const ezUuid& track, double fValue)
{
  const ezInt64 iScrubberPos = (ezInt64)m_pDocument->GetScrubberPosition();
  ezUuid cpGuid = m_pDocument->FindCurveCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    const ezAbstractProperty* pValueProp = ezGetStaticRTTI<ezCurveControlPointData>()->FindPropertyByName("Value");
    m_ObjAccessor.SetValue(pCP, pValueProp, fValue);
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertCurveCpAt(track, iScrubberPos, fValue);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return ezStatus(EZ_SUCCESS);
}
