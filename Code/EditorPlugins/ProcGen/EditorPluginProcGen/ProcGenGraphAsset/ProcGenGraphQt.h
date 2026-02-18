#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class ezQtProcGenNode : public ezQtVisualGraphNode
{
public:
  ezQtProcGenNode();

  virtual void InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

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
