#pragma once

#include <GuiFoundation/VisualGraph/Connection.h>
#include <GuiFoundation/VisualGraph/Node.h>
#include <GuiFoundation/VisualGraph/Pin.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>

/// Qt graphics item for state machine pins.
///
/// Displays connection points for state transitions with custom rectangular pin geometry.
class ezQtStateMachinePin : public ezQtVisualGraphPin
{
public:
  ezQtStateMachinePin();

  virtual void SetPin(const ezVisualGraphPin& pin) override;
  virtual QRectF GetPinRect() const override;
};

/// Qt graphics item for state machine transitions.
///
/// Renders the visual connections between states, representing the possible transitions.
class ezQtStateMachineConnection : public ezQtVisualGraphConnection
{
public:
  ezQtStateMachineConnection();
};

/// Qt graphics item for state machine nodes.
///
/// Represents individual states in a state machine. The initial state and "Any State" are displayed
/// with distinct header colors for easy identification.
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

/// Qt scene for state machine graphs.
///
/// Manages the visual scene for state machine editing, including handling the designation of the initial state.
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
