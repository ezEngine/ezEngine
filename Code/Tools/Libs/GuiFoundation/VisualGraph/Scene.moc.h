#pragma once

#include <Foundation/Containers/Map.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

class ezQtVisualGraphNode;
class ezQtVisualGraphPin;
class ezQtVisualGraphConnection;
struct ezSelectionManagerEvent;

/// Qt graphics scene for displaying and interacting with visual graphs.
///
/// This class manages the visual representation of node graphs, including rendering nodes, pins, and connections.
/// It handles user interactions such as node creation, connection dragging, and selection.
/// Works in conjunction with ezVisualGraphObjectManager which manages the document-side graph data.
class EZ_GUIFOUNDATION_DLL ezQtVisualGraphScene : public QGraphicsScene
{
  Q_OBJECT
public:
  enum Type
  {
    Node = QGraphicsItem::UserType + 1,
    Pin,
    Connection
  };

  explicit ezQtVisualGraphScene(QObject* pParent = nullptr);
  ~ezQtVisualGraphScene();

  virtual void InitScene(const ezVisualGraphObjectManager* pManager);
  const ezVisualGraphObjectManager* GetDocumentNodeManager() const;
  const ezDocument* GetDocument() const;

  static ezRttiMappedObjectFactory<ezQtVisualGraphNode>& GetNodeFactory();
  static ezRttiMappedObjectFactory<ezQtVisualGraphPin>& GetPinFactory();
  static ezRttiMappedObjectFactory<ezQtVisualGraphConnection>& GetConnectionFactory();
  static ezVec2 GetLastMouseInteractionPos() { return s_vLastMouseInteraction; }

  /// Visual style for rendering connections between pins
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
  void CreateNodeObject(const ezVisualGraphNodeDesc& nodeTemplate);
  void NodeEventsHandler(const ezVisualGraphObjectManagerEvent& e);
  void PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e);
  void SelectionEventsHandler(const ezSelectionManagerEvent& e);
  void GetSelectedNodes(ezDeque<ezQtVisualGraphNode*>& selection) const;
  void MarkupConnectablePins(ezQtVisualGraphPin* pSourcePin);
  void ResetConnectablePinMarkup();
  void OpenSearchMenu(QPoint screenPos);

protected:
  virtual ezStatus RemoveNode(ezQtVisualGraphNode* pNode);
  virtual void RemoveSelectedNodesAction();
  virtual void ConnectPinsAction(const ezVisualGraphPin& sourcePin, const ezVisualGraphPin& targetPin);
  virtual void DisconnectPinsAction(ezQtVisualGraphConnection* pConnection);
  virtual void DisconnectPinsAction(ezQtVisualGraphPin* pPin);

private Q_SLOTS:
  void OnMenuItemTriggered(const QString& sName, const QVariant& variant);
  void OnSelectionChanged();

private:
  static ezRttiMappedObjectFactory<ezQtVisualGraphNode> s_NodeFactory;
  static ezRttiMappedObjectFactory<ezQtVisualGraphPin> s_PinFactory;
  static ezRttiMappedObjectFactory<ezQtVisualGraphConnection> s_ConnectionFactory;

protected:
  const ezVisualGraphObjectManager* m_pManager = nullptr;

  ezMap<const ezDocumentObject*, ezQtVisualGraphNode*> m_Nodes;
  ezMap<const ezDocumentObject*, ezQtVisualGraphConnection*> m_Connections;

private:
  bool m_bIgnoreSelectionChange = false;
  ezQtVisualGraphPin* m_pStartPin = nullptr;
  ezQtVisualGraphConnection* m_pTempConnection = nullptr;
  ezQtVisualGraphNode* m_pTempNode = nullptr;
  ezDeque<const ezDocumentObject*> m_Selection;
  ezVec2 m_vMousePos = ezVec2::MakeZero();
  QString m_sContextMenuSearchText;
  ezDynamicArray<const ezQtVisualGraphPin*> m_ConnectablePins;
  ezEnum<ConnectionStyle> m_ConnectionStyle;
  ezBitflags<ConnectionDecorationFlags> m_ConnectionDecorationFlags;

  ezDynamicArray<ezVisualGraphNodeDesc> m_NodeCreationTemplates;

  static ezVec2 s_vLastMouseInteraction;
};

EZ_DECLARE_FLAGS_OPERATORS(ezQtVisualGraphScene::ConnectionDecorationFlags);
