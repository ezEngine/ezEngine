#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QPainter>

ezQtVisualScriptAssetScene::ezQtVisualScriptAssetScene(QObject* parent)
  : ezQtNodeScene(parent)
{
}

ezQtVisualScriptAssetScene::~ezQtVisualScriptAssetScene()
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptPin::ezQtVisualScriptPin()
{
}

void ezQtVisualScriptPin::SetPin(const ezPin* pPin)
{
  ezQtPin::SetPin(pPin);

  const ezVisualScriptPin* pShaderPin = ezDynamicCast<const ezVisualScriptPin*>(pPin);
  EZ_ASSERT_DEV(pShaderPin != nullptr, "Invalid pin type");

  ezStringBuilder sTooltip;

  if (!pShaderPin->GetTooltip().IsEmpty())
  {
    sTooltip = pShaderPin->GetTooltip();
  }
  else
  {
    sTooltip = pShaderPin->GetName();
  }

  setToolTip(sTooltip.GetData());

  const ezColorGammaUB col = pShaderPin->GetColor();

  QPen p = pen();
  p.setColor(qRgb(col.r, col.g, col.b));
  setPen(p);
}

void ezQtVisualScriptPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  const ezVisualScriptPin* pVsPin = static_cast<const ezVisualScriptPin*>(GetPin());

  painter->save();
  painter->setBrush(brush());
  painter->setPen(pen());

  if (pVsPin->GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::Data)
  {
      painter->drawRect(this->path().boundingRect());
  }
  else
  {
    QPainterPath p = path();
    painter->drawPath(p);
  }

  painter->restore();
}

void ezQtVisualScriptPin::ConnectedStateChanged(bool bConnected)
{
  if (bConnected)
  {
    setBrush(pen().color());
  }
  else
  {
    setBrush(QApplication::palette().base());
  }
}

bool ezQtVisualScriptPin::AdjustRenderingForHighlight(ezQtPinHighlightState state)
{
  const ezVisualScriptPin* pShaderPin = ezDynamicCast<const ezVisualScriptPin*>(GetPin());
  EZ_ASSERT_DEV(pShaderPin != nullptr, "Invalid pin type");

  const ezColorGammaUB col = pShaderPin->GetColor();

  switch (state)
  {
  case ezQtPinHighlightState::None:
    {
      QPen p = pen();
      p.setColor(qRgb(col.r, col.g, col.b));
      setPen(p);

      setBrush(GetConnections().IsEmpty() ? QApplication::palette().base() : pen().color());
    }
    break;

  case ezQtPinHighlightState::CannotConnect:
  case ezQtPinHighlightState::CannotConnectSameDirection:
    {
      QPen p = pen();
      p.setColor(QApplication::palette().base().color().lighter());
      setPen(p);

      setBrush(QApplication::palette().base());
    }
    break;

  case ezQtPinHighlightState::CanReplaceConnection:
  case ezQtPinHighlightState::CanAddConnection:
    {
      QPen p = pen();
      p.setColor(qRgb(col.r, col.g, col.b));
      setPen(p);

      setBrush(QApplication::palette().base());
    }
    break;
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptConnection::ezQtVisualScriptConnection(QGraphicsItem* parent /*= 0*/)
{

}


QPen ezQtVisualScriptConnection::DeterminePen() const
{
  const ezVisualScriptPin* pSourcePin = static_cast<const ezVisualScriptPin*>(m_pConnection->GetSourcePin());
  const ezVisualScriptPin* pTargetPin = static_cast<const ezVisualScriptPin*>(m_pConnection->GetTargetPin());

  ezColorGammaUB color;
  const ezColorGammaUB color1 = pSourcePin->GetDescriptor()->m_Color;
  const ezColorGammaUB color2 = pTargetPin->GetDescriptor()->m_Color;

  const bool isGrey1 = (color1.r == color1.g && color1.r == color1.b);
  const bool isGrey2 = (color2.r == color2.g && color2.r == color2.b);

  if (!isGrey1)
  {
    color = ezMath::Lerp(color1, color2, 0.2f);
  }
  else if (!isGrey2)
  {
    color = ezMath::Lerp(color1, color2, 0.8f);
  }
  else
  {
    color = ezMath::Lerp(color1, color2, 0.5f);
  }

  auto palette = QApplication::palette();
  QPen pen(QBrush(qRgb(color.r, color.g, color.b)), 3, Qt::SolidLine);

  return pen;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


ezQtVisualScriptNode::ezQtVisualScriptNode()
{
  // this costs too much performance :-(
  EnableDropShadow(false);
}

void ezQtVisualScriptNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject, const char* szHeaderText /*= nullptr*/)
{
  ezStringBuilder temp = pObject->GetTypeAccessor().GetType()->GetTypeName();
  if (temp.StartsWith_NoCase("VisualScriptNode::"))
    temp.Shrink(18, 0);

  if (temp.StartsWith_NoCase("ezVisualScriptNode_"))
    temp.Shrink(19, 0);

  ezQtNode::InitNode(pManager, pObject, temp);

  const auto* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc != nullptr)
  {
    m_HeaderColor = qRgb(pDesc->m_Color.r, pDesc->m_Color.g, pDesc->m_Color.b);
  }
  else
  {
    m_HeaderColor = qRgb(255, 0, 0);
    ezLog::Error("Could not initialize node type, node descriptor is invalid");
  }
}

