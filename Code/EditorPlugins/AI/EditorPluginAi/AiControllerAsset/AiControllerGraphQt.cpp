#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorPluginAi/AiControllerAsset/AiControllerAsset.h>
#include <EditorPluginAi/AiControllerAsset/AiControllerGraphQt.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/Math/ColorScheme.h>
#include <GuiFoundation/GuiFoundationDLL.h>

ezQtAiControllerNode::ezQtAiControllerNode() = default;

void ezQtAiControllerNode::InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject)
{
  ezQtVisualGraphNode::InitNode(pManager, pObject);

  ezColorScheme::Enum color = ezColorScheme::Gray;
  switch (ezAiControllerNodeManager::GetNodeKind(pObject->GetType()))
  {
    case ezAiControllerNodeManager::NodeKind::Sensor:
      color = ezColorScheme::Cyan;
      break;
    case ezAiControllerNodeManager::NodeKind::PerceptionGenerator:
      color = ezColorScheme::Violet;
      break;
    case ezAiControllerNodeManager::NodeKind::Behavior:
      color = ezColorScheme::Orange;
      break;
    case ezAiControllerNodeManager::NodeKind::Consideration:
      color = ezColorScheme::Lime;
      break;
    default:
      break;
  }

  m_HeaderColor = ezToQtColor(ezColorScheme::DarkUI(color));
}

void ezQtAiControllerNode::UpdateState()
{
  TitleFormat format;
  format.m_bQuoteStrings = false;

  // the part before '::' becomes the subtitle
  ezStringBuilder sTemplate;
  switch (ezAiControllerNodeManager::GetNodeKind(GetObject()->GetType()))
  {
    case ezAiControllerNodeManager::NodeKind::Sensor:
    case ezAiControllerNodeManager::NodeKind::Consideration:
      sTemplate = "{Name}::";
      break;
    case ezAiControllerNodeManager::NodeKind::Behavior:
      sTemplate = "{Category} {Score}::";
      break;
    default:
      break;
  }

  sTemplate.Append(ezAiControllerNodeManager::GetShortTypeName(GetObject()->GetType()));

  ezStringBuilder sTitle;
  ezTokenParseUtils::RenderTemplate(sTemplate, [&](ezStringView sPlaceholder, ezVariant index, bool bOptional, ezStringBuilder& ref_sOutput)
    { ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); }, sTitle);

  SetTitleAndSubtitle(sTitle, format);
}
