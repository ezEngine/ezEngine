#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <EditorFramework/EngineProcess/EngineProcessMessages.h>
#include <QPainter>
#include <QTimer>

ezQtVisualScriptAssetScene::ezQtVisualScriptAssetScene(QObject* parent)
  : ezQtNodeScene(parent)
{
}

ezQtVisualScriptAssetScene::~ezQtVisualScriptAssetScene()
{
}

void ezQtVisualScriptAssetScene::GetAllVsNodes(ezDynamicArray<const ezDocumentObject *>& allNodes) const
{
  ezVisualScriptTypeRegistry* pTypeRegistry = ezVisualScriptTypeRegistry::GetSingleton();
  const ezRTTI* pNodeBaseRtti = pTypeRegistry->GetNodeBaseType();

  allNodes.Clear();
  allNodes.Reserve(64);

  const auto& children = GetDocumentNodeManager()->GetRootObject()->GetChildren();
  for (const ezDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    if (!pType->IsDerivedFrom(pNodeBaseRtti))
      continue;

    allNodes.PushBack(pObject);
  }
}

void ezQtVisualScriptAssetScene::VisualScriptActivityEventHandler(const ezVisualScriptActivityEvent& ae)
{
  // ignore activity from other objects
  if (ae.m_ObjectGuid != m_DebugObject)
    return;

  const ezVisualScriptInstanceActivity* pActivity = ae.m_pActivityData;

  const ezDocumentNodeManager* pNodeManager = GetDocumentNodeManager();

  ezDynamicArray<const ezDocumentObject *> allNodes;
  GetAllVsNodes(allNodes);

  const ezTime tNow = ezTime::Now();
  const ezTime tHighlight = tNow + ezTime::Milliseconds(300);

  for (auto act : pActivity->m_ActiveExecutionConnections)
  {
    const ezUInt32 uiNode = act >> 16;
    const ezUInt32 uiPin = act & 0x0000FFFF;

    if (uiNode >= allNodes.GetCount())
      continue;

    const ezDocumentObject* pSrcObject = allNodes[uiNode];
    ezQtNode* pQtNode = m_Nodes[pSrcObject];

    if (pQtNode == nullptr)
      continue;

    const ezPin* pFoundPin = nullptr;
    {
      const ezArrayPtr<ezPin* const> outputPins = pNodeManager->GetOutputPins(pSrcObject);

      for (auto pSearchPin : outputPins)
      {
        if (static_cast<ezVisualScriptPin*>(pSearchPin)->GetDescriptor()->m_uiPinIndex == uiPin)
        {
          pFoundPin = pSearchPin;
          break;
        }
      }

      if (pFoundPin == nullptr)
        continue;
    }

    ezQtPin* pQtPin = pQtNode->GetOutputPin(pFoundPin);

    const ezArrayPtr<ezQtConnection*> connectionsOut = pQtPin->GetConnections();

    for (auto pQtCon : connectionsOut)
    {
      ezQtVisualScriptConnection* pVsCon = static_cast<ezQtVisualScriptConnection*>(pQtCon);

      pVsCon->m_HighlightUntil = tHighlight;

      if (!pVsCon->m_bExecutionHighlight)
      {
        pVsCon->m_bExecutionHighlight = true;

        pVsCon->UpdateConnection();
        pVsCon->update();
      }
    }
  }

  ResetActiveConnections(allNodes);
}


void ezQtVisualScriptAssetScene::VisualScriptInterDocumentMessageHandler(ezReflectedClass* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezGatherObjectsForDebugVisMsgInterDoc>())
  {
    if (m_DebugObject.IsValid())
    {
      ezGatherObjectsForDebugVisMsgInterDoc* pMessage = static_cast<ezGatherObjectsForDebugVisMsgInterDoc*>(pMsg);
      pMessage->m_Objects.PushBack(m_DebugObject);
    }
  }
}

void ezQtVisualScriptAssetScene::SetDebugObject(const ezUuid& objectGuid)
{
  m_DebugObject = objectGuid;
}

void ezQtVisualScriptAssetScene::ResetActiveConnections(ezDynamicArray<const ezDocumentObject *> &allNodes)
{
  const ezDocumentNodeManager* pNodeManager = GetDocumentNodeManager();

  const ezTime tNow = ezTime::Now();
  bool bUpdateAgain = false;

  // reset all connections that are not active anymore
  for (ezUInt32 srcNodeIdx = 0; srcNodeIdx < allNodes.GetCount(); ++srcNodeIdx)
  {
    const ezDocumentObject* pSrcObject = allNodes[srcNodeIdx];
    ezQtNode* pQtNode = m_Nodes[pSrcObject];

    if (pQtNode == nullptr)
      continue;

    const ezArrayPtr<ezPin* const> outputPins = pNodeManager->GetOutputPins(pSrcObject);

    for (auto pPin : outputPins)
    {
      ezQtPin* pQtPin = pQtNode->GetOutputPin(pPin);

      const ezArrayPtr<ezQtConnection*> connectionsOut = pQtPin->GetConnections();

      for (auto pQtCon : connectionsOut)
      {
        ezQtVisualScriptConnection* pVsCon = static_cast<ezQtVisualScriptConnection*>(pQtCon);

        if (pVsCon->m_bExecutionHighlight)
        {
          if (pVsCon->m_HighlightUntil <= tNow)
          {
            pVsCon->m_bExecutionHighlight = false;

            pVsCon->UpdateConnection();
          }

          bUpdateAgain = true;
          pVsCon->update();
        }
      }
    }
  }

  if (bUpdateAgain)
  {
    QTimer::singleShot(100, this, SLOT(OnUpdateDisplay()));
  }
}

void ezQtVisualScriptAssetScene::OnUpdateDisplay()
{
  ezDynamicArray<const ezDocumentObject *> allNodes;
  GetAllVsNodes(allNodes);

  ResetActiveConnections(allNodes);
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
  if (m_bExecutionHighlight)
  {
    QPen pen(QBrush(qRgb(255, 50, 50)), 4, Qt::DashLine);
    return pen;
  }

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

void ezQtVisualScriptNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

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

void ezQtVisualScriptNode::UpdateTitle()
{
  ezStringBuilder temp;
  
  const auto* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());

  if (pDesc == nullptr)
    return;

  if (!pDesc->m_sTitle.IsEmpty())
  {
    ezHybridArray<ezAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

    ezVariant values[4];
    for (ezUInt32 i = 0; i < ezMath::Min(4U, properties.GetCount()); ++i)
    {
      values[i] = GetObject()->GetTypeAccessor().GetValue(properties[i]->GetPropertyName());
    }

    temp.Format(pDesc->m_sTitle, values[0].ConvertTo<ezString>(), values[1].ConvertTo<ezString>(), values[2].ConvertTo<ezString>(), values[3].ConvertTo<ezString>());
  }
  else
  {
    temp = GetObject()->GetTypeAccessor().GetType()->GetTypeName();
    if (temp.StartsWith_NoCase("VisualScriptNode::"))
      temp.Shrink(18, 0);

    if (temp.StartsWith_NoCase("ezVisualScriptNode_"))
      temp.Shrink(19, 0);
  }

  m_pLabel->setPlainText(temp.GetData());
}

