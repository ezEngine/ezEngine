#include <EditorPluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QPainter>
#include <QTimer>
#include <ToolsFoundation/Command/NodeCommands.h>

ezQtVisualScriptAssetScene::ezQtVisualScriptAssetScene(QObject* parent)
    : ezQtNodeScene(parent)
{
}

ezQtVisualScriptAssetScene::~ezQtVisualScriptAssetScene() {}

void ezQtVisualScriptAssetScene::GetAllVsNodes(ezDynamicArray<const ezDocumentObject*>& allNodes) const
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

  ezDynamicArray<const ezDocumentObject*> allNodes;
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

void ezQtVisualScriptAssetScene::ResetActiveConnections(ezDynamicArray<const ezDocumentObject*>& allNodes)
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
  ezDynamicArray<const ezDocumentObject*> allNodes;
  GetAllVsNodes(allNodes);

  ResetActiveConnections(allNodes);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptPin::ezQtVisualScriptPin() {}

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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptConnection::ezQtVisualScriptConnection(QGraphicsItem* parent /*= 0*/) {}


QPen ezQtVisualScriptConnection::DeterminePen() const
{
  if (m_bExecutionHighlight)
  {
    QPen pen(QBrush(qRgb(255, 50, 50)), 4, Qt::DashLine);
    return pen;
  }

  return ezQtConnection::DeterminePen();
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

  const ezVisualScriptNodeDescriptor* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType());

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

void ezQtVisualScriptNode::UpdateState()
{
  ezStringBuilder sTitle;

  const ezVisualScriptNodeDescriptor* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());

  if (pDesc == nullptr)
    return;

  if (!pDesc->m_sTitle.IsEmpty())
  {
    ezStringBuilder temp;

    ezHybridArray<ezAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

    sTitle = pDesc->m_sTitle;

    for (const auto& pin : GetInputPins())
    {
      if (pin->HasAnyConnections())
      {
        temp.Set("{", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
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
    if (sTitle.StartsWith_NoCase("VisualScriptNode::"))
      sTitle.Shrink(18, 0);

    if (sTitle.StartsWith_NoCase("ezVisualScriptNode_"))
      sTitle.Shrink(19, 0);
  }

  m_pLabel->setPlainText(sTitle.GetData());
}
