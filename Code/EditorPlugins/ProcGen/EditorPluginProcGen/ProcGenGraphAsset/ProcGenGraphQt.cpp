#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphQt.h>


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

void ezQtProcGenNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

  const ezRTTI* pRtti = pObject->GetType();

  if (const ezCategoryAttribute* pAttr = pRtti->GetAttributeByType<ezCategoryAttribute>())
  {
    m_HeaderColor = ezToQtColor(CategoryColor(pAttr->GetCategory()));
  }
}

void ezQtProcGenNode::UpdateState()
{
  ezStringBuilder sTitle;

  const ezRTTI* pRtti = GetObject()->GetType();
  auto& typeAccessor = GetObject()->GetTypeAccessor();

  if (const ezTitleAttribute* pAttr = pRtti->GetAttributeByType<ezTitleAttribute>())
  {
    ezStringBuilder temp;
    ezStringBuilder temp2;

    ezHybridArray<const ezAbstractProperty*, 32> properties;
    pRtti->GetAllProperties(properties);

    sTitle = pAttr->GetTitle();

    for (const auto& pin : GetInputPins())
    {
      temp.Set("{", pin->GetPin()->GetName(), "}");

      if (pin->HasAnyConnections())
      {
        sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
      }
      else
      {
        temp2.Set("{Input", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, temp2);
      }
    }

    ezVariant val;
    ezStringBuilder sVal;
    ezStringBuilder sEnumVal;

    for (const auto& prop : properties)
    {
      if (prop->GetCategory() == ezPropertyCategory::Set)
      {
        sVal = "{";

        ezHybridArray<ezVariant, 16> values;
        typeAccessor.GetValues(prop->GetPropertyName(), values);
        for (auto& setVal : values)
        {
          if (sVal.GetElementCount() > 1)
          {
            sVal.Append(", ");
          }
          sVal.Append(setVal.ConvertTo<ezString>().GetView());
        }

        sVal.Append("}");
      }
      else
      {
        val = typeAccessor.GetValue(prop->GetPropertyName());

        if (prop->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>())
        {
          ezReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<ezInt64>(), sEnumVal);
          sVal = ezTranslate(sEnumVal);
        }
        else if (prop->GetSpecificType() == ezGetStaticRTTI<bool>())
        {
          sVal = val.Get<bool>() ? "[x]" : "[ ]";

          if (ezStringUtils::IsEqual(prop->GetPropertyName(), "Active"))
          {
            SetActive(val.Get<bool>());
          }
        }
        else if (val.CanConvertTo<ezString>())
        {
          sVal = val.ConvertTo<ezString>();
        }
      }

      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);
    }
  }
  else
  {
    sTitle = pRtti->GetTypeName();
    if (sTitle.StartsWith_NoCase("ezProcGen"))
    {
      sTitle.Shrink(9, 0);
    }
  }

  m_pTitleLabel->setPlainText(sTitle.GetData());
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
  if (pEvent->key() == Qt::Key_D || pEvent->key() == Qt::Key_F9)
  {
    SetDebug(!m_bDebug);
  }
}

void ezQtProcGenPin::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
  ezQtPin::paint(pPainter, pOption, pWidget);

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
  QRectF bounds = ezQtPin::boundingRect();
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
  : ezQtNodeScene(pParent)
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

ezStatus ezQtProcGenScene::RemoveNode(ezQtNode* pNode)
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

  return ezQtNodeScene::RemoveNode(pNode);
}
