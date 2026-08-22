#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>


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
  TitleFormat format;
  format.m_uiMaxStringLength = 0;
  format.m_bQuoteStrings = false;
  format.m_bSplitAtDoubleColon = false;

  auto pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());

  ezStringBuilder sTemplate;
  if (pDesc != nullptr && !pDesc->m_sTitle.IsEmpty())
  {
    sTemplate = pDesc->m_sTitle;
  }
  else
  {
    sTemplate = GetObject()->GetType()->GetTypeName();
    if (sTemplate.StartsWith_NoCase("ShaderNode::"))
    {
      sTemplate.Shrink(12, 0);
    }
  }

  ezStringBuilder sTitle;
  ezTokenParseUtils::RenderTemplate(sTemplate, [&](ezStringView sPlaceholder, ezVariant index, bool bOptional, ezStringBuilder& ref_sOutput)
    { ResolvePlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); },
    sTitle);

  SetTitleAndSubtitle(sTitle, format);
}

void ezQtVisualShaderNode::ResolvePlaceholder(ezStringView sPlaceholder, const ezVariant& index, bool bOptional, const TitleFormat& format, ezStringBuilder& ref_sOutput)
{
  auto pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());

  if (pDesc != nullptr)
  {
    // the titles refer to pins and properties by position, e.g. {$in0} and {$prop0}
    ezUInt32 uiSlot = 0;

    if (TryParseSlotPlaceholder(sPlaceholder, "in"_ezsv, pDesc->m_InputPins.GetCount(), uiSlot))
    {
      AppendInputPinValue(pDesc->m_InputPins[uiSlot], uiSlot, format, ref_sOutput);
      return;
    }

    if (TryParseSlotPlaceholder(sPlaceholder, "prop"_ezsv, pDesc->m_Properties.GetCount(), uiSlot))
    {
      ResolvePropertyPlaceholder(pDesc->m_Properties[uiSlot].m_sName, index, bOptional, format, ref_sOutput);
      return;
    }
  }

  ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput);
}

bool ezQtVisualShaderNode::TryParseSlotPlaceholder(ezStringView sPlaceholder, ezStringView sPrefix, ezUInt32 uiSlotCount, ezUInt32& out_uiSlot)
{
  if (!sPlaceholder.StartsWith(sPrefix))
    return false;

  sPlaceholder.Shrink(sPrefix.GetElementCount(), 0);

  ezInt32 iSlot = 0;
  if (ezConversionUtils::StringToInt(sPlaceholder, iSlot).Failed() || iSlot < 0 || static_cast<ezUInt32>(iSlot) >= uiSlotCount)
    return false;

  out_uiSlot = static_cast<ezUInt32>(iSlot);
  return true;
}

void ezQtVisualShaderNode::AppendInputPinValue(const ezVisualShaderPinDescriptor& pinDesc, ezUInt32 uiPin, const TitleFormat& format, ezStringBuilder& ref_sOutput)
{
  const auto& inputPins = GetInputPins();

  if (uiPin < inputPins.GetCount() && inputPins[uiPin]->HasAnyConnections())
  {
    ref_sOutput.Append(pinDesc.m_sName.GetView());
    return;
  }

  if (pinDesc.m_bExposeAsProperty)
  {
    const ezAbstractProperty* pProp = GetObject()->GetType()->FindPropertyByName(pinDesc.m_PropertyDesc.m_sName);
    const ezVariant value = GetObject()->GetTypeAccessor().GetValue(pinDesc.m_PropertyDesc.m_sName);

    if (pProp != nullptr && value.IsValid() && value.CanConvertTo<ezString>())
    {
      AppendPropertyValue(pProp, {}, false, format, ref_sOutput);
    }
    else
    {
      ref_sOutput.Append(pinDesc.m_sDefaultValue.GetView());
    }

    return;
  }

  ref_sOutput.Append(pinDesc.m_sDefaultValue.IsEmpty() ? pinDesc.m_sName.GetView() : pinDesc.m_sDefaultValue.GetView());
}
