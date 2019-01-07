#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <QGraphicsScene>
#include <QGraphicsItem>

class ezQtNode;
class ezQtPin;
class ezQtConnection;
struct ezSelectionManagerEvent;

class EZ_GUIFOUNDATION_DLL ezQtNodeScene : public QGraphicsScene
{
  Q_OBJECT
public:
  enum Type
  {
    Node = QGraphicsItem::UserType + 1,
    Pin,
    Connection
  };

  explicit ezQtNodeScene(QObject* parent = nullptr);
  ~ezQtNodeScene();

  void SetDocumentNodeManager(const ezDocumentNodeManager* pManager);
  const ezDocumentNodeManager* GetDocumentNodeManager() const;
  const ezDocument* GetDocument() const;

  static ezRttiMappedObjectFactory<ezQtNode>& GetNodeFactory();
  static ezRttiMappedObjectFactory<ezQtPin>& GetPinFactory();
  static ezRttiMappedObjectFactory<ezQtConnection>& GetConnectionFactory();
  static ezVec2 GetLastMouseInteractionPos() { return s_LastMouseInteraction; }

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* /*event*/) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
  virtual void keyPressEvent(QKeyEvent* event) override;

private:
  void Clear();
  void CreateNode(const ezDocumentObject* pObject);
  void DeleteNode(const ezDocumentObject* pObject);
  void NodeEventsHandler(const ezDocumentNodeManagerEvent& e);
  void PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e);
  void SelectionEventsHandler(const ezSelectionManagerEvent& e);
  void GetSelection(ezDeque<const ezDocumentObject*>& selection) const;
  void GetSelectedNodes(ezDeque<ezQtNode*>& selection) const;
  void ConnectPins(const ezConnection* pConnection);
  void DisconnectPins(const ezConnection* pConnection);
  void MarkupConnectablePins(ezQtPin* pSourcePin);
  void ResetConnectablePinMarkup();

protected:
  virtual ezStatus RemoveNode(ezQtNode* pNode);
  virtual void RemoveSelectedNodesAction();
  virtual void ConnectPinsAction(const ezPin* pSourcePin, const ezPin* pTargetPin);
  virtual void DisconnectPinsAction(ezQtConnection* pConnection);
  virtual void DisconnectPinsAction(ezQtPin* pPin);

private Q_SLOTS:
  void OnMenuAction();
  void CreateNode(const ezRTTI* pRtti);
  void OnMenuItemTriggered(const QString& sName, const QVariant& variant);

private:
  static ezRttiMappedObjectFactory<ezQtNode> s_NodeFactory;
  static ezRttiMappedObjectFactory<ezQtPin> s_PinFactory;
  static ezRttiMappedObjectFactory<ezQtConnection> s_ConnectionFactory;

protected:
  const ezDocumentNodeManager* m_pManager;

  ezMap<const ezDocumentObject*, ezQtNode*> m_Nodes;
  ezMap<const ezConnection*, ezQtConnection*> m_ConnectionsSourceTarget;

private:
  bool m_bIgnoreSelectionChange;
  ezQtPin* m_pStartPin;
  ezQtConnection* m_pTempConnection;
  ezDeque<const ezDocumentObject*> m_Selection;
  ezVec2 m_vPos;
  QString m_sContextMenuSearchText;
  ezDynamicArray<const ezQtPin*> m_ConnectablePins;

  static ezVec2 s_LastMouseInteraction;
};


