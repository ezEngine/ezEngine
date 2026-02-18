#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>

/// Qt graphics item for procedural generation nodes.
///
/// Visual representation of procedural generation nodes, such as noise generators, modifiers, or output nodes.
class ezQtProcGenNode : public ezQtVisualGraphNode
{
public:
  ezQtProcGenNode();

  virtual void InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

/// Qt graphics item for procedural generation pins.
///
/// Extends the base pin with debugging support. Pins can be marked for debug visualization,
/// allowing users to inspect intermediate results in the procedural generation pipeline.
class ezQtProcGenPin : public ezQtVisualGraphPin
{
public:
  ezQtProcGenPin();
  ~ezQtProcGenPin();

  virtual void ExtendContextMenu(QMenu& ref_menu) override;

  virtual void keyPressEvent(QKeyEvent* pEvent) override;
  virtual void paint(QPainter* pPainter, const QStyleOptionGraphicsItem* pOption, QWidget* pWidget) override;
  virtual QRectF boundingRect() const override;

  void SetDebug(bool bDebug);

private:
  bool m_bDebug = false;
};

/// Qt scene for procedural generation graphs.
///
/// Manages the visual scene for procedural generation graph editing, including debug pin tracking
/// for visualizing intermediate generation results.
class ezQtProcGenScene : public ezQtVisualGraphScene
{
public:
  ezQtProcGenScene(QObject* pParent = nullptr);
  ~ezQtProcGenScene();

  void SetDebugPin(ezQtProcGenPin* pDebugPin);

private:
  virtual ezStatus RemoveNode(ezQtVisualGraphNode* pNode) override;

  bool m_bUpdatingDebugPin = false;
  ezQtProcGenPin* m_pDebugPin = nullptr;
};
