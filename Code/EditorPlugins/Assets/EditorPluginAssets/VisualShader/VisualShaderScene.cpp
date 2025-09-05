#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>
#include <EditorPluginAssets/VisualShader/VisualShaderScene.moc.h>


ezQtVisualShaderScene::ezQtVisualShaderScene(QObject* pParent)
  : ezQtNodeScene(pParent)
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

void ezQtVisualShaderPin::paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget)
{
  QPainterPath p = path();

  const ezVisualShaderPin* pVsPin = static_cast<const ezVisualShaderPin*>(GetPin());

  pPainter->save();
  pPainter->setBrush(brush());
  pPainter->setPen(pen());

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

      pPainter->setPen(pen);

      pPainter->drawRect(this->path().boundingRect());
      pPainter->restore();
      return;
    }
  }

  pPainter->drawPath(p);
  pPainter->restore();
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
    m_HeaderColor = ezToQtColor(pDesc->m_Color);
    m_pTitleLabel->setToolTip(ezMakeQString(pDesc->m_sDocs));
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

  m_pTitleLabel->setPlainText(temp.GetData());
}
