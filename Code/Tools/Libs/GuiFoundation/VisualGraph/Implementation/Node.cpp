#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsPixmapItem>
#include <QPainter>

ezQtVisualGraphNode::ezQtVisualGraphNode()
{
  auto palette = QApplication::palette();

  setFlag(QGraphicsItem::ItemIsMovable);
  setFlag(QGraphicsItem::ItemIsSelectable);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges);

  setBrush(palette.window());
  QPen pen(palette.mid().color(), 3, Qt::SolidLine);
  setPen(pen);

  {
    QFont font = QApplication::font();
    font.setBold(true);

    m_pTitleLabel = new QGraphicsTextItem(this);
    m_pTitleLabel->setFont(font);
  }

  {
    QFont font = QApplication::font();
    font.setPointSizeF(font.pointSizeF() * 0.9f);

    m_pSubtitleLabel = new QGraphicsTextItem(this);
    m_pSubtitleLabel->setFont(font);
    m_pSubtitleLabel->setPos(0, m_pTitleLabel->boundingRect().bottom() - 5);
  }

  {
    m_pIcon = new QGraphicsPixmapItem(this);
  }

  m_HeaderColor = palette.alternateBase().color();
}

ezQtVisualGraphNode::~ezQtVisualGraphNode()
{
  EnableDropShadow(false);
}

void ezQtVisualGraphNode::EnableDropShadow(bool bEnable)
{
  if (bEnable && m_pShadow == nullptr)
  {
    auto palette = QApplication::palette();

    m_pShadow = new QGraphicsDropShadowEffect();
    m_pShadow->setOffset(3, 3);
    m_pShadow->setColor(palette.color(QPalette::Shadow));
    m_pShadow->setBlurRadius(10);
    setGraphicsEffect(m_pShadow);
  }

  if (!bEnable && m_pShadow != nullptr)
  {
    delete m_pShadow;
    m_pShadow = nullptr;
  }
}

void ezQtVisualGraphNode::InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject)
{
  m_pManager = pManager;
  m_pObject = pObject;
  CreatePins();
  UpdateState();

  UpdateGeometry();

  if (const ezColorAttribute* pColorAttr = pObject->GetType()->GetAttributeByType<ezColorAttribute>())
  {
    m_HeaderColor = ezToQtColor(pColorAttr->GetColor());
  }

  m_DirtyFlags.Add(ezQtVisualGraphNodeFlags::UpdateTitle);
}

void ezQtVisualGraphNode::UpdateGeometry()
{
  prepareGeometryChange();

  QRectF iconRect = m_pIcon->boundingRect();
  iconRect.moveTo(m_pIcon->pos());
  iconRect.setSize(iconRect.size() * m_pIcon->scale());

  QRectF titleRect;
  {
    QPointF titlePos = m_pTitleLabel->pos();
    titlePos.setX(iconRect.right());
    m_pTitleLabel->setPos(titlePos);

    titleRect = m_pTitleLabel->boundingRect();
    titleRect.moveTo(titlePos);
  }

  m_pIcon->setPos(0, (titleRect.bottom() - iconRect.height()) / 2);

  QRectF subtitleRect;
  if (m_pSubtitleLabel->toPlainText().isEmpty() == false)
  {
    QPointF subtitlePos = m_pSubtitleLabel->pos();
    subtitlePos.setX(iconRect.right());
    m_pSubtitleLabel->setPos(subtitlePos);

    subtitleRect = m_pSubtitleLabel->boundingRect();
    subtitleRect.moveTo(m_pSubtitleLabel->pos());
  }

  int h = ezMath::Max(titleRect.bottom(), subtitleRect.bottom()) + 5;

  int y = h;

  // Align inputs
  int maxInputWidth = 10;
  for (ezQtVisualGraphPin* pQtPin : m_Inputs)
  {
    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    maxInputWidth = ezMath::Max(maxInputWidth, (int)rectPin.width());
    y += rectPin.height();
  }

  int maxheight = y;
  y = h;

  // Align outputs
  int maxOutputWidth = 10;
  for (ezQtVisualGraphPin* pQtPin : m_Outputs)
  {
    auto rectPin = pQtPin->GetPinRect();
    pQtPin->setPos(QPointF(-rectPin.x(), y - rectPin.y()));

    maxOutputWidth = ezMath::Max(maxOutputWidth, (int)rectPin.width());
    y += rectPin.height();
  }

  int w = maxInputWidth + maxOutputWidth + 20;

  const int headerWidth = ezMath::Max(titleRect.width(), subtitleRect.width()) + iconRect.width();
  w = ezMath::Max(w, headerWidth);

  maxheight = ezMath::Max(maxheight, y);

  // Align outputs to the right
  for (ezUInt32 i = 0; i < m_Outputs.GetCount(); ++i)
  {
    auto rectPin = m_Outputs[i]->GetPinRect();
    m_Outputs[i]->setX(w - rectPin.width());
  }

  m_HeaderRect = QRectF(-5, -3, w + 10, ezMath::Max(titleRect.bottom(), subtitleRect.bottom()) + 5);

  {
    QPainterPath p;
    p.addRoundedRect(-5, -3, w + 10, maxheight + 10, 5, 5);
    setPath(p);
  }
}

