#pragma once

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class ezQtStateMachinePin : public ezQtPin
{
public:
  ezQtStateMachinePin();

  virtual void SetPin(const ezPin& pin) override;
  virtual QRectF GetPinRect() const override;
};

class ezQtStateMachineConnection : public ezQtConnection
{
public:
  ezQtStateMachineConnection(QGraphicsItem* parent = nullptr);
};

class ezQtStateMachineNode : public ezQtNode
{
public:
  ezQtStateMachineNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject) override;
  virtual void UpdateGeometry() override;
  virtual void UpdateState() override;
  virtual void ExtendContextMenu(QMenu& menu) override;

  bool IsInitialState() const;
  bool IsAnyState() const;

private:
  void UpdateHeaderColor();
};

class ezQtStateMachineAssetScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtStateMachineAssetScene(QObject* parent = nullptr);
  ~ezQtStateMachineAssetScene();

  void SetInitialState(ezQtStateMachineNode* pNode);

private:
  virtual ezStatus RemoveNode(ezQtNode* pNode) override;
};
