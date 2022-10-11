#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>


ezQtVisualShaderScene::ezQtVisualShaderScene(QObject* parent)
  : ezQtNodeScene(parent)
{
}

ezQtVisualShaderScene::~ezQtVisualShaderScene() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezQtVisualShaderPin::ezQtVisualShaderPin() = default;

void ezQtVisualShaderPin::SetPin(const ezPin& pin)
{
  ezQtPin::SetPin(pin);

  const ezVisualShaderPin& shaderPin = ezStaticCast<const ezVisualShaderPin&>(pin);

  ezStringBuilder sTooltip;
  if (!shaderPin.GetTooltip().IsEmpty())
  {
    sTooltip = shaderPin.GetTooltip();
  }
  else
  {
    sTooltip = shaderPin.GetName();
  }

  if (!shaderPin.GetDescriptor()->m_sDefaultValue.IsEmpty())
  {
    if (!sTooltip.IsEmpty())
      sTooltip.Append("\n");

    sTooltip.Append("Default is ", shaderPin.GetDescriptor()->m_sDefaultValue);
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

      QPen pen;
      pen.setColor(qRgb(255, 0, 0));
      pen.setWidth(3);
      pen.setCosmetic(true);
      pen.setStyle(Qt::PenStyle::SolidLine);
      pen.setCapStyle(Qt::PenCapStyle::SquareCap);

      painter->setPen(pen);

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

ezQtVisualShaderNode::ezQtVisualShaderNode() = default;

void ezQtVisualShaderNode::InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject)
{
  ezQtNode::InitNode(pManager, pObject);

  if (auto pDesc = ezVisualShaderTypeRegistry::GetSingleton()->GetDescriptorForType(pObject->GetType()))
  {
    m_HeaderColor = qRgb(pDesc->m_Color.r, pDesc->m_Color.g, pDesc->m_Color.b);
  }
  else
  {
    m_HeaderColor = qRgb(255, 0, 0);
    ezLog::Error("Could not initialize node type, node descriptor is invalid");
  }
}

void ezQtVisualShaderNode::UpdateState()
{
  ezStringBuilder temp = GetObject()->GetTypeAccessor().GetType()->GetTypeName();
  if (temp.StartsWith_NoCase("ShaderNode::"))
    temp.Shrink(12, 0);

  m_pLabel->setPlainText(temp.GetData());
}
