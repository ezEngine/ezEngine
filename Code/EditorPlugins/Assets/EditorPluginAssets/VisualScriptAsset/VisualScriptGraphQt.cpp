#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraphQt.moc.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

ezQtVisualScriptAssetScene::ezQtVisualScriptAssetScene(QObject* pParent)
  : ezQtNodeScene(pParent)
{
}

ezQtVisualScriptAssetScene::~ezQtVisualScriptAssetScene() = default;

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
  const ezTime tHighlight = tNow + ezTime::MakeFromMilliseconds(300);

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
      auto outputPins = pNodeManager->GetOutputPins(pSrcObject);

      for (auto& pSearchPin : outputPins)
      {
        if (static_cast<const ezVisualScriptPin_Legacy&>(*pSearchPin).GetDescriptor()->m_uiPinIndex == uiPin)
        {
          pFoundPin = pSearchPin.Borrow();
          break;
        }
      }

      if (pFoundPin == nullptr)
        continue;
    }

    ezQtPin* pQtPin = pQtNode->GetOutputPin(*pFoundPin);

    const ezArrayPtr<ezQtConnection*> connectionsOut = pQtPin->GetConnections();

    for (auto pQtCon : connectionsOut)
    {
      ezQtVisualScriptConnection_Legacy* pVsCon = static_cast<ezQtVisualScriptConnection_Legacy*>(pQtCon);

      pVsCon->m_HighlightUntil = tHighlight;

      if (!pVsCon->m_bExecutionHighlight)
      {
        pVsCon->m_bExecutionHighlight = true;
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

    auto outputPins = pNodeManager->GetOutputPins(pSrcObject);

    for (auto& pPin : outputPins)
    {
      ezQtPin* pQtPin = pQtNode->GetOutputPin(*pPin);

      const ezArrayPtr<ezQtConnection*> connectionsOut = pQtPin->GetConnections();

      for (auto pQtCon : connectionsOut)
      {
        ezQtVisualScriptConnection_Legacy* pVsCon = static_cast<ezQtVisualScriptConnection_Legacy*>(pQtCon);

        if (pVsCon->m_bExecutionHighlight)
        {
          if (pVsCon->m_HighlightUntil <= tNow)
          {
            pVsCon->m_bExecutionHighlight = false;
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

ezQtVisualScriptPin_Legacy::ezQtVisualScriptPin_Legacy() = default;

void ezQtVisualScriptPin_Legacy::SetPin(const ezPin& pin)
{
  ezQtPin::SetPin(pin);

  const ezVisualScriptPin_Legacy& vsPin = ezStaticCast<const ezVisualScriptPin_Legacy&>(pin);

  ezStringBuilder sTooltip;
  if (!vsPin.GetTooltip().IsEmpty())
  {
    sTooltip = vsPin.GetTooltip();
  }
  else
  {
    sTooltip = vsPin.GetName();

    if (vsPin.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Data)
    {
      ezStringBuilder sDataType;
      if (ezReflectionUtils::EnumerationToString(ezGetStaticRTTI<ezVisualScriptDataPinType>(), vsPin.GetDescriptor()->m_DataType, sDataType, ezReflectionUtils::EnumConversionMode::ValueNameOnly))
      {
        sTooltip.Append(": ", sDataType);
      }
    }
  }

  setToolTip(sTooltip.GetData());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptConnection_Legacy::ezQtVisualScriptConnection_Legacy(QGraphicsItem* pParent /*= 0*/) {}


QPen ezQtVisualScriptConnection_Legacy::DeterminePen() const
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

ezQtVisualScriptNode_Legacy::ezQtVisualScriptNode_Legacy() = default;

void ezQtVisualScriptNode_Legacy::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
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

void ezQtVisualScriptNode_Legacy::UpdateState()
{
  ezStringBuilder sTitle;

  const ezVisualScriptNodeDescriptor* pDesc = ezVisualScriptTypeRegistry::GetSingleton()->GetDescriptorForType(GetObject()->GetType());

  if (pDesc == nullptr)
    return;

  if (!pDesc->m_sTitle.IsEmpty())
  {
    ezStringBuilder temp;

    ezHybridArray<const ezAbstractProperty*, 32> properties;
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

  m_pTitleLabel->setPlainText(sTitle.GetData());
}
