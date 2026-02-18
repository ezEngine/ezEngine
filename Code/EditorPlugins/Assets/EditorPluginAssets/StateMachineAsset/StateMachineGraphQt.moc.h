#pragma once

#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class ezQtStateMachinePin : public ezQtVisualGraphPin
{
public:
  ezQtStateMachinePin();

  virtual void SetPin(const ezVisualGraphPin& pin) override;
  virtual QRectF GetPinRect() const override;
};

class ezQtStateMachineConnection : public ezQtVisualGraphConnection
{
public:
  ezQtStateMachineConnection();
};

class ezQtStateMachineNode : public ezQtVisualGraphNode
{
public:
  ezQtStateMachineNode();

  virtual void InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject) override;
  virtual void UpdateGeometry() override;
  virtual void UpdateState() override;
  virtual void ExtendContextMenu(QMenu& ref_menu) override;

  bool IsInitialState() const;
  bool IsAnyState() const;

private:
  void UpdateHeaderColor();
};

class ezQtStateMachineAssetScene : public ezQtVisualGraphScene
{
  Q_OBJECT

public:
  ezQtStateMachineAssetScene(QObject* pParent = nullptr);
  ~ezQtStateMachineAssetScene();

  void SetInitialState(ezQtStateMachineNode* pNode);

private:
  virtual ezStatus RemoveNode(ezQtVisualGraphNode* pNode) override;
};
