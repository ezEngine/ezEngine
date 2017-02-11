#pragma once

#include <GuiFoundation/Basics.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <QGraphicsPathItem>

class ezPin;

class EZ_GUIFOUNDATION_DLL ezQtConnection : public QGraphicsPathItem
{
public:
  explicit ezQtConnection(QGraphicsItem* parent = 0);
  ~ezQtConnection();
  virtual int type() const override { return ezQtNodeScene::Connection; }

  void SetPosIn(const QPointF& point);
  void SetPosOut(const QPointF& point);
  void SetDirIn(const QPointF& dir);
  void SetDirOut(const QPointF& dir);

  void SetConnection(const ezConnection* pConnection) { m_pConnection = pConnection; }
  const ezConnection* GetConnection() const { return m_pConnection; }

  virtual void UpdateConnection();
  virtual QPen DeterminePen() const;

protected:
  const ezConnection* m_pConnection;
  QPointF m_InPoint;
  QPointF m_OutPoint;
  QPointF m_InDir;
  QPointF m_OutDir;
};
