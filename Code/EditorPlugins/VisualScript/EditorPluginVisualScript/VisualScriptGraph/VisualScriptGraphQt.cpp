#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

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
  ezQtPin::SetPin(pin);

  const ezVisualScriptPin& vsPin = ezStaticCast<const ezVisualScriptPin&>(pin);

  ezStringBuilder sTooltip;
  sTooltip = vsPin.GetName();

  if (vsPin.IsDataPin())
  {
    auto scriptDataType = vsPin.GetScriptDataType();
    if (scriptDataType != ezVisualScriptDataType::TypedPointer)
    {
      sTooltip.Append(": ", ezVisualScriptDataType::GetName(scriptDataType));
    }
    else
    {
      sTooltip.Append(": ", vsPin.GetDataType()->GetTypeName());
    }

    if (vsPin.IsRequired())
    {
      sTooltip.Append(" (Required)");
    }
  }

  setToolTip(sTooltip.GetData());
}

bool ezQtVisualScriptPin::UpdatePinColors(const ezColorGammaUB* pOverwriteColor)
{
  ezColorGammaUB overwriteColor;
  const ezVisualScriptPin& vsPin = ezStaticCast<const ezVisualScriptPin&>(*GetPin());
  if (vsPin.GetScriptDataType() == ezVisualScriptDataType::Any)
  {
    auto pManager = static_cast<const ezVisualScriptNodeManager*>(vsPin.GetParent()->GetDocumentObjectManager());
    auto deductedType = pManager->GetDeductedType(vsPin.GetParent());
    overwriteColor = ezVisualScriptNodeRegistry::PinDesc::GetColorForScriptDataType(deductedType);
    pOverwriteColor = &overwriteColor;
  }

  bool res = ezQtPin::UpdatePinColors(pOverwriteColor);

  if (vsPin.IsRequired() && HasAnyConnections() == false)
  {
    QColor requiredColor = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Red));

    QPen p = pen();
    p.setColor(requiredColor);
    setPen(p);

    m_pLabel->setDefaultTextColor(requiredColor);

    return true;
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptConnection::ezQtVisualScriptConnection() = default;

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptNode::ezQtVisualScriptNode() = default;

void ezQtVisualScriptNode::UpdateState()
{
  ezStringBuilder sTitle;

  auto pType = GetObject()->GetType();
  if (auto pTitleAttribute = pType->GetAttributeByType<ezTitleAttribute>())
  {
    sTitle = pTitleAttribute->GetTitle();

    ezHybridArray<ezAbstractProperty*, 32> properties;
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
      else if (val.IsA<ezString>())
      {
        sVal = val.Get<ezString>();
        if (sVal.GetCharacterCount() > 16)
        {
          sVal.Shrink(0, sVal.GetCharacterCount() - 13);
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
    m_pSubtitleLabel->setPlainText(sSubTitle.GetData());
  }
  else
  {
    m_pTitleLabel->setPlainText(sTitle.GetData());

    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pType);
    if (pNodeDesc->m_bNeedsDataTypeDeduction)
    {
      auto pManager = static_cast<const ezVisualScriptNodeManager*>(GetObject()->GetDocumentObjectManager());
      ezVisualScriptDataType::Enum deductedType = pManager->GetDeductedType(GetObject());
      const char* sSubTitle = deductedType != ezVisualScriptDataType::Invalid ? ezVisualScriptDataType::GetName(deductedType) : "Unknown";
      m_pSubtitleLabel->setPlainText(sSubTitle);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

ezQtVisualScriptNodeScene::ezQtVisualScriptNodeScene(QObject* parent /*= nullptr*/)
  : ezQtNodeScene(parent)
{
}

ezQtVisualScriptNodeScene::~ezQtVisualScriptNodeScene() = default;

void ezQtVisualScriptNodeScene::SetDocumentNodeManager(const ezDocumentNodeManager* pManager)
{
  ezQtNodeScene::SetDocumentNodeManager(pManager);

  static_cast<const ezVisualScriptNodeManager*>(pManager)->m_DeductedTypeChangedEvent.AddEventHandler(ezMakeDelegate(&ezQtVisualScriptNodeScene::DeductedTypeChangedHandler, this));
}

void ezQtVisualScriptNodeScene::DeductedTypeChangedHandler(const ezDocumentObject* pObject)
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
