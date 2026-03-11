#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>


ezQtVisualShaderScene::ezQtVisualShaderScene(QObject* pParent)
  : ezQtVisualGraphScene(pParent)
{
}

ezQtVisualShaderScene::~ezQtVisualShaderScene() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualShaderPin::ezQtVisualShaderPin() = default;

void ezQtVisualShaderPin::SetPin(const ezVisualGraphPin& pin)
{
  ezQtVisualGraphPin::SetPin(pin);

  const ezVisualShaderPin& shaderPin = ezStaticCast<const ezVisualShaderPin&>(pin);

  ezStringBuilder sTooltip;
  if (!shaderPin.GetTooltip().IsEmpty())
  {
    sTooltip = shaderPin.GetTooltip();
  }
  else
  {
    sTooltip = shaderPin.GetName();
  }

  if (!shaderPin.GetDescriptor()->m_sDefaultValue.IsEmpty())
  {
    if (!sTooltip.IsEmpty())
      sTooltip.Append("\n");

    sTooltip.Append("Default is ", shaderPin.GetDescriptor()->m_sDefaultValue);
  }

  setToolTip(sTooltip.GetData());
}

void ezQtVisualShaderPin::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
  QPainterPath p = path();

  const ezVisualShaderPin* pVsPin = static_cast<const ezVisualShaderPin*>(GetPin());

  pPainter->save();
  pPainter->setBrush(brush());
  pPainter->setPen(pen());

  if (pVsPin->GetType() == ezVisualGraphPin::Type::Input && GetConnections().IsEmpty())
  {
    if (pVsPin->GetDescriptor()->m_sDefaultValue.IsEmpty())
    {
      // this pin MUST be connected

      QPen pen;
      pen.setColor(qRgb(255, 0, 0));
      pen.setWidth(3);
      pen.setCosmetic(true);
      pen.setStyle(Qt::PenStyle::SolidLine);
      pen.setCapStyle(Qt::PenCapStyle::SquareCap);

      pPainter->setPen(pen);

      pPainter->drawRect(this->path().boundingRect());
      pPainter->restore();
      return;
    }
  }

  pPainter->drawPath(p);
  pPainter->restore();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualShaderNode::ezQtVisualShaderNode() = default;

void ezQtVisualShaderNode::InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject)
{
  ezQtVisualGraphNode::InitNode(pManager, pObject);

  if (auto pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType()))
  {
    m_HeaderColor = ezToQtColor(pDesc->m_Color);
    m_pTitleLabel->setToolTip(ezMakeQString(pDesc->m_sDocs));
  }
  else
  {
    m_HeaderColor = qRgb(255, 0, 0);
    ezLog::Error("Could not initialize node type, node descriptor is invalid");
  }
}

