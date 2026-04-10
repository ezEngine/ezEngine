#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/Node.h>

class QGraphicsTextItem;
class ezVisualGraphObjectManager;

/// Qt graphics item for comment boxes in visual graphs, similar to Blueprint comments.
class EZ_GUIFOUNDATION_DLL ezQtVisualGraphCommentNode : public ezQtVisualGraphNode
{
public:
  ezQtVisualGraphCommentNode();

  virtual void InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject) override;
  virtual void UpdateGeometry() override;
  virtual void UpdateState() override;

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
  virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  virtual void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;

private:
  enum ResizeEdge : ezUInt8
  {
    None = 0,
    Left = EZ_BIT(0),
    Right = EZ_BIT(1),
    Top = EZ_BIT(2),
    Bottom = EZ_BIT(3),
  };

  ezUInt8 DetectResizeEdge(const QPointF& localPos) const;
  void UpdateCursorForEdge(ezUInt8 uiEdge);

  const ezVisualGraphObjectManager* m_pManager = nullptr;
  QGraphicsTextItem* m_pCommentLabel = nullptr;
  QColor m_CommentColor;

  // Resize state
  ezUInt8 m_uiActiveResizeEdge = None;
  QPointF m_ResizeStartMouseScene;
  QPointF m_ResizeStartPos;
  ezVec2 m_vResizeStartSize;
  ezVec2 m_vCurrentSize = ezVec2(300, 200);

  // Containment tracking: when moving the comment, contained nodes move with it
  QPointF m_PrevPos;
  ezDynamicArray<QGraphicsItem*> m_ContainedNodes;

  static constexpr float s_fEdgeThreshold = 10.0f;
  static constexpr float s_fHeaderHeight = 26.0f;
  static constexpr float s_fMinWidth = 120.0f;
  static constexpr float s_fMinHeight = 80.0f;
};
