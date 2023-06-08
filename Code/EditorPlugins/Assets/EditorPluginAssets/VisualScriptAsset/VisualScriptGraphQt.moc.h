#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>

class ezQtNodeView;
struct ezVisualScriptActivityEvent;
struct ezVisualScriptInstanceActivity;

class ezQtVisualScriptAssetScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtVisualScriptAssetScene(QObject* pParent = nullptr);
  ~ezQtVisualScriptAssetScene();

  void VisualScriptActivityEventHandler(const ezVisualScriptActivityEvent& ae);
  void VisualScriptInterDocumentMessageHandler(ezReflectedClass* pMsg);
  void SetDebugObject(const ezUuid& objectGuid);
  ezUuid GetDebugObject() const { return m_DebugObject; }

private Q_SLOTS:
  void OnUpdateDisplay();

private:
  void GetAllVsNodes(ezDynamicArray<const ezDocumentObject*>& allNodes) const;
  void ResetActiveConnections(ezDynamicArray<const ezDocumentObject*>& allNodes);

  ezUuid m_DebugObject;
};

class ezQtVisualScriptPin_Legacy : public ezQtPin
{
public:
  ezQtVisualScriptPin_Legacy();

  virtual void SetPin(const ezPin& pin) override;
};

class ezQtVisualScriptConnection_Legacy : public ezQtConnection
{
public:
  ezQtVisualScriptConnection_Legacy(QGraphicsItem* pParent = 0);

  virtual QPen DeterminePen() const override;

  bool m_bExecutionHighlight = false;
  ezTime m_HighlightUntil;
};

class ezQtVisualScriptNode_Legacy : public ezQtNode
{
public:
  ezQtVisualScriptNode_Legacy();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateState() override;
};

