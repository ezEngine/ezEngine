#include <EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerGraphQt.h>

//#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
//#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
//#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
//#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
//#include <Foundation/Strings/StringBuilder.h>
//#include <Foundation/Strings/TranslationLookup.h>
//#include <GameEngine/VisualScript/VisualScriptInstance.h>
//#include <GuiFoundation/UIServices/UIServices.moc.h>
//#include <QPainter>
//#include <QTimer>
//#include <ToolsFoundation/Command/NodeCommands.h>
//
//ezQtVisualScriptAssetScene::ezQtVisualScriptAssetScene(QObject* parent)
//  : ezQtNodeScene(parent)
//{
//}
//
//ezQtVisualScriptAssetScene::~ezQtVisualScriptAssetScene() {}
//
//void ezQtVisualScriptAssetScene::GetAllVsNodes(ezDynamicArray<const ezDocumentObject*>& allNodes) const
//{
//  ezVisualScriptTypeRegistry* pTypeRegistry = ezVisualScriptTypeRegistry::GetSingleton();
//  const ezRTTI* pNodeBaseRtti = pTypeRegistry->GetNodeBaseType();
//
//  allNodes.Clear();
//  allNodes.Reserve(64);
//
//  const auto& children = GetDocumentNodeManager()->GetRootObject()->GetChildren();
//  for (const ezDocumentObject* pObject : children)
//  {
//    auto pType = pObject->GetTypeAccessor().GetType();
//    if (!pType->IsDerivedFrom(pNodeBaseRtti))
//      continue;
//
//    allNodes.PushBack(pObject);
//  }
//}
//
//void ezQtVisualScriptAssetScene::VisualScriptActivityEventHandler(const ezVisualScriptActivityEvent& ae)
//{
//  // ignore activity from other objects
//  if (ae.m_ObjectGuid != m_DebugObject)
//    return;
//
//  const ezVisualScriptInstanceActivity* pActivity = ae.m_pActivityData;
//
//  const ezDocumentNodeManager* pNodeManager = GetDocumentNodeManager();
//
//  ezDynamicArray<const ezDocumentObject*> allNodes;
//  GetAllVsNodes(allNodes);
//
//  const ezTime tNow = ezTime::Now();
//  const ezTime tHighlight = tNow + ezTime::Milliseconds(300);
//
//  for (auto act : pActivity->m_ActiveExecutionConnections)
//  {
//    const ezUInt32 uiNode = act >> 16;
//    const ezUInt32 uiPin = act & 0x0000FFFF;
//
//    if (uiNode >= allNodes.GetCount())
//      continue;
//
//    const ezDocumentObject* pSrcObject = allNodes[uiNode];
//    ezQtNode* pQtNode = m_Nodes[pSrcObject];
//
//    if (pQtNode == nullptr)
//      continue;
//
//    const ezPin* pFoundPin = nullptr;
//    {
//      const ezArrayPtr<ezPin* const> outputPins = pNodeManager->GetOutputPins(pSrcObject);
//
//      for (auto pSearchPin : outputPins)
//      {
//        if (static_cast<ezVisualScriptPin*>(pSearchPin)->GetDescriptor()->m_uiPinIndex == uiPin)
//        {
//          pFoundPin = pSearchPin;
//          break;
//        }
//      }
//
//      if (pFoundPin == nullptr)
//        continue;
//    }
//
//    ezQtPin* pQtPin = pQtNode->GetOutputPin(pFoundPin);
//
//    const ezArrayPtr<ezQtConnection*> connectionsOut = pQtPin->GetConnections();
//
//    for (auto pQtCon : connectionsOut)
//    {
//      ezQtVisualScriptConnection* pVsCon = static_cast<ezQtVisualScriptConnection*>(pQtCon);
//
//      pVsCon->m_HighlightUntil = tHighlight;
//
//      if (!pVsCon->m_bExecutionHighlight)
//      {
//        pVsCon->m_bExecutionHighlight = true;
//
//        pVsCon->UpdateConnection();
//        pVsCon->update();
//      }
//    }
//  }
//
//  ResetActiveConnections(allNodes);
//}
//
//
//void ezQtVisualScriptAssetScene::VisualScriptInterDocumentMessageHandler(ezReflectedClass* pMsg)
//{
//  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezGatherObjectsForDebugVisMsgInterDoc>())
//  {
//    if (m_DebugObject.IsValid())
//    {
//      ezGatherObjectsForDebugVisMsgInterDoc* pMessage = static_cast<ezGatherObjectsForDebugVisMsgInterDoc*>(pMsg);
//      pMessage->m_Objects.PushBack(m_DebugObject);
//    }
//  }
//}
//
//void ezQtVisualScriptAssetScene::SetDebugObject(const ezUuid& objectGuid)
//{
//  m_DebugObject = objectGuid;
//}
//
//void ezQtVisualScriptAssetScene::ResetActiveConnections(ezDynamicArray<const ezDocumentObject*>& allNodes)
//{
//  const ezDocumentNodeManager* pNodeManager = GetDocumentNodeManager();
//
//  const ezTime tNow = ezTime::Now();
//  bool bUpdateAgain = false;
//
//  // reset all connections that are not active anymore
//  for (ezUInt32 srcNodeIdx = 0; srcNodeIdx < allNodes.GetCount(); ++srcNodeIdx)
//  {
//    const ezDocumentObject* pSrcObject = allNodes[srcNodeIdx];
//    ezQtNode* pQtNode = m_Nodes[pSrcObject];
//
//    if (pQtNode == nullptr)
//      continue;
//
//    const ezArrayPtr<ezPin* const> outputPins = pNodeManager->GetOutputPins(pSrcObject);
//
//    for (auto pPin : outputPins)
//    {
//      ezQtPin* pQtPin = pQtNode->GetOutputPin(pPin);
//
//      const ezArrayPtr<ezQtConnection*> connectionsOut = pQtPin->GetConnections();
//
//      for (auto pQtCon : connectionsOut)
//      {
//        ezQtVisualScriptConnection* pVsCon = static_cast<ezQtVisualScriptConnection*>(pQtCon);
//
//        if (pVsCon->m_bExecutionHighlight)
//        {
//          if (pVsCon->m_HighlightUntil <= tNow)
//          {
//            pVsCon->m_bExecutionHighlight = false;
//
//            pVsCon->UpdateConnection();
//          }
//
//          bUpdateAgain = true;
//          pVsCon->update();
//        }
//      }
//    }
//  }
//
//  if (bUpdateAgain)
//  {
//    QTimer::singleShot(100, this, SLOT(OnUpdateDisplay()));
//  }
//}
//
//void ezQtVisualScriptAssetScene::OnUpdateDisplay()
//{
//  ezDynamicArray<const ezDocumentObject*> allNodes;
//  GetAllVsNodes(allNodes);
//
//  ResetActiveConnections(allNodes);
//}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//ezQtVisualScriptPin::ezQtVisualScriptPin() {}
//
//void ezQtVisualScriptPin::SetPin(const ezPin* pPin)
//{
//  ezQtPin::SetPin(pPin);
//
//  const ezVisualScriptPin* pShaderPin = ezDynamicCast<const ezVisualScriptPin*>(pPin);
//  EZ_ASSERT_DEV(pShaderPin != nullptr, "Invalid pin type");
//
//  ezStringBuilder sTooltip;
//
//  if (!pShaderPin->GetTooltip().IsEmpty())
//  {
//    sTooltip = pShaderPin->GetTooltip();
//  }
//  else
//  {
//    sTooltip = pShaderPin->GetName();
//  }
//
//  setToolTip(sTooltip.GetData());
//}
//
//void ezQtVisualScriptPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
//{
//  const ezVisualScriptPin* pVsPin = static_cast<const ezVisualScriptPin*>(GetPin());
//
//  painter->save();
//  painter->setBrush(brush());
//  painter->setPen(pen());
//
//  if (pVsPin->GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Data)
//  {
//    painter->drawRect(this->path().boundingRect());
//  }
//  else
//  {
//    QPainterPath p = path();
//    painter->drawPath(p);
//  }
//
//  painter->restore();
//}
//
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//
//ezQtVisualScriptConnection::ezQtVisualScriptConnection(QGraphicsItem* parent /*= 0*/) {}
//
//
//QPen ezQtVisualScriptConnection::DeterminePen() const
//{
//  if (m_bExecutionHighlight)
//  {
//    QPen pen(QBrush(qRgb(255, 50, 50)), 4, Qt::DashLine);
//    return pen;
//  }
//
//  return ezQtConnection::DeterminePen();
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


