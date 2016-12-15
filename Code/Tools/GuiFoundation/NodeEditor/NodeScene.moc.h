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
  void GetSelection(ezDeque<const ezDocumentObject*>& selection) const;
  void GetSelectedNodes(ezDeque<ezQtNode*>& selection) const;
  void ConnectPins(const ezConnection* pConnection);
  void DisconnectPins(const ezConnection* pConnection);

protected:
  virtual ezStatus RemoveNode(ezQtNode* pNode);
  virtual void RemoveSelectedNodesAction();
  virtual void ConnectPinsAction(const ezPin* pSourcePin, const ezPin* pTargetPin);
  virtual void DisconnectPinsAction(ezQtConnection* pConnection);
  virtual void DisconnectPinsAction(ezQtPin* pPin);
  virtual void CopySelectedNodes();

private slots:
  void OnMenuAction();
  void CreateNode(const ezRTTI* pRtti);
  void OnMenuItemTriggered(const QString& sName, const QVariant& variant);

private:
  static ezRttiMappedObjectFactory<ezQtNode> s_NodeFactory;
  static ezRttiMappedObjectFactory<ezQtPin> s_PinFactory;
  static ezRttiMappedObjectFactory<ezQtConnection> s_ConnectionFactory;

protected:
  const ezDocumentNodeManager* m_pManager;

private:
  ezMap<const ezDocumentObject*, ezQtNode*> m_Nodes;
  ezMap<const ezConnection*, ezQtConnection*> m_ConnectionsSourceTarget;

  ezQtPin* m_pStartPin;
  ezQtConnection* m_pTempConnection;
  ezDeque<const ezDocumentObject*> m_Selection;
  ezVec2 m_vPos;
  QString m_sContextMenuSearchText;
};


