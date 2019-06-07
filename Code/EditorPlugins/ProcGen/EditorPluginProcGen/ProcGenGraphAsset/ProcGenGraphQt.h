#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class ezQtProcGenNode : public ezQtNode
{
public:
  ezQtProcGenNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

class ezQtProcGenPin : public ezQtPin
{
public:
  ezQtProcGenPin();
  ~ezQtProcGenPin();

  virtual void ExtendContextMenu(QMenu& menu) override;

  virtual void keyPressEvent(QKeyEvent* event) override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QRectF boundingRect() const override;

  void SetDebug(bool bDebug);

private:
  bool m_bDebug = false;
};

class ezQtProcGenScene : public ezQtNodeScene
{
public:
  ezQtProcGenScene(QObject* parent = nullptr);
  ~ezQtProcGenScene();

  void SetDebugPin(ezQtProcGenPin* pDebugPin);

private:
  virtual ezStatus RemoveNode(ezQtNode* pNode) override;

  ezQtProcGenPin* m_pDebugPin = nullptr;
};
