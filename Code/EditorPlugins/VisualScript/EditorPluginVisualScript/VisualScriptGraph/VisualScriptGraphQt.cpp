#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraphQt.moc.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, Factories)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezVisualScriptNodeRegistry);
    const ezRTTI* pBaseType = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    ezQtVisualGraphScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptPin>(), [](const ezRTTI* pRtti)->ezQtVisualGraphPin* { return new ezQtVisualScriptPin(); });
    /*ezQtVisualGraphScene::GetConnectionFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptConnection>(), [](const ezRTTI* pRtti)->ezQtVisualGraphConnection* { return new ezQtVisualScriptConnection(); });    */
    ezQtVisualGraphScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtVisualGraphNode* { return new ezQtVisualScriptNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const ezRTTI* pBaseType = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    ezQtVisualGraphScene::GetPinFactory().UnregisterCreator(ezGetStaticRTTI<ezVisualScriptPin>());
    //ezQtVisualGraphScene::GetConnectionFactory().UnregisterCreator(ezGetStaticRTTI<ezVisualScriptConnection>());
    ezQtVisualGraphScene::GetNodeFactory().UnregisterCreator(pBaseType);

    ezVisualScriptNodeRegistry* pDummy = ezVisualScriptNodeRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptPin::ezQtVisualScriptPin() = default;

void ezQtVisualScriptPin::SetPin(const ezVisualGraphPin& pin)
{
  m_bTranslatePinName = false;

  ezQtVisualGraphPin::SetPin(pin);

  UpdateTooltip();
}

bool ezQtVisualScriptPin::UpdatePinColors(const ezColorGammaUB* pOverwriteColor)
{
  ezColorGammaUB overwriteColor;
  const ezVisualScriptPin& vsPin = ezStaticCast<const ezVisualScriptPin&>(*GetPin());

  ezVisualScriptDataType::Enum type = vsPin.GetResolvedScriptDataType();
  if (vsPin.NeedsTypeDeduction())
  {
    overwriteColor = ezVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(type);
    pOverwriteColor = &overwriteColor;
  }

  bool res = ezQtVisualGraphPin::UpdatePinColors(pOverwriteColor);

  if (vsPin.IsRequired() && type != ezVisualScriptDataType::GameObject && HasAnyConnections() == false)
  {
    QColor requiredColor = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Red));

    QPen p = pen();
    p.setColor(requiredColor);
    setPen(p);

    m_pLabel->setDefaultTextColor(requiredColor);

    return true;
  }

  UpdateTooltip();

  return res;
}

void ezQtVisualScriptPin::UpdateTooltip()
{
  const ezVisualScriptPin& vsPin = ezStaticCast<const ezVisualScriptPin&>(*GetPin());

  ezStringBuilder sTooltip;
  sTooltip = vsPin.GetName();

  if (vsPin.IsDataPin())
  {
    sTooltip.Append(": ", vsPin.GetDataTypeName());

    if (vsPin.IsRequired())
    {
      sTooltip.Append(" (Required)");
    }
  }

  setToolTip(sTooltip.GetData());
}

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptConnection::ezQtVisualScriptConnection() = default;

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptNode::ezQtVisualScriptNode() = default;

void ezQtVisualScriptNode::UpdateState()
{
  const TitleFormat format;

  ezStringBuilder sTemplate;
  if (!TryGetTitleTemplateFromAttribute(sTemplate))
  {
    sTemplate = ezVisualScriptNodeManager::GetNiceTypeName(GetObject());
  }

  ezStringBuilder sTitle;
  ezTokenParseUtils::RenderTemplate(sTemplate, [&](ezStringView sPlaceholder, ezVariant index, bool bOptional, ezStringBuilder& ref_sOutput)
    { ResolvePlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); },
    sTitle);

  SetTitleAndSubtitle(sTitle, format);

  auto pManager = static_cast<const ezVisualScriptNodeManager*>(GetObject()->GetDocumentObjectManager());

  if (m_pSubtitleLabel->toPlainText().isEmpty())
  {
    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(GetObject()->GetType());
    if (pNodeDesc != nullptr && pNodeDesc->NeedsTypeDeduction())
    {
      ezVisualScriptDataType::Enum deductedType = pManager->GetDeductedType(GetObject());
      m_pSubtitleLabel->setPlainText(deductedType != ezVisualScriptDataType::Invalid ? ezVisualScriptDataType::GetName(deductedType) : "Unknown");
    }
  }

  auto pScene = static_cast<ezQtVisualScriptNodeScene*>(scene());

  if (pManager->IsCoroutine(GetObject()))
  {
    m_pIcon->setPixmap(pScene->GetCoroutineIcon());
    m_pIcon->setScale(0.5);
  }
  else if (pManager->IsLoop(GetObject()))
  {
    m_pIcon->setPixmap(pScene->GetLoopIcon());
    m_pIcon->setScale(0.5);
  }
  else
  {
    m_pIcon->setPixmap(QPixmap());
  }
}

void ezQtVisualScriptNode::ResolvePlaceholder(ezStringView sPlaceholder, const ezVariant& index, bool bOptional, const TitleFormat& format, ezStringBuilder& ref_sOutput)
{
  for (const auto& pin : GetInputPins())
  {
    if (pin->GetPin()->GetName() != sPlaceholder || !pin->HasAnyConnections())
      continue;

    // the value of a connected string pin is unknown here, so nothing is shown for it
    if (!bOptional && static_cast<const ezVisualScriptPin*>(pin->GetPin())->GetScriptDataType() != ezVisualScriptDataType::String)
    {
      ref_sOutput.Append(sPlaceholder);
    }

    return;
  }

  ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput);
}

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptNodeScene::ezQtVisualScriptNodeScene(QObject* pParent /*= nullptr*/)
  : ezQtVisualGraphScene(pParent)
{
  constexpr int iconSize = 32;
  m_CoroutineIcon = QIcon(":/EditorPluginVisualScript/Icons/Coroutine.svg").pixmap(QSize(iconSize, iconSize));
  m_LoopIcon = QIcon(":/EditorPluginVisualScript/Icons/Loop.svg").pixmap(QSize(iconSize, iconSize));
}

ezQtVisualScriptNodeScene::~ezQtVisualScriptNodeScene()
{
  if (m_pManager != nullptr)
  {
    static_cast<const ezVisualScriptNodeManager*>(m_pManager)->m_NodeChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtVisualScriptNodeScene::NodeChangedHandler, this));
  }
}

void ezQtVisualScriptNodeScene::InitScene(const ezVisualGraphObjectManager* pManager)
{
  ezQtVisualGraphScene::InitScene(pManager);

  static_cast<const ezVisualScriptNodeManager*>(pManager)->m_NodeChangedEvent.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptNodeScene::NodeChangedHandler, this));
}

void ezQtVisualScriptNodeScene::NodeChangedHandler(const ezDocumentObject* pObject)
{
  auto it = m_Nodes.Find(pObject);
  if (it.IsValid() == false)
    return;

  ezQtVisualGraphNode* pNode = it.Value();

  pNode->ResetFlags();
  pNode->update();

  auto& inputPins = pNode->GetInputPins();
  for (ezQtVisualGraphPin* pPin : inputPins)
  {
    if (static_cast<ezQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }

  auto& outputPins = pNode->GetOutputPins();
  for (ezQtVisualGraphPin* pPin : outputPins)
  {
    if (static_cast<ezQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }
}
