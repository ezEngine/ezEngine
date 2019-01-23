#include <PCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>
#include <EditorPluginAssets/VisualShader/VisualShaderTypeRegistry.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QPainter>
#include <ToolsFoundation/Command/NodeCommands.h>


ezQtVisualShaderScene::ezQtVisualShaderScene(QObject* parent)
    : ezQtNodeScene(parent)
{
}

ezQtVisualShaderScene::~ezQtVisualShaderScene() {}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


ezQtVisualShaderPin::ezQtVisualShaderPin() {}


void ezQtVisualShaderPin::SetPin(const ezPin* pPin)
{
  ezQtPin::SetPin(pPin);

  const ezVisualShaderPin* pShaderPin = ezDynamicCast<const ezVisualShaderPin*>(pPin);
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

  if (!pShaderPin->GetDescriptor()->m_sDefaultValue.IsEmpty())
  {
    if (!sTooltip.IsEmpty())
      sTooltip.Append("\n");

    sTooltip.Append("Default is ", pShaderPin->GetDescriptor()->m_sDefaultValue);
  }

  setToolTip(sTooltip.GetData());
}

void ezQtVisualShaderPin::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  QPainterPath p = path();

  const ezVisualShaderPin* pVsPin = static_cast<const ezVisualShaderPin*>(GetPin());

  painter->save();
  painter->setBrush(brush());
  painter->setPen(pen());

  if (pVsPin->GetType() == ezPin::Type::Input && GetConnections().IsEmpty())
  {
    if (pVsPin->GetDescriptor()->m_sDefaultValue.IsEmpty())
    {
      // this pin MUST be connected

      QPen p;
      p.setColor(qRgb(255, 0, 0));
      p.setWidth(3);
      p.setCosmetic(true);
      p.setStyle(Qt::PenStyle::SolidLine);
      p.setCapStyle(Qt::PenCapStyle::SquareCap);

      painter->setPen(p);

      painter->drawRect(this->path().boundingRect());
      painter->restore();
      return;
    }
  }

  painter->drawPath(p);
  painter->restore();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualShaderNode::ezQtVisualShaderNode()
{
  // this costs too much performance :-(
  EnableDropShadow(false);
}

void ezQtVisualShaderNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

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

void ezQtVisualShaderNode::UpdateTitle()
{
  ezStringBuilder temp = GetObject()->GetTypeAccessor().GetType()->GetTypeName();
  if (temp.StartsWith_NoCase("ShaderNode::"))
    temp.Shrink(12, 0);

  m_pLabel->setPlainText(temp.GetData());
}