ezQtAnimationControllerNode::ezQtAnimationControllerNode()
{
  // this costs too much performance :-(
  EnableDropShadow(false);
}

void ezQtAnimationControllerNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

  if (const ezColorAttribute* pAttr = pObject->GetType()->GetAttributeByType<ezColorAttribute>())
  {
    ezColorGammaUB c = pAttr->GetColor();
    m_HeaderColor = qRgb(c.r, c.g, c.b);
  }
}

void ezQtAnimationControllerNode::UpdateState()
{
  ezQtNode::UpdateState();

  ezStringBuilder sTitle;

  const ezRTTI* pRtti = GetObject()->GetType();

  ezVariant customTitle = GetObject()->GetTypeAccessor().GetValue("CustomTitle");
  if (customTitle.IsValid() && customTitle.CanConvertTo<ezString>())
  {
    sTitle = customTitle.ConvertTo<ezString>();
  }

  if (sTitle.IsEmpty())
  {
    if (const ezTitleAttribute* pAttr = pRtti->GetAttributeByType<ezTitleAttribute>())
    {
      sTitle = pAttr->GetTitle();

      ezHybridArray<ezAbstractProperty*, 32> properties;
      pRtti->GetAllProperties(properties);

      ezVariant val;
      ezStringBuilder sVal, temp;
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

        if (ezConversionUtils::IsStringUuid(sVal))
        {
          if (auto pAsset = ezAssetCurator::GetSingleton()->FindSubAsset(sVal))
          {
            sVal = pAsset->GetName();
          }
        }

        temp.Set("{", prop->GetPropertyName(), "}");
        sTitle.ReplaceAll(temp, sVal);
      }
    }
  }

  sTitle.ReplaceAll("''", "");
  sTitle.ReplaceAll("\"\"", "");
  sTitle.ReplaceAll("  ", " ");
  sTitle.Trim(" ");

  if (!sTitle.IsEmpty())
  {
    m_pLabel->setPlainText(sTitle.GetData());
  }
}
