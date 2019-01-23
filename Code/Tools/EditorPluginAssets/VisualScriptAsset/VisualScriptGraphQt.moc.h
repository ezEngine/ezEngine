#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Connection.h>

class ezQtNodeView;
struct ezVisualScriptActivityEvent;
struct ezVisualScriptInstanceActivity;

class ezQtVisualScriptAssetScene : public ezQtNodeScene
{
  Q_OBJECT

public:
  ezQtVisualScriptAssetScene(QObject* parent = nullptr);
  ~ezQtVisualScriptAssetScene();

  void VisualScriptActivityEventHandler(const ezVisualScriptActivityEvent& ae);
  void VisualScriptInterDocumentMessageHandler(ezReflectedClass* pMsg);
  void SetDebugObject(const ezUuid& objectGuid);
  ezUuid GetDebugObject() const { return m_DebugObject; }

private Q_SLOTS:
  void OnUpdateDisplay();

private:
  void GetAllVsNodes(ezDynamicArray<const ezDocumentObject *>& allNodes) const;
  void ResetActiveConnections(ezDynamicArray<const ezDocumentObject *> &allNodes);

  ezUuid m_DebugObject;
};

class ezQtVisualScriptPin : public ezQtPin
{
public:
  ezQtVisualScriptPin();

  virtual void SetPin(const ezPin* pPin) override;
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

class ezQtVisualScriptConnection : public ezQtConnection
{
public:
  ezQtVisualScriptConnection(QGraphicsItem* parent = 0);

  virtual QPen DeterminePen() const override;

  bool m_bExecutionHighlight = false;
  ezTime m_HighlightUntil;
};

class ezQtVisualScriptNode : public ezQtNode
{
public:
  ezQtVisualScriptNode();

  virtual void InitNode(const ezDocumentNodeManager* pManager, const ezDocumentObject* pObject) override;

  virtual void UpdateTitle() override;
};

