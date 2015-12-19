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
  const ezDocumentNodeManager* GetDocumentNodeManager();

  static ezRttiMappedObjectFactory<ezQtNode>& GetNodeFactory();
  static ezRttiMappedObjectFactory<ezQtPin>& GetPinFactory();
  static ezRttiMappedObjectFactory<ezQtConnection>& GetConnectionFactory();

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;

private:
  void Clear();
  void CreateNode(const ezDocumentObject* pObject);
  void DeleteNode(const ezDocumentObject* pObject);
  void NodeEventsHandler(const ezDocumentNodeManagerEvent& e);
  void GetSelection(ezDeque<const ezDocumentObject*>& selection);
  void ConnectPins(const ezConnection* pConnection);
  void DisconnectPins(const ezConnection* pConnection);

  void RemoveNodeAction(ezQtNode* pNode);
  void ConnectPinsAction(const ezPin* pSourcePin, const ezPin* pTargetPin);
  void DisconnectPinsAction(ezQtConnection* pConnection);
  void DisconnectPinsAction(ezQtPin* pPin);

private slots:
  void OnMenuAction();

private:
  static ezRttiMappedObjectFactory<ezQtNode> s_NodeFactory;
  static ezRttiMappedObjectFactory<ezQtPin> s_PinFactory;
  static ezRttiMappedObjectFactory<ezQtConnection> s_ConnectionFactory;

private:
  const ezDocumentNodeManager* m_pManager;
  ezMap<const ezDocumentObject*, ezQtNode*> m_Nodes;
  ezMap<const ezConnection*, ezQtConnection*> m_ConnectionsSourceTarget;

  ezQtPin* m_pStartPin;
  ezQtConnection* m_pTempConnection;
  ezDeque<const ezDocumentObject*> m_Selection;
  ezVec2 m_vPos;
};