void ezQtVisualGraphNode::UpdateState()
{
  const TitleFormat format;

  ezStringBuilder sTemplate;
  if (!TryGetTitleTemplateFromProperty("CustomTitle", sTemplate) && !TryGetTitleTemplateFromAttribute(sTemplate))
  {
    GetDefaultTitleTemplate(sTemplate);
  }

  ezStringBuilder sTitle;
  ezTokenParseUtils::RenderTemplate(sTemplate, [&](ezStringView sPlaceholder, ezVariant index, bool bOptional, ezStringBuilder& ref_sOutput)
    { ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); },
    sTitle);

  SetTitleAndSubtitle(sTitle, format);
}

bool ezQtVisualGraphNode::TryGetTitleTemplateFromProperty(ezStringView sPropertyName, ezStringBuilder& out_sTemplate)
{
  const ezVariant value = GetObject()->GetTypeAccessor().GetValue(sPropertyName);
  if (!value.IsValid() || !value.CanConvertTo<ezString>())
    return false;

  out_sTemplate = value.ConvertTo<ezString>();
  return !out_sTemplate.IsEmpty();
}

bool ezQtVisualGraphNode::TryGetTitleTemplateFromAttribute(ezStringBuilder& out_sTemplate)
{
  auto pTitleAttribute = GetObject()->GetType()->GetAttributeByType<ezTitleAttribute>();
  if (pTitleAttribute == nullptr)
    return false;

  out_sTemplate = pTitleAttribute->GetTitle();
  return true;
}

void ezQtVisualGraphNode::GetDefaultTitleTemplate(ezStringBuilder& out_sTemplate)
{
  auto& typeAccessor = GetObject()->GetTypeAccessor();

  ezVariant name = typeAccessor.GetValue("Name");
  if (name.IsA<ezString>() && !name.Get<ezString>().IsEmpty())
  {
    out_sTemplate = name.Get<ezString>();
  }
  else
  {
    out_sTemplate = ezTranslate(typeAccessor.GetType()->GetTypeName());
  }
}

void ezQtVisualGraphNode::ResolvePropertyPlaceholder(ezStringView sPlaceholder, const ezVariant& index, bool bOptional, const TitleFormat& format, ezStringBuilder& ref_sOutput)
{
  const ezAbstractProperty* pProp = GetObject()->GetType()->FindPropertyByName(sPlaceholder);
  if (pProp == nullptr)
    return;

  AppendPropertyValue(pProp, index, bOptional, format, ref_sOutput);
}

