#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectAccessor.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

ezPropertyAnimObjectAccessor::ezPropertyAnimObjectAccessor(ezPropertyAnimAssetDocument* pDoc, ezCommandHistory* pHistory)
  : ezObjectCommandAccessor(pHistory)
  , m_pDocument(pDoc)
  , m_pObjectManager(static_cast<ezPropertyAnimObjectManager*>(pDoc->GetObjectManager()))
{
  m_pObjAccessor = EZ_DEFAULT_NEW(ezObjectCommandAccessor, pHistory);
}

ezStatus ezPropertyAnimObjectAccessor::GetValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant& out_value, ezVariant index /*= ezVariant()*/)
{
  return ezObjectCommandAccessor::GetValue(pObject, pProp, out_value, index);
}

ezStatus ezPropertyAnimObjectAccessor::SetValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index)
{
  if (IsTemporary(pObject))
  {
    ezVariant oldValue;
    EZ_VERIFY(m_pObjAccessor->GetValue(pObject, pProp, oldValue, index).Succeeded(), "Property does not exist, can't animate");

    ezVariantType::Enum type = pProp->GetSpecificType()->GetVariantType();
    if (type >= ezVariantType::Bool && type <= ezVariantType::Double)
    {
      return SetCurveCp(pObject, pProp, index, ezPropertyAnimTarget::Number, oldValue.ConvertTo<double>(), newValue.ConvertTo<double>());
    }
    else if (type >= ezVariantType::Vector2 && type <= ezVariantType::Vector4U)
    {
      const ezUInt32 uiComponents = ezReflectionUtils::GetComponentCount(type);
      for (ezUInt32 c = 0; c < uiComponents; c++)
      {
        const double fOldValue = ezReflectionUtils::GetComponent(oldValue, c);
        const double fValue = ezReflectionUtils::GetComponent(newValue, c);

        if (ezMath::IsEqual(fOldValue, fValue, ezMath::SmallEpsilon<double>()))
          continue;

        EZ_SUCCEED_OR_RETURN(
          SetCurveCp(pObject, pProp, index, static_cast<ezPropertyAnimTarget::Enum>((int)ezPropertyAnimTarget::VectorX + c), fOldValue, fValue));
      }

      return ezStatus(EZ_SUCCESS);
    }
    else if (type == ezVariantType::Color)
    {
      auto oldColor = oldValue.Get<ezColor>();
      ezColorGammaUB oldColorGamma;
      ezUInt8 oldAlpha;
      float oldIntensity;
      SeparateColor(oldColor, oldColorGamma, oldAlpha, oldIntensity);
      auto newColor = newValue.Get<ezColor>();
      ezColorGammaUB newColorGamma;
      ezUInt8 newAlpha;
      float newIntensity;
      SeparateColor(newColor, newColorGamma, newAlpha, newIntensity);

      ezStatus res(EZ_SUCCESS);
      if (oldColorGamma != newColorGamma)
      {
        res = SetColorCurveCp(pObject, pProp, index, oldColorGamma, newColorGamma);
      }
      if (oldAlpha != newAlpha && res.Succeeded())
      {
        res = SetAlphaCurveCp(pObject, pProp, index, oldAlpha, newAlpha);
      }
      if (!ezMath::IsEqual(oldIntensity, newIntensity, ezMath::SmallEpsilon<float>()) && res.Succeeded())
      {
        res = SetIntensityCurveCp(pObject, pProp, index, oldIntensity, newIntensity);
      }
      return res;
    }
    else if (type == ezVariantType::ColorGamma)
    {
      auto oldColorGamma = oldValue.Get<ezColorGammaUB>();
      ezUInt8 oldAlpha = oldColorGamma.a;
      oldColorGamma.a = 255;

      auto newColorGamma = newValue.Get<ezColorGammaUB>();
      ezUInt8 newAlpha = newColorGamma.a;
      newColorGamma.a = 255;

      ezStatus res(EZ_SUCCESS);
      if (oldColorGamma != newColorGamma)
      {
        res = SetColorCurveCp(pObject, pProp, index, oldColorGamma, newColorGamma);
      }
      if (oldAlpha != newAlpha && res.Succeeded())
      {
        res = SetAlphaCurveCp(pObject, pProp, index, oldAlpha, newAlpha);
      }
      return res;
    }
    else if (type == ezVariantType::Quaternion)
    {

      const ezQuat qOldRot = oldValue.Get<ezQuat>();
      const ezQuat qNewRot = newValue.Get<ezQuat>();

      ezAngle oldEuler[3];
      qOldRot.GetAsEulerAngles(oldEuler[0], oldEuler[1], oldEuler[2]);
      ezAngle newEuler[3];
      qNewRot.GetAsEulerAngles(newEuler[0], newEuler[1], newEuler[2]);

      for (ezUInt32 c = 0; c < 3; c++)
      {
        EZ_SUCCEED_OR_RETURN(
          m_pDocument->CanAnimate(pObject, pProp, index, static_cast<ezPropertyAnimTarget::Enum>((int)ezPropertyAnimTarget::RotationX + c)));
        float oldValue = oldEuler[c].GetDegree();
        ezUuid track = FindOrAddTrack(pObject, pProp, index, static_cast<ezPropertyAnimTarget::Enum>((int)ezPropertyAnimTarget::RotationX + c),
          [this, oldValue](const ezUuid& trackGuid)
          {
            // add a control point at the start of the curve with the original value
            m_pDocument->InsertCurveCpAt(trackGuid, 0, oldValue);
          });
        const auto* pTrack = m_pDocument->GetTrack(track);
        oldEuler[c] = ezAngle::MakeFromDegree(pTrack->m_FloatCurve.Evaluate(m_pDocument->GetScrubberPosition()));
      }

      for (ezUInt32 c = 0; c < 3; c++)
      {
        // We assume the change is less than 180 degrees from the old value
        float fDiff = (newEuler[c] - oldEuler[c]).GetDegree();
        float iRounds = ezMath::RoundToMultiple(fDiff, 360.0f);
        fDiff -= iRounds;
        newEuler[c] = oldEuler[c] + ezAngle::MakeFromDegree(fDiff);
        if (oldEuler[c].IsEqualSimple(newEuler[c], ezAngle::MakeFromDegree(0.01f)))
          continue;

        EZ_SUCCEED_OR_RETURN(SetCurveCp(pObject, pProp, index, static_cast<ezPropertyAnimTarget::Enum>((int)ezPropertyAnimTarget::RotationX + c),
          oldEuler[c].GetDegree(), newEuler[c].GetDegree()));
      }

      return ezStatus(EZ_SUCCESS);
    }

    return ezStatus(ezFmt("The property '{0}' cannot be animated.", pProp->GetPropertyName()));
  }
  else
  {
    return ezObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
  }
}

