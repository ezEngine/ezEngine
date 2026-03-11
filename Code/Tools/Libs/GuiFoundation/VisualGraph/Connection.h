#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <QGraphicsPathItem>

class ezVisualGraphPin;

/// Qt graphics item representing a connection between two pins in a visual graph.
///
/// Renders the visual connection path between pins using different styles (bezier curves, straight lines, or subway-style routing).
/// Updates its geometry automatically when connected pins move.
class EZ_GUIFOUNDATION_DLL ezQtVisualGraphConnection : public QGraphicsPathItem
{
public:
  explicit ezQtVisualGraphConnection(QGraphicsItem* pParent = 0);
  ~ezQtVisualGraphConnection();
  virtual int type() const override { return ezQtVisualGraphScene::Connection; }

  const ezDocumentObject* GetObject() const { return m_pObject; }
  const ezVisualGraphConnection* GetConnection() const { return m_pConnection; }
  void InitConnection(const ezDocumentObject* pObject, const ezVisualGraphConnection* pConnection);

  void SetPosIn(const QPointF& point);
  void SetPosOut(const QPointF& point);
  void SetDirIn(const QPointF& dir);
  void SetDirOut(const QPointF& dir);

  virtual void UpdateGeometry();
  virtual QPen DeterminePen() const;

  const QPointF& GetInPos() const { return m_InPoint; }
  const QPointF& GetOutPos() const { return m_OutPoint; }

  bool m_bAdjacentNodeSelected = false;

  virtual void ExtendContextMenu(QMenu& ref_menu) {}

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

  // Draws connections following the rules of subway maps (angles of 45 degrees only).
  void DrawSubwayPath(QPainterPath& path, const QPointF& startPoint, const QPointF& endPoint);

  const ezDocumentObject* m_pObject = nullptr;
  const ezVisualGraphConnection* m_pConnection = nullptr;

  QPointF m_InPoint;
  QPointF m_OutPoint;
  QPointF m_InDir;
  QPointF m_OutDir;
};