void ezQtVisualShaderNode::UpdateState()
{
  ezStringBuilder sTitle = GetObject()->GetTypeAccessor().GetType()->GetTypeName();
  if (sTitle.StartsWith_NoCase("ShaderNode::"))
    sTitle.Shrink(12, 0);

  auto pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());
  if (pDesc != nullptr && !pDesc->m_sTitle.IsEmpty())
  {
    // Use the configured title and replace placeholders
    sTitle = pDesc->m_sTitle;

    // Replace input pin placeholders (e.g., {$in0}, {$in1})
    ezStringBuilder temp;
    ezStringBuilder sVal;
    ezVariant val;
    const auto& inputPins = GetInputPins();

    for (ezUInt32 i = 0; i < pDesc->m_InputPins.GetCount(); ++i)
    {
      const ezVisualShaderPinDescriptor& pinDesc = pDesc->m_InputPins[i];

      // Build placeholder string: {$in0}, {$in1}, etc.
      temp.SetFormat("$in{}", i);
      temp.Prepend("{");
      temp.Append("}");

      if (i < inputPins.GetCount() && inputPins[i]->HasAnyConnections())
      {
        // Pin is connected, show pin name
        sTitle.ReplaceAll(temp, pinDesc.m_sName);
      }
      else if (pinDesc.m_bExposeAsProperty)
      {
        // Pin is exposed as a property, get the actual property value
        ezVariant val = GetObject()->GetTypeAccessor().GetValue(pinDesc.m_PropertyDesc.m_sName);

        if (val.IsA<ezColor>())
        {
          ezColor color = val.Get<ezColor>();
          sVal = ezConversionUtils::GetColorName(color);
        }
        else if (val.IsA<ezColorGammaUB>())
        {
          ezColorGammaUB colorGamma = val.Get<ezColorGammaUB>();
          ezColor color = colorGamma;
          sVal = ezConversionUtils::GetColorName(color);
        }
        else if (val.IsA<ezVec2>())
        {
          ezVec2 v = val.Get<ezVec2>();
          sVal.SetFormat("({}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2));
        }
        else if (val.IsA<ezVec3>())
        {
          ezVec3 v = val.Get<ezVec3>();
          sVal.SetFormat("({}, {}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2), ezArgF(v.z, 2));
        }
        else if (val.IsA<ezVec4>())
        {
          ezVec4 v = val.Get<ezVec4>();
          sVal.SetFormat("({}, {}, {}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2), ezArgF(v.z, 2), ezArgF(v.w, 2));
        }
        else if (val.CanConvertTo<ezString>())
        {
          sVal = val.ConvertTo<ezString>();
        }
        else
        {
          sVal = pinDesc.m_sDefaultValue;
        }

        sTitle.ReplaceAll(temp, sVal);
      }
      else if (!pinDesc.m_sDefaultValue.IsEmpty())
      {
        // Pin is not connected and not exposed, use default value
        sTitle.ReplaceAll(temp, pinDesc.m_sDefaultValue);
      }
      else
      {
        // No default value, show pin name
        sTitle.ReplaceAll(temp, pinDesc.m_sName);
      }
    }

    // Replace property index placeholders (e.g., {$prop0}, {$prop1})
    for (ezUInt32 i = 0; i < pDesc->m_Properties.GetCount(); ++i)
    {
      const ezReflectedPropertyDescriptor& propDesc = pDesc->m_Properties[i];

      // Build placeholder string: {$prop0}, {$prop1}, etc.
      temp.SetFormat("$prop{}", i);
      temp.Prepend("{");
      temp.Append("}");

      val = GetObject()->GetTypeAccessor().GetValue(propDesc.m_sName);

      // Get the actual property to check its type
      const ezAbstractProperty* pProp = GetObject()->GetType()->FindPropertyByName(propDesc.m_sName);
      if (pProp != nullptr && (pProp->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || pProp->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>()))
      {
        ezReflectionUtils::EnumerationToString(pProp->GetSpecificType(), val.ConvertTo<ezInt64>(), sVal);
      }
      else if (val.IsA<ezColor>())
      {
        ezColor color = val.Get<ezColor>();
        sVal = ezConversionUtils::GetColorName(color);
      }
      else if (val.IsA<ezColorGammaUB>())
      {
        ezColorGammaUB colorGamma = val.Get<ezColorGammaUB>();
        ezColor color = colorGamma;
        sVal = ezConversionUtils::GetColorName(color);
      }
      else if (val.IsA<ezVec2>())
      {
        ezVec2 v = val.Get<ezVec2>();
        sVal.SetFormat("({}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2));
      }
      else if (val.IsA<ezVec3>())
      {
        ezVec3 v = val.Get<ezVec3>();
        sVal.SetFormat("({}, {}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2), ezArgF(v.z, 2));
      }
      else if (val.IsA<ezVec4>())
      {
        ezVec4 v = val.Get<ezVec4>();
        sVal.SetFormat("({}, {}, {}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2), ezArgF(v.z, 2), ezArgF(v.w, 2));
      }
      else if (val.CanConvertTo<ezString>())
      {
        sVal = val.ConvertTo<ezString>();
      }
      else
      {
        sVal = "<Invalid>";
      }

      sTitle.ReplaceAll(temp, sVal);
    }

    // Replace property name placeholders (e.g., {PropertyName} or {$PropertyName})
    ezTempHybridArray<const ezAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

    for (const auto& prop : properties)
    {
      val = GetObject()->GetTypeAccessor().GetValue(prop->GetPropertyName());

      if (prop->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>())
      {
        ezReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<ezInt64>(), sVal);
      }
      else if (val.IsA<ezColor>())
      {
        ezColor color = val.Get<ezColor>();
        sVal = ezConversionUtils::GetColorName(color);
      }
      else if (val.IsA<ezColorGammaUB>())
      {
        ezColorGammaUB colorGamma = val.Get<ezColorGammaUB>();
        ezColor color = colorGamma;
        sVal = ezConversionUtils::GetColorName(color);
      }
      else if (val.IsA<ezVec2>())
      {
        ezVec2 v = val.Get<ezVec2>();
        sVal.SetFormat("({}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2));
      }
      else if (val.IsA<ezVec3>())
      {
        ezVec3 v = val.Get<ezVec3>();
        sVal.SetFormat("({}, {}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2), ezArgF(v.z, 2));
      }
      else if (val.IsA<ezVec4>())
      {
        ezVec4 v = val.Get<ezVec4>();
        sVal.SetFormat("({}, {}, {}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2), ezArgF(v.z, 2), ezArgF(v.w, 2));
      }
      else if (val.CanConvertTo<ezString>())
      {
        sVal = val.ConvertTo<ezString>();
      }
      else
      {
        sVal = "<Invalid>";
      }

      // Replace both {PropertyName} and {$PropertyName} patterns
      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);

      temp.Set("{$", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);
    }
  }

  m_pTitleLabel->setPlainText(sTitle.GetData());
}
