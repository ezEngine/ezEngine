#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

class ezQtVisualGraphView;

/// Qt scene for visual shader asset editing.
///
/// Manages the visual scene for editing visual shader assets in the editor.
class ezQtVisualShaderScene : public ezQtVisualGraphScene
{
  Q_OBJECT

public:
  ezQtVisualShaderScene(QObject* pParent = nullptr);
  ~ezQtVisualShaderScene();
};

/// Qt graphics item for visual shader pins.
///
/// Displays shader node pins with custom rendering for shader-specific visual feedback.
class ezQtVisualShaderPin : public ezQtVisualGraphPin
{
public:
  ezQtVisualShaderPin();

  virtual void SetPin(const ezVisualGraphPin& pin) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
};

/// Qt graphics item for visual shader nodes.
///
/// Visual representation of shader nodes such as texture samplers, math operations, or output nodes.
class ezQtVisualShaderNode : public ezQtVisualGraphNode
{
public:
  ezQtVisualShaderNode();

  virtual void InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateState() override;
};