void ezQtVisualGraphNode::AppendPropertyValue(const ezAbstractProperty* pProp, const ezVariant& index, bool bOptional, const TitleFormat& format, ezStringBuilder& ref_sOutput)
{
  if ((pProp->GetCategory() == ezPropertyCategory::Set || pProp->GetCategory() == ezPropertyCategory::Array) && !index.IsValid())
  {
    ezTempHybridArray<ezVariant, 16> values;
    GetObject()->GetTypeAccessor().GetValues(pProp->GetPropertyName(), values);

    if (bOptional && values.IsEmpty())
      return;

    ezStringBuilder sSet("{");
    for (const auto& setValue : values)
    {
      if (sSet.GetElementCount() > 1)
      {
        sSet.Append(", ");
      }
      sSet.Append(setValue.ConvertTo<ezString>().GetView());
    }
    sSet.Append("}");

    ref_sOutput.Append(sSet.GetView());
    return;
  }

  const ezVariant value = GetObject()->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index);

  if (!value.IsValid())
  {
    if (format.m_bIgnoreInvalidProperties || bOptional)
      return;

    ref_sOutput.Append("<Invalid>");
    return;
  }

  if (bOptional)
  {
    if (value == ezVariant(0))
      return;

    if ((value.IsA<ezString>() || value.IsA<ezHashedString>()) && value.ConvertTo<ezString>().IsEmpty())
      return;
  }


  ezStringBuilder sValue;
  if (pProp->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || pProp->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>())
  {
    ezReflectionUtils::EnumerationToString(pProp->GetSpecificType(), value.ConvertTo<ezInt64>(), sValue);
    sValue = ezTranslate(sValue);
  }
  else if (value.IsA<bool>())
  {
    if (format.m_bBoolsAsTicks)
      sValue.Set(value.Get<bool>() ? "[x]" : "[ ]");
    else
      sValue.Set(value.Get<bool>() ? "true" : "false");
  }
  else if (value.IsA<ezColor>())
  {
    sValue = ezConversionUtils::GetColorName(value.Get<ezColor>());
  }
  else if (value.IsA<ezColorGammaUB>())
  {
    sValue = ezConversionUtils::GetColorName(ezColor(value.Get<ezColorGammaUB>()));
  }
  else if (value.IsA<ezVec2>())
  {
    const ezVec2 v = value.Get<ezVec2>();
    sValue.SetFormat("({}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2));
  }
  else if (value.IsA<ezVec3>())
  {
    const ezVec3 v = value.Get<ezVec3>();
    sValue.SetFormat("({}, {}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2), ezArgF(v.z, 2));
  }
  else if (value.IsA<ezVec4>())
  {
    const ezVec4 v = value.Get<ezVec4>();
    sValue.SetFormat("({}, {}, {}, {})", ezArgF(v.x, 2), ezArgF(v.y, 2), ezArgF(v.z, 2), ezArgF(v.w, 2));
  }
  else if (value.IsA<ezString>() || value.IsA<ezHashedString>())
  {
    sValue = value.ConvertTo<ezString>();

    // asset references are stored as document GUIDs, which are meaningless to the user
    if (ezConversionUtils::IsStringUuid(sValue) && ezToolsProject::GetSingleton() != nullptr)
    {
      const ezStringBuilder sPath = ezToolsProject::GetSingleton()->GetPathForDocumentGuid(ezConversionUtils::ConvertStringToUuid(sValue));

      if (!sPath.IsEmpty())
      {
        sValue = ezPathUtils::GetFileName(sPath);
      }
    }

    sValue.ReplaceAll("\n", " ");
    sValue.ReplaceAll("\t", " ");

    if (format.m_uiMaxStringLength > 0 && sValue.GetCharacterCount() > format.m_uiMaxStringLength)
    {
      sValue.Shrink(0, sValue.GetCharacterCount() - (format.m_uiMaxStringLength - 2));
      sValue.Append("...");
    }

    if (format.m_bQuoteStrings && !sValue.IsEmpty())
    {
      sValue.Prepend("\"");
      sValue.Append("\"");
    }
  }
  else if (value.CanConvertTo<ezString>())
  {
    sValue = value.ConvertTo<ezString>();
  }
  else
  {
    sValue = "<not-implemented>";
  }

  ref_sOutput.Append(sValue.GetView());
}

void ezQtVisualGraphNode::SetTitleAndSubtitle(ezStringView sTitle, const TitleFormat& format)
{
  ezStringBuilder sCleaned = sTitle;

  if (format.m_uiMaxTitleLength > 0 && sCleaned.GetCharacterCount() > format.m_uiMaxTitleLength)
  {
    sCleaned.Shrink(0, sCleaned.GetCharacterCount() - (format.m_uiMaxTitleLength + 1));
    sCleaned.Append("...");
  }

  if (!format.m_bSplitAtDoubleColon)
  {
    m_pTitleLabel->setPlainText(ezMakeQString(sCleaned));
    return;
  }

  if (const char* szSeparator = sCleaned.FindSubString("::"))
  {
    m_pTitleLabel->setPlainText(szSeparator + 2);

    ezStringBuilder sSubTitle = ezStringView(sCleaned.GetData(), szSeparator);
    sSubTitle.Trim("\"");
    m_pSubtitleLabel->setPlainText(ezMakeQString(sSubTitle));
  }
  else
  {
    m_pTitleLabel->setPlainText(ezMakeQString(sCleaned));
    m_pSubtitleLabel->setPlainText(QString());
  }
}

void ezQtVisualGraphNode::SetActive(bool bActive)
{
  if (m_bIsActive != bActive)
  {
    m_bIsActive = bActive;

    for (auto pInputPin : m_Inputs)
    {
      pInputPin->SetActive(bActive);
    }

    for (auto pOutputPin : m_Outputs)
    {
      pOutputPin->SetActive(bActive);
    }
  }

  update();
}

void ezQtVisualGraphNode::CreatePins()
{
  for (auto pQtPin : m_Inputs)
  {
    delete pQtPin;
  }
  m_Inputs.Clear();

  for (auto pQtPin : m_Outputs)
  {
    delete pQtPin;
  }
  m_Outputs.Clear();

  auto inputs = m_pManager->GetInputPins(m_pObject);
  for (auto& pPinTarget : inputs)
  {
    ezQtVisualGraphPin* pQtPin = ezQtVisualGraphScene::GetPinFactory().CreateObject(pPinTarget->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new ezQtVisualGraphPin();
    }
    pQtPin->setParentItem(this);
    m_Inputs.PushBack(pQtPin);

    pQtPin->SetPin(*pPinTarget);
  }

  auto outputs = m_pManager->GetOutputPins(m_pObject);
  for (auto& pPinSource : outputs)
  {
    ezQtVisualGraphPin* pQtPin = ezQtVisualGraphScene::GetPinFactory().CreateObject(pPinSource->GetDynamicRTTI());
    if (pQtPin == nullptr)
    {
      pQtPin = new ezQtVisualGraphPin();
    }

    pQtPin->setParentItem(this);
    m_Outputs.PushBack(pQtPin);

    pQtPin->SetPin(*pPinSource);
  }
}

ezQtVisualGraphPin* ezQtVisualGraphNode::GetInputPin(const ezVisualGraphPin& pin)
{
  for (ezQtVisualGraphPin* pQtPin : m_Inputs)
  {
    if (pQtPin->GetPin() == &pin)
      return pQtPin;
  }
  return nullptr;
}

ezQtVisualGraphPin* ezQtVisualGraphNode::GetOutputPin(const ezVisualGraphPin& pin)
{
  for (ezQtVisualGraphPin* pQtPin : m_Outputs)
  {
    if (pQtPin->GetPin() == &pin)
      return pQtPin;
  }
  return nullptr;
}

ezBitflags<ezQtVisualGraphNodeFlags> ezQtVisualGraphNode::GetFlags() const
{
  return m_DirtyFlags;
}

void ezQtVisualGraphNode::ResetFlags()
{
  m_DirtyFlags = ezQtVisualGraphNodeFlags::UpdateTitle;
}

void ezQtVisualGraphNode::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  if (m_DirtyFlags.IsSet(ezQtVisualGraphNodeFlags::UpdateTitle))
  {
    UpdateState();
    UpdateGeometry();
    m_DirtyFlags.Remove(ezQtVisualGraphNodeFlags::UpdateTitle);
  }

  auto palette = QApplication::palette();

  // Draw background
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(brush());
  painter->drawPath(path());

  QColor headerColor = m_HeaderColor;
  if (!m_bIsActive)
    headerColor.setAlpha(50);

  // Draw separator
  {
    QColor separatorColor = pen().color();
    separatorColor.setAlphaF(headerColor.alphaF() * 0.5f);
    QPen p = pen();
    p.setColor(separatorColor);
    painter->setPen(p);
    painter->drawLine(m_HeaderRect.bottomLeft() + QPointF(2, 0), m_HeaderRect.bottomRight() - QPointF(2, 0));
  }

  // Draw header
  QLinearGradient headerGradient(m_HeaderRect.topLeft(), m_HeaderRect.bottomLeft());
  headerGradient.setColorAt(0.0f, headerColor);
  headerGradient.setColorAt(1.0f, headerColor.darker(120));

  painter->setClipPath(path());
  painter->setPen(QPen(Qt::NoPen));
  painter->setBrush(headerGradient);
  painter->drawRect(m_HeaderRect);
  painter->setClipping(false);

  QColor labelColor;

  // Draw outline
  if (isSelected())
  {
    QPen p = pen();
    p.setColor(palette.highlight().color());
    painter->setPen(p);

    labelColor = ezToQtColor(ezColor::White);
  }
  else
  {
    painter->setPen(pen());

    labelColor = palette.buttonText().color();
  }

  // Label
  if (!m_bIsActive)
    labelColor = labelColor.darker(150);

  const bool bBackgroundIsLight = m_HeaderColor.lightnessF() > 0.6f;
  if (bBackgroundIsLight)
  {
    labelColor.setRed(255 - labelColor.red());
    labelColor.setGreen(255 - labelColor.green());
    labelColor.setBlue(255 - labelColor.blue());
  }

  m_pTitleLabel->setDefaultTextColor(labelColor);
  m_pSubtitleLabel->setDefaultTextColor(labelColor.darker(110));

  painter->setBrush(QBrush(Qt::NoBrush));
  painter->drawPath(path());
}

QVariant ezQtVisualGraphNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
  if (!m_pObject)
    return QGraphicsPathItem::itemChange(change, value);

  ezCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  switch (change)
  {
    case QGraphicsItem::ItemPositionHasChanged:
    {
      if (!pHistory->IsInUndoRedo() && !pHistory->IsInTransaction())
        m_DirtyFlags.Add(ezQtVisualGraphNodeFlags::Moved);
    }
    break;

    default:
      break;
  }
  return QGraphicsPathItem::itemChange(change, value);
}
