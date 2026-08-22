#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>


#include <QMenu>
#include <QPainter>

namespace
{
  static ezColorGammaUB CategoryColor(const char* szCategory)
  {
    ezColorScheme::Enum color = ezColorScheme::Green;
    if (ezStringUtils::IsEqual(szCategory, "Input"))
      color = ezColorScheme::Lime;
    else if (ezStringUtils::IsEqual(szCategory, "Output"))
      color = ezColorScheme::Cyan;
    else if (ezStringUtils::IsEqual(szCategory, "Math"))
      color = ezColorScheme::Blue;

    return ezColorScheme::DarkUI(color);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

ezQtProcGenNode::ezQtProcGenNode() = default;

void ezQtProcGenNode::InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject)
{
  ezQtVisualGraphNode::InitNode(pManager, pObject);

  const ezRTTI* pRtti = pObject->GetType();

  if (const ezCategoryAttribute* pAttr = pRtti->GetAttributeByType<ezCategoryAttribute>())
  {
    m_HeaderColor = ezToQtColor(CategoryColor(pAttr->GetCategory()));
  }
}

void ezQtProcGenNode::UpdateState()
{
  TitleFormat format;
  format.m_uiMaxStringLength = 0;
  format.m_bQuoteStrings = false;
  format.m_bSplitAtDoubleColon = false;
  format.m_bBoolsAsTicks = true;

  ezStringBuilder sTemplate;
  if (!TryGetTitleTemplateFromAttribute(sTemplate))
  {
    sTemplate = GetObject()->GetType()->GetTypeName();
    if (sTemplate.StartsWith_NoCase("ezProcGen"))
    {
      sTemplate.Shrink(9, 0);
    }
    sTemplate.TrimLeft("_");
  }

  ezStringBuilder sTitle;
  ezTokenParseUtils::RenderTemplate(sTemplate, [&](ezStringView sPlaceholder, ezVariant index, bool bOptional, ezStringBuilder& ref_sOutput)
    { ResolvePlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); },
    sTitle);

  SetTitleAndSubtitle(sTitle, format);

  ezVariant active = GetObject()->GetTypeAccessor().GetValue("Active");
  if (active.IsA<bool>())
  {
    SetActive(active.Get<bool>());
  }
}

void ezQtProcGenNode::ResolvePlaceholder(ezStringView sPlaceholder, const ezVariant& index, bool bOptional, const TitleFormat& format, ezStringBuilder& ref_sOutput)
{
  for (const auto& pin : GetInputPins())
  {
    if (pin->GetPin()->GetName() != sPlaceholder)
      continue;

    if (pin->HasAnyConnections())
    {
      if (!bOptional)
      {
        ref_sOutput.Append(sPlaceholder);
      }
    }
    else
    {
      // an unconnected pin falls back to the constant stored in the matching 'Input<PinName>' property
      ezStringBuilder sInputProperty("Input", sPlaceholder);
      ResolvePropertyPlaceholder(sInputProperty, index, bOptional, format, ref_sOutput);
    }

    return;
  }

  ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput);
}

//////////////////////////////////////////////////////////////////////////

ezQtProcGenPin::ezQtProcGenPin() = default;
ezQtProcGenPin::~ezQtProcGenPin() = default;

void ezQtProcGenPin::ExtendContextMenu(QMenu& ref_menu)
{
  QAction* pAction = new QAction("Debug", &ref_menu);
  pAction->setCheckable(true);
  pAction->setChecked(m_bDebug);
  pAction->connect(pAction, &QAction::triggered, [this](bool bChecked)
    { SetDebug(bChecked); });

  ref_menu.addAction(pAction);
}

void ezQtProcGenPin::keyPressEvent(QKeyEvent* pEvent)
{
  if (ezQtUtils::IsEquivalentQtKey(pEvent, Qt::Key_D) || pEvent->key() == Qt::Key_F9)
  {
    SetDebug(!m_bDebug);
  }
}

void ezQtProcGenPin::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
  ezQtVisualGraphPin::paint(pPainter, pOption, pWidget);

  pPainter->save();
  pPainter->setPen(QPen(QColor(220, 0, 0), 3.5f, Qt::DotLine));
  pPainter->setBrush(Qt::NoBrush);

  if (m_bDebug)
  {
    float pad = 3.5f;
    QRectF bounds = path().boundingRect().adjusted(-pad, -pad, pad, pad);
    pPainter->drawEllipse(bounds);
  }

  pPainter->restore();
}

QRectF ezQtProcGenPin::boundingRect() const
{
  QRectF bounds = ezQtVisualGraphPin::boundingRect();
  return bounds.adjusted(-6, -6, 6, 6);
}

void ezQtProcGenPin::SetDebug(bool bDebug)
{
  if (m_bDebug != bDebug)
  {
    m_bDebug = bDebug;

    auto pScene = static_cast<ezQtProcGenScene*>(scene());
    pScene->SetDebugPin(bDebug ? this : nullptr);

    update();
  }
}

//////////////////////////////////////////////////////////////////////////

ezQtProcGenScene::ezQtProcGenScene(QObject* pParent /*= nullptr*/)
  : ezQtVisualGraphScene(pParent)
{
}

ezQtProcGenScene::~ezQtProcGenScene() = default;

void ezQtProcGenScene::SetDebugPin(ezQtProcGenPin* pDebugPin)
{
  if (m_pDebugPin == pDebugPin || m_bUpdatingDebugPin)
    return;

  if (m_pDebugPin != nullptr)
  {
    // don't recursively call this function, otherwise the resource is written twice
    // once with debug disabled, then with it enabled, and because it is so quick after each other
    // the resource manager may ignore the second update, because the first one is still ongoing
    m_bUpdatingDebugPin = true;
    m_pDebugPin->SetDebug(false);
    m_bUpdatingDebugPin = false;
  }

  m_pDebugPin = pDebugPin;

  if (ezQtDocumentWindow* window = qobject_cast<ezQtDocumentWindow*>(parent()))
  {
    auto document = static_cast<ezProcGenGraphAssetDocument*>(window->GetDocument());
    document->SetDebugPin(pDebugPin != nullptr ? pDebugPin->GetPin() : nullptr);
  }
}

ezStatus ezQtProcGenScene::RemoveNode(ezQtVisualGraphNode* pNode)
{
  auto pins = pNode->GetInputPins();
  pins.PushBackRange(pNode->GetOutputPins());

  for (auto pPin : pins)
  {
    if (pPin == m_pDebugPin)
    {
      m_pDebugPin->SetDebug(false);
    }
  }

  return ezQtVisualGraphScene::RemoveNode(pNode);
}
