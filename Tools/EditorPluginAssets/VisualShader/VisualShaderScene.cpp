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
  if (res.m_Result.Succeeded())
  {
    ezQtNodeScene::ConnectPinsAction(pSourcePin, pTargetPin);
    return;
  }

  const ezVisualShaderPin* pPinSource = ezDynamicCast<const ezVisualShaderPin*>(pSourcePin);
  const ezVisualShaderPin* pPinTarget = ezDynamicCast<const ezVisualShaderPin*>(pTargetPin);
  //if (pPinSource->GetDataType() != pPinTarget->GetDataType())
  //{
  //  res = ezStatus("Incompatible data types");
  //  ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node connect failed.");
  //  return;
  //}

  if (!pTargetPin->GetConnections().IsEmpty())
  {
    // If we already have a connection at this input pin, delete it.
    ezCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Replace Input Pin");

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

    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node connect failed.");
  }
}


ezQtVisualShaderPin::ezQtVisualShaderPin()
{
}


void ezQtVisualShaderPin::SetPin(const ezPin* pPin)
{
  ezQtPin::SetPin(pPin);

  const ezVisualShaderPin* pShaderPin = ezDynamicCast<const ezVisualShaderPin*>(pPin);
  EZ_ASSERT_DEV(pShaderPin != nullptr, "Invalid pin type");

  setToolTip(pShaderPin->GetDataType()->GetTypeName());

  const ezColorGammaUB col = pShaderPin->GetColor();

  QPen p = pen();
  p.setColor(qRgb(col.r, col.g, col.b));
  setPen(p);
}

void ezQtVisualShaderPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  QPainterPath p = path();

  painter->save();
  painter->setBrush(brush());
  painter->setPen(pen());
  painter->drawPath(p);
  painter->restore();
}

void ezQtVisualShaderPin::ConnectedStateChanged(bool bConnected)
{
  if (bConnected)
  {
    setBrush(pen().color().darker(140));
  }
  else
  {
    auto palette = QApplication::palette();
    setBrush(palette.base());
  }
}

ezQtVisualShaderNode::ezQtVisualShaderNode()
{

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
    ezLog::Error("Could not initialize node type, node descriptor is invalid");
  }
}