ezStatus ezPropertyAnimObjectAccessor::InsertValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
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

ezStatus ezPropertyAnimObjectAccessor::RemoveValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
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

ezStatus ezPropertyAnimObjectAccessor::MoveValue(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
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

ezStatus ezPropertyAnimObjectAccessor::AddObject(
  const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
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

ezStatus ezPropertyAnimObjectAccessor::MoveObject(
  const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const ezAbstractProperty* pParentProp, const ezVariant& index)
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


ezStatus ezPropertyAnimObjectAccessor::SetCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index,
  ezPropertyAnimTarget::Enum target, double fOldValue, double fNewValue)
{
  EZ_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, target));
  ezUuid track = FindOrAddTrack(pObject, pProp, index, target, [this, fOldValue](const ezUuid& trackGuid)
    {
    // add a control point at the start of the curve with the original value
    m_pDocument->InsertCurveCpAt(trackGuid, 0, fOldValue); });
  return SetOrInsertCurveCp(track, fNewValue);
}

ezUuid ezPropertyAnimObjectAccessor::FindOrAddTrack(
  const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezPropertyAnimTarget::Enum target, OnAddTrack onAddTrack)
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
    onAddTrack(track);

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
    EZ_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Value", fValue).Succeeded(), "");
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

ezStatus ezPropertyAnimObjectAccessor::SetColorCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, const ezColorGammaUB& oldValue, const ezColorGammaUB& newValue)
{
  EZ_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, ezPropertyAnimTarget::Color));
  ezUuid track = FindOrAddTrack(pObject, pProp, index, ezPropertyAnimTarget::Color, [this, &oldValue](const ezUuid& trackGuid)
    {
      // add a control point at the start of the curve with the original value
      m_pDocument->InsertGradientColorCpAt(trackGuid, 0, oldValue);
      //
    });

  EZ_SUCCEED_OR_RETURN(SetOrInsertColorCurveCp(track, newValue));

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPropertyAnimObjectAccessor::SetOrInsertColorCurveCp(const ezUuid& track, const ezColorGammaUB& value)
{
  const ezInt64 iScrubberPos = (ezInt64)m_pDocument->GetScrubberPosition();
  ezUuid cpGuid = m_pDocument->FindGradientColorCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    EZ_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Red", value.r).Succeeded(), "");
    EZ_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Green", value.g).Succeeded(), "");
    EZ_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Blue", value.b).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientColorCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPropertyAnimObjectAccessor::SetAlphaCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, ezUInt8 oldValue, ezUInt8 newValue)
{
  EZ_SUCCEED_OR_RETURN(m_pDocument->CanAnimate(pObject, pProp, index, ezPropertyAnimTarget::Color));
  ezUuid track = FindOrAddTrack(pObject, pProp, index, ezPropertyAnimTarget::Color, [this, &oldValue](const ezUuid& trackGuid)
    {
      // add a control point at the start of the curve with the original value
      m_pDocument->InsertGradientAlphaCpAt(trackGuid, 0, oldValue);
      //
    });

  EZ_SUCCEED_OR_RETURN(SetOrInsertAlphaCurveCp(track, newValue));
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPropertyAnimObjectAccessor::SetOrInsertAlphaCurveCp(const ezUuid& track, ezUInt8 value)
{
  const ezInt64 iScrubberPos = (ezInt64)m_pDocument->GetScrubberPosition();
  ezUuid cpGuid = m_pDocument->FindGradientAlphaCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    EZ_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Alpha", value).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientAlphaCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPropertyAnimObjectAccessor::SetIntensityCurveCp(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index, float oldValue, float newValue)
{
  ezUuid track = FindOrAddTrack(pObject, pProp, index, ezPropertyAnimTarget::Color, [this, &oldValue](const ezUuid& trackGuid)
    {
      // add a control point at the start of the curve with the original value
      m_pDocument->InsertGradientIntensityCpAt(trackGuid, 0, oldValue);
      //
    });

  EZ_SUCCEED_OR_RETURN(SetOrInsertIntensityCurveCp(track, newValue));
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPropertyAnimObjectAccessor::SetOrInsertIntensityCurveCp(const ezUuid& track, float value)
{
  const ezInt64 iScrubberPos = (ezInt64)m_pDocument->GetScrubberPosition();
  ezUuid cpGuid = m_pDocument->FindGradientIntensityCp(track, iScrubberPos);
  if (cpGuid.IsValid())
  {
    auto pCP = GetObject(cpGuid);
    EZ_VERIFY(m_pObjAccessor->SetValueByName(pCP, "Intensity", value).Succeeded(), "");
  }
  else
  {
    auto pHistory = m_pDocument->GetCommandHistory();
    bool bWasTemporaryTransaction = pHistory->InTemporaryTransaction();
    if (bWasTemporaryTransaction)
    {
      pHistory->SuspendTemporaryTransaction();
    }
    cpGuid = m_pDocument->InsertGradientIntensityCpAt(track, iScrubberPos, value);
    if (bWasTemporaryTransaction)
    {
      pHistory->ResumeTemporaryTransaction();
    }
  }
  return ezStatus(EZ_SUCCESS);
}

void ezPropertyAnimObjectAccessor::SeparateColor(const ezColor& color, ezColorGammaUB& gamma, ezUInt8& alpha, float& intensity)
{
  alpha = static_cast<ezColorGammaUB>(color).a;
  intensity = ezMath::Max(color.r, color.g, color.b);
  if (intensity > 1.0f)
  {
    gamma = (color / intensity);
    gamma.a = 255;
  }
  else
  {
    intensity = 1.0f;
    gamma = color;
    gamma.a = 255;
  }
}
