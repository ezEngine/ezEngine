#pragma once

#include <Foundation/Containers/Map.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

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

  explicit ezQtNodeScene(QObject* pParent = nullptr);
  ~ezQtNodeScene();

  virtual void InitScene(const ezDocumentNodeManager* pManager);
  const ezDocumentNodeManager* GetDocumentNodeManager() const;
  const ezDocument* GetDocument() const;

  static ezRttiMappedObjectFactory<ezQtNode>& GetNodeFactory();
  static ezRttiMappedObjectFactory<ezQtPin>& GetPinFactory();
  static ezRttiMappedObjectFactory<ezQtConnection>& GetConnectionFactory();
  static ezVec2 GetLastMouseInteractionPos() { return s_vLastMouseInteraction; }

  struct ConnectionStyle
  {
    using StorageType = ezUInt32;

    enum Enum
    {
      BezierCurve,
      StraightLine,
      SubwayLines,

      Default = BezierCurve
    };
  };

  void SetConnectionStyle(ezEnum<ConnectionStyle> style);
  ezEnum<ConnectionStyle> GetConnectionStyle() const { return m_ConnectionStyle; }

  struct ConnectionDecorationFlags
  {
    using StorageType = ezUInt32;

    enum Enum
    {
      DirectionArrows = EZ_BIT(0), ///< Draw an arrow to indicate the connection's direction. Only works with straight lines atm.
      DrawDebugging = EZ_BIT(1),   ///< Draw animated effect to denote debugging.

      Default = 0
    };

    struct Bits
    {
      StorageType DirectionArrows : 1;
      StorageType DrawDebugging : 1;
    };
  };

  void SetConnectionDecorationFlags(ezBitflags<ConnectionDecorationFlags> flags);
  ezBitflags<ConnectionDecorationFlags> GetConnectionDecorationFlags() const { return m_ConnectionDecorationFlags; }

protected:
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent) override;
  virtual void keyPressEvent(QKeyEvent* event) override;

private:
  void Clear();
  void CreateQtNode(const ezDocumentObject* pObject);
  void DeleteQtNode(const ezDocumentObject* pObject);
  void CreateQtConnection(const ezDocumentObject* pObject);
  void DeleteQtConnection(const ezDocumentObject* pObject);
  void RecreateQtPins(const ezDocumentObject* pObject);
  void CreateNodeObject(const ezNodeCreationTemplate& nodeTemplate);
  void NodeEventsHandler(const ezDocumentNodeManagerEvent& e);
  void PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e);
  void SelectionEventsHandler(const ezSelectionManagerEvent& e);
  void GetSelectedNodes(ezDeque<ezQtNode*>& selection) const;
  void MarkupConnectablePins(ezQtPin* pSourcePin);
  void ResetConnectablePinMarkup();
  void OpenSearchMenu(QPoint screenPos);

protected:
  virtual ezStatus RemoveNode(ezQtNode* pNode);
  virtual void RemoveSelectedNodesAction();
  virtual void ConnectPinsAction(const ezPin& sourcePin, const ezPin& targetPin);
  virtual void DisconnectPinsAction(ezQtConnection* pConnection);
  virtual void DisconnectPinsAction(ezQtPin* pPin);

private Q_SLOTS:
  void OnMenuItemTriggered(const QString& sName, const QVariant& variant);
  void OnSelectionChanged();

private:
  static ezRttiMappedObjectFactory<ezQtNode> s_NodeFactory;
  static ezRttiMappedObjectFactory<ezQtPin> s_PinFactory;
  static ezRttiMappedObjectFactory<ezQtConnection> s_ConnectionFactory;

protected:
  const ezDocumentNodeManager* m_pManager = nullptr;

  ezMap<const ezDocumentObject*, ezQtNode*> m_Nodes;
  ezMap<const ezDocumentObject*, ezQtConnection*> m_Connections;

private:
  bool m_bIgnoreSelectionChange = false;
  ezQtPin* m_pStartPin = nullptr;
  ezQtConnection* m_pTempConnection = nullptr;
  ezQtNode* m_pTempNode = nullptr;
  ezDeque<const ezDocumentObject*> m_Selection;
  ezVec2 m_vMousePos = ezVec2::MakeZero();
  QString m_sContextMenuSearchText;
  ezDynamicArray<const ezQtPin*> m_ConnectablePins;
  ezEnum<ConnectionStyle> m_ConnectionStyle;
  ezBitflags<ConnectionDecorationFlags> m_ConnectionDecorationFlags;

  ezDynamicArray<ezNodeCreationTemplate> m_NodeCreationTemplates;

  static ezVec2 s_vLastMouseInteraction;
};

EZ_DECLARE_FLAGS_OPERATORS(ezQtNodeScene::ConnectionDecorationFlags);
