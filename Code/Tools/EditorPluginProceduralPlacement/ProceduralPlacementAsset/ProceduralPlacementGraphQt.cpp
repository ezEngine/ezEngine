#include <EditorPluginProceduralPlacementPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementGraphQt.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>

#include <QPainter>
#include <QTimer>

namespace
{
  static ezColorGammaUB CategoryColor(const char* szCategory)
  {
    if (ezStringUtils::IsEqual(szCategory, "Input"))
      return ezColorGammaUB(38, 105, 0);
    else if (ezStringUtils::IsEqual(szCategory, "Output"))
      return ezColorGammaUB(0, 101, 105);
    else if (ezStringUtils::IsEqual(szCategory, "Math"))
      return ezColorGammaUB(0, 53, 91);

    return ezColor::DarkOliveGreen;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

ezQtProceduralPlacementNode::ezQtProceduralPlacementNode()
{
  // this costs too much performance :-(
  EnableDropShadow(false);
}

void ezQtProceduralPlacementNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

  const ezRTTI* pRtti = pObject->GetType();

  if (const ezCategoryAttribute* pAttr = pRtti->GetAttributeByType<ezCategoryAttribute>())
  {
    ezColorGammaUB color = CategoryColor(pAttr->GetCategory());
    m_HeaderColor = qRgb(color.r, color.g, color.b);
  }
}

void ezQtProceduralPlacementNode::UpdateState()
{
  ezStringBuilder sTitle;

  const ezRTTI* pRtti = GetObject()->GetType();

  if (const ezTitleAttribute* pAttr = pRtti->GetAttributeByType<ezTitleAttribute>())
  {
    ezStringBuilder temp;
    ezStringBuilder temp2;

    ezHybridArray<ezAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

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
    for (const auto& prop : properties)
    {
      val = GetObject()->GetTypeAccessor().GetValue(prop->GetPropertyName());

      if (prop->GetSpecificType()->IsDerivedFrom<ezEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<ezBitflagsBase>())
      {
        ezReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<ezInt64>(), sVal);
        sVal = ezTranslate(sVal);
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

      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);
    }
  }
  else
  {
    sTitle = GetObject()->GetTypeAccessor().GetType()->GetTypeName();
    if (sTitle.StartsWith_NoCase("ezProceduralPlacement"))
    {
      sTitle.Shrink(21, 0);
    }
  }

  m_pLabel->setPlainText(sTitle.GetData());
}

//////////////////////////////////////////////////////////////////////////

ezQtProceduralPlacementPin::ezQtProceduralPlacementPin() = default;

void ezQtProceduralPlacementPin::ExtendContextMenu(QMenu& menu)
{
  QAction* pAction = new QAction("Debug", &menu);
  pAction->setCheckable(true);
  pAction->setChecked(m_bDebug);
  pAction->connect(pAction, &QAction::triggered, [this](bool bChecked) { SetDebug(bChecked); });

  menu.addAction(pAction);
}

void ezQtProceduralPlacementPin::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_D || event->key() == Qt::Key_F9)
  {
    SetDebug(!m_bDebug);
  }
}

void ezQtProceduralPlacementPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  ezQtPin::paint(painter, option, widget);

  painter->save();
  painter->setPen(QPen(QColor(220, 0, 0), 3.5f, Qt::DotLine));
  painter->setBrush(Qt::NoBrush);

  if (m_bDebug)
  {
    float pad = 3.5f;
    QRectF bounds = path().boundingRect().adjusted(-pad, -pad, pad, pad);
    painter->drawEllipse(bounds);
  }

  painter->restore();
}

QRectF ezQtProceduralPlacementPin::boundingRect() const
{
  QRectF bounds = ezQtPin::boundingRect();
  return bounds.adjusted(-6, -6, 6, 6);
}

void ezQtProceduralPlacementPin::SetDebug(bool bDebug)
{
  m_bDebug = bDebug;

  update();
}
