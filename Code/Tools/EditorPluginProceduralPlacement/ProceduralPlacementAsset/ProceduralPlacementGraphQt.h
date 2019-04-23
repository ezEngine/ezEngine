#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Node.h>

class ezQtProceduralPlacementNode : public ezQtNode
{
public:
  ezQtProceduralPlacementNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

class ezQtProceduralPlacementPin : public ezQtPin
{
public:
  ezQtProceduralPlacementPin();
  ~ezQtProceduralPlacementPin();

  virtual void ExtendContextMenu(QMenu& menu) override;

  virtual void keyPressEvent(QKeyEvent* event) override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QRectF boundingRect() const override;

  void SetDebug(bool bDebug);

private:
  bool m_bDebug = false;
};

class ezQtProceduralPlacementScene : public ezQtNodeScene
{
public:
  ezQtProceduralPlacementScene(QObject* parent = nullptr);
  ~ezQtProceduralPlacementScene();

  void SetDebugPin(ezQtProceduralPlacementPin* pDebugPin);

private:
  virtual ezStatus RemoveNode(ezQtNode* pNode) override;

  ezQtProceduralPlacementPin* m_pDebugPin = nullptr;
};
