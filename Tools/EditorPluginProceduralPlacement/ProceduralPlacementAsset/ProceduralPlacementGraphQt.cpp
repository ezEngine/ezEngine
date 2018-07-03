#include <PCH.h>
#include <EditorPluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementGraphQt.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <QPainter>
#include <QTimer>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/NodeEditor/Pin.h>

namespace
{
  static ezColorGammaUB CategoryColor(const char* szCategory)
  {
    if (ezStringUtils::IsEqual(szCategory, "Input"))
      return ezColorGammaUB(38, 105, 0);
    else if (ezStringUtils::IsEqual(szCategory, "Math"))
      return ezColorGammaUB(0, 53, 91);

    return ezColor::Pink;
  }
}


ezQtProceduralPlacementNode::ezQtProceduralPlacementNode()
{
  // this costs too much performance :-(
  EnableDropShadow(false);
}

void ezQtProceduralPlacementNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

  const ezRTTI* pRtti = pObject->GetType();

  if (const ezCategoryAttribute* pAttr = pRtti->GetAttributeByType<ezCategoryAttribute>())
  {
    ezColorGammaUB color = CategoryColor(pAttr->GetCategory());
    m_HeaderColor = qRgb(color.r, color.g, color.b);
  }
}

void ezQtProceduralPlacementNode::UpdateTitle()
{
  ezStringBuilder sTitle;

  const ezRTTI* pRtti = GetObject()->GetType();

  if (const ezTitleAttribute* pAttr = pRtti->GetAttributeByType<ezTitleAttribute>())
  {
    ezStringBuilder temp;
    ezStringBuilder temp2;

    ezHybridArray<ezAbstractProperty*, 32> properties;
    GetObject()->GetType()->GetAllProperties(properties);

    sTitle = pAttr->GetTitle();

    for (const auto& pin : GetInputPins())
    {
      temp.Set("{", pin->GetPin()->GetName(), "}");

      if (pin->HasAnyConnections())
      {
        sTitle.ReplaceAll(temp, pin->GetPin()->GetName());
      }
      else
      {
        temp2.Set("{Input", pin->GetPin()->GetName(), "}");
        sTitle.ReplaceAll(temp, temp2);
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
    if (sTitle.StartsWith_NoCase("ezProceduralPlacement"))
    {
      sTitle.Shrink(21, 0);
    }
  }

  m_pLabel->setPlainText(sTitle.GetData());
}

