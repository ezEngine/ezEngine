#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Math/Curve1D.h>
#include <Foundation/Containers/Set.h>
#include <Code/Tools/GuiFoundation/ui_Curve1DEditorWidget.h>

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsItem>

class QMouseEvent;
class ezQtCurve1DEditorWidget;

class ezQCurveControlPoint : public QGraphicsEllipseItem
{
public:
  ezQCurveControlPoint(QGraphicsItem* parent = nullptr);

  virtual int type() const override { return QGraphicsItem::UserType + 1; }

  ezQtCurve1DEditorWidget* m_pOwner;
  ezUInt32 m_uiCurveIdx;
  ezUInt32 m_uiControlPoint;

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};

class ezQCurveSegment : public QGraphicsPathItem
{
public:
  ezQCurveSegment(QGraphicsItem* parent = nullptr);

  virtual int type() const override { return QGraphicsItem::UserType + 2; }

  ezQtCurve1DEditorWidget* m_pOwner;
  ezUInt32 m_uiCurveIdx;
  ezUInt32 m_uiSegment;

  void UpdateSegment();
};

class ezQCurveTangent : public QGraphicsEllipseItem
{
public:
  ezQCurveTangent(QGraphicsItem* parent = nullptr);

  virtual int type() const override { return QGraphicsItem::UserType + 3; }

  ezQtCurve1DEditorWidget* m_pOwner;
  ezUInt32 m_uiCurveIdx;
  ezUInt32 m_uiControlPoint;
  bool m_bRightTangent;

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
};

struct ControlPointMove
{
  ezUInt32 curveIdx;
  ezUInt32 cpIdx;
  ezInt32 tangentIdx; // 0 == point, 1 == left tangent, 2 == right tangent
  float x;
  float y;

  bool operator<(const ControlPointMove& rhs) const
  {
    if (curveIdx < rhs.curveIdx)
      return true;
    if (curveIdx > rhs.curveIdx)
      return false;
    if (cpIdx < rhs.cpIdx)
      return true;
    if (cpIdx > rhs.cpIdx)
      return false;

    return tangentIdx < rhs.tangentIdx;
  }
};

class EZ_GUIFOUNDATION_DLL ezQtCurve1DEditorWidget : public QWidget, public Ui_Curve1DEditorWidget
{
  Q_OBJECT

public:
  explicit ezQtCurve1DEditorWidget(QWidget* pParent);
  ~ezQtCurve1DEditorWidget();

  void SetNumCurves(ezUInt32 num);

  void SetCurve1D(ezUInt32 idx, const ezCurve1D& curve);
  const ezCurve1D& GetCurve1D(ezUInt32 idx) const { return m_Curves[idx].m_Curve; }

  void FrameCurve();

  //void SetControlPoint(ezUInt32 curveIdx, ezUInt32 cpIdx, float x, float y);
  void SetControlPoints(const ezSet<ControlPointMove>& moves);



signals:
  //void CpAdded(float posX, float value);
  void CpMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY);
  void CpDeleted(ezUInt32 curveIdx, ezUInt32 cpIdx);
  void TangentMoved(ezUInt32 curveIdx, ezUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent);

  void NormalizeRangeX();
  void NormalizeRangeY();

  void BeginCpChanges();
  void EndCpChanges();

  void BeginOperation();
  void EndOperation(bool commit);


private slots:
  void on_ButtonFrame_clicked();
  void on_SpinPosition_valueChanged(double value);
  void on_SpinValue_valueChanged(double value);
  void on_ButtonNormalizeX_clicked();
  void on_ButtonNormalizeY_clicked();
  void onDeleteCPs();

private:
  void UpdateCpUi();

  struct Data
  {
    ezCurve1D m_Curve;
    ezHybridArray<ezQCurveControlPoint*, 10> m_ControlPoints;
    ezHybridArray<ezQCurveSegment*, 10> m_Segments;
    ezHybridArray<ezQCurveTangent*, 10> m_TangentsLeft;
    ezHybridArray<ezQCurveTangent*, 10> m_TangentsRight;
  };

  ezHybridArray<Data, 4> m_Curves;
  QGraphicsScene m_Scene;
};
