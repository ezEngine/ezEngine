#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraphQt.moc.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptNodeRegistry.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginVisualScript, Factories)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezVisualScriptNodeRegistry);
    const ezRTTI* pBaseType = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    ezQtNodeScene::GetPinFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptPin>(), [](const ezRTTI* pRtti)->ezQtPin* { return new ezQtVisualScriptPin(); });
    /*ezQtNodeScene::GetConnectionFactory().RegisterCreator(ezGetStaticRTTI<ezVisualScriptConnection>(), [](const ezRTTI* pRtti)->ezQtConnection* { return new ezQtVisualScriptConnection(); });    */
    ezQtNodeScene::GetNodeFactory().RegisterCreator(pBaseType, [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtVisualScriptNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    const ezRTTI* pBaseType = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType();

    ezQtNodeScene::GetPinFactory().UnregisterCreator(ezGetStaticRTTI<ezVisualScriptPin>());
    //ezQtNodeScene::GetConnectionFactory().UnregisterCreator(ezGetStaticRTTI<ezVisualScriptConnection>());
    ezQtNodeScene::GetNodeFactory().UnregisterCreator(pBaseType);

    ezVisualScriptNodeRegistry* pDummy = ezVisualScriptNodeRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptPin::ezQtVisualScriptPin() = default;

void ezQtVisualScriptPin::SetPin(const ezPin& pin)
{
  m_bTranslatePinName = false;

  ezQtPin::SetPin(pin);

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

  bool res = ezQtPin::UpdatePinColors(pOverwriteColor);

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
  ezStringBuilder sTitle;

  auto pManager = static_cast<const ezVisualScriptNodeManager*>(GetObject()->GetDocumentObjectManager());
  auto pType = GetObject()->GetType();

  if (auto pTitleAttribute = pType->GetAttributeByType<ezTitleAttribute>())
  {
    sTitle = pTitleAttribute->GetTitle();

    ezHybridArray<const ezAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

    ezStringBuilder temp;
    for (const auto& pin : GetInputPins())
    {
      if (pin->HasAnyConnections())
      {
        temp.Set("{", pin->GetPin()->GetName(), "}");
        if (static_cast<const ezVisualScriptPin*>(pin->GetPin())->GetScriptDataType() == ezVisualScriptDataType::String)
        {
          sTitle.ReplaceAll(temp, "");
        }
        else
        {
          sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
        }

        temp.Set("{?", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, "");
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
      else if (val.IsA<ezString>() || val.IsA<ezHashedString>())
      {
        sVal = val.ConvertTo<ezString>();

        if (prop->GetAttributeByType<ezAssetBrowserAttribute>())
        {
          if (ezConversionUtils::IsStringUuid(sVal))
          {
            const ezUuid AssetGuid = ezConversionUtils::ConvertStringToUuid(sVal);

            auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

            if (pAsset)
              sVal = pAsset->m_pAssetInfo->m_Path.GetDataDirRelativePath().GetFileName();
            else
              sVal = "<unknown>";
          }
        }

        sVal.ReplaceAll("\n", " ");
        sVal.ReplaceAll("\t", " ");

        if (sVal.GetCharacterCount() > 23)
        {
          sVal.Shrink(0, sVal.GetCharacterCount() - 21);
          sVal.Append("...");
        }
        sVal.Prepend("\"");
        sVal.Append("\"");
      }
      else if (val.CanConvertTo<ezString>())
      {
        sVal = val.ConvertTo<ezString>();
      }
      else
      {
        sVal = "<Invalid>";
      }

      temp.Set("{", prop->GetPropertyName(), "}");
      sTitle.ReplaceAll(temp, sVal);

      temp.Set("{?", prop->GetPropertyName(), "}");
      if (val == ezVariant(0))
      {
        sTitle.ReplaceAll(temp, "");
      }
      else
      {
        sTitle.ReplaceAll(temp, sVal);
      }
    }
  }
  else
  {
    sTitle = ezVisualScriptNodeManager::GetNiceTypeName(GetObject());
  }

  if (const char* szSeparator = sTitle.FindSubString("::"))
  {
    m_pTitleLabel->setPlainText(szSeparator + 2);

    ezStringBuilder sSubTitle = ezStringView(sTitle.GetData(), szSeparator);
    sSubTitle.Trim("\"");
    m_pSubtitleLabel->setPlainText(sSubTitle.GetData());
  }
  else
  {
    m_pTitleLabel->setPlainText(sTitle.GetData());

    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pType);
    if (pNodeDesc != nullptr && pNodeDesc->NeedsTypeDeduction())
    {
      ezVisualScriptDataType::Enum deductedType = pManager->GetDeductedType(GetObject());
      const char* sSubTitle = deductedType != ezVisualScriptDataType::Invalid ? ezVisualScriptDataType::GetName(deductedType) : "Unknown";
      m_pSubtitleLabel->setPlainText(sSubTitle);
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

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptNodeScene::ezQtVisualScriptNodeScene(QObject* pParent /*= nullptr*/)
  : ezQtNodeScene(pParent)
{
  constexpr int iconSize = 32;
  m_CoroutineIcon = QIcon(":/EditorPluginVisualScript/Coroutine.svg").pixmap(QSize(iconSize, iconSize));
  m_LoopIcon = QIcon(":/EditorPluginVisualScript/Loop.svg").pixmap(QSize(iconSize, iconSize));
}

ezQtVisualScriptNodeScene::~ezQtVisualScriptNodeScene()
{
  if (m_pManager != nullptr)
  {
    static_cast<const ezVisualScriptNodeManager*>(m_pManager)->m_NodeChangedEvent.RemoveEventHandler(ezMakeDelegate(&ezQtVisualScriptNodeScene::NodeChangedHandler, this));
  }
}

void ezQtVisualScriptNodeScene::InitScene(const ezDocumentNodeManager* pManager)
{
  ezQtNodeScene::InitScene(pManager);

  static_cast<const ezVisualScriptNodeManager*>(pManager)->m_NodeChangedEvent.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptNodeScene::NodeChangedHandler, this));
}

void ezQtVisualScriptNodeScene::NodeChangedHandler(const ezDocumentObject* pObject)
{
  auto it = m_Nodes.Find(pObject);
  if (it.IsValid() == false)
    return;

  ezQtNode* pNode = it.Value();

  pNode->ResetFlags();
  pNode->update();

  auto& inputPins = pNode->GetInputPins();
  for (ezQtPin* pPin : inputPins)
  {
    if (static_cast<ezQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }

  auto& outputPins = pNode->GetOutputPins();
  for (ezQtPin* pPin : outputPins)
  {
    if (static_cast<ezQtVisualScriptPin*>(pPin)->UpdatePinColors())
    {
      pPin->update();
    }
  }
}
