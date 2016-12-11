#include <PCH.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <QPainter>


ezQtVisualShaderScene::ezQtVisualShaderScene(QObject* parent)
  : ezQtNodeScene(parent)
{
}

ezQtVisualShaderScene::~ezQtVisualShaderScene()
{
}

void ezQtVisualShaderScene::ConnectPinsAction(const ezPin* pSourcePin, const ezPin* pTargetPin)
{
  ezStatus res = m_pManager->CanConnect(pSourcePin, pTargetPin);

  if (res.Succeeded())
  {
    const ezVisualShaderPin* pPinSource = ezDynamicCast<const ezVisualShaderPin*>(pSourcePin);
    const ezVisualShaderPin* pPinTarget = ezDynamicCast<const ezVisualShaderPin*>(pTargetPin);

    ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Connect Pins");

    // If we already have a connection at this input pin, delete it.
    if (!pTargetPin->GetConnections().IsEmpty())
    {
      const ezArrayPtr<const ezConnection* const> connections = pTargetPin->GetConnections();
      EZ_ASSERT_DEV(connections.GetCount() == 1, "A render pipeline should only support one input connection at a time.");
      const ezConnection* pConnection = connections[0];

      ezDisconnectNodePinsCommand cmd;
      cmd.m_ObjectSource = pConnection->GetSourcePin()->GetParent()->GetGuid();
      cmd.m_ObjectTarget = pConnection->GetTargetPin()->GetParent()->GetGuid();
      cmd.m_sSourcePin = pConnection->GetSourcePin()->GetName();
      cmd.m_sTargetPin = pConnection->GetTargetPin()->GetName();

      res = history->AddCommand(cmd);
    }

    // so far so good, try connecting
    if (res.Succeeded())
    {
      ezConnectNodePinsCommand cmd;
      cmd.m_ObjectSource = pSourcePin->GetParent()->GetGuid();
      cmd.m_ObjectTarget = pTargetPin->GetParent()->GetGuid();
      cmd.m_sSourcePin = pSourcePin->GetName();
      cmd.m_sTargetPin = pTargetPin->GetName();

      res = history->AddCommand(cmd);
    }

    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();
  }

  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to connect nodes.");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


ezQtVisualShaderPin::ezQtVisualShaderPin()
{
}


void ezQtVisualShaderPin::SetPin(const ezPin* pPin)
{
  ezQtPin::SetPin(pPin);

  const ezVisualShaderPin* pShaderPin = ezDynamicCast<const ezVisualShaderPin*>(pPin);
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

  if (!pShaderPin->GetDescriptor()->m_sDefaultValue.IsEmpty())
  {
    if (!sTooltip.IsEmpty())
      sTooltip.Append("\n");

    sTooltip.Append("Default is ", pShaderPin->GetDescriptor()->m_sDefaultValue);
  }

  setToolTip(sTooltip.GetData());

  const ezColorGammaUB col = pShaderPin->GetColor();

  QPen p = pen();
  p.setColor(qRgb(col.r, col.g, col.b));
  setPen(p);
}

void ezQtVisualShaderPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  QPainterPath p = path();

  const ezVisualShaderPin* pVsPin = static_cast<const ezVisualShaderPin*>(GetPin());

  painter->save();
  painter->setBrush(brush());
  painter->setPen(pen());

  if (pVsPin->GetType() == ezPin::Type::Input && GetConnections().IsEmpty())
  {
    if (pVsPin->GetDescriptor()->m_sDefaultValue.IsEmpty())
    {
      // this pin MUST be connected

      QPen p;
      p.setColor(qRgb(255, 0, 0));
      p.setWidth(3);
      p.setCosmetic(true);
      p.setStyle(Qt::PenStyle::SolidLine);
      p.setCapStyle(Qt::PenCapStyle::SquareCap);

      painter->setPen(p);

      painter->drawRect(this->path().boundingRect());
      painter->restore();
      return;
    }
  }

  painter->drawPath(p);
  painter->restore();
}

void ezQtVisualShaderPin::ConnectedStateChanged(bool bConnected)
{
  if (bConnected)
  {
    setBrush(pen().color());
  }
  else
  {
    auto palette = QApplication::palette();
    setBrush(palette.base());
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualShaderConnection::ezQtVisualShaderConnection(QGraphicsItem* parent /*= 0*/)
{

}


QPen ezQtVisualShaderConnection::DeterminePen() const
{
  const ezVisualShaderPin* pSourcePin = static_cast<const ezVisualShaderPin*>(m_pConnection->GetSourcePin());
  const ezVisualShaderPin* pTargetPin = static_cast<const ezVisualShaderPin*>(m_pConnection->GetTargetPin());

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


ezQtVisualShaderNode::ezQtVisualShaderNode()
{
  // this costs too much performance :-(
  EnableDropShadow(false);
}

void ezQtVisualShaderNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

  ezStringBuilder temp = pObject->GetTypeAccessor().GetType()->GetTypeName();
  if (temp.StartsWith_NoCase("ShaderNode::"))
    temp.Shrink(12, 0);
  m_pLabel->setPlainText(temp.GetData());

  const auto* pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

  if (pDesc != nullptr)
  {
    m_HeaderColor = qRgb(pDesc->m_Color.r, pDesc->m_Color.g, pDesc->m_Color.b);
  }
  else
  {
    m_HeaderColor = qRgb(255, 0, 0);
    ezLog::ErrorPrintf("Could not initialize node type, node descriptor is invalid");
  }
}

