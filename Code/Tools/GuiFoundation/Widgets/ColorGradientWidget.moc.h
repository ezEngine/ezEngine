#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Math/ColorGradient.h>

#include <QWidget>

class QMouseEvent;

class EZ_GUIFOUNDATION_DLL ezQtColorGradientWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ezQtColorGradientWidget(QWidget* pParent);
  ~ezQtColorGradientWidget();

  void setColorGradientData(const ezColorGradient* gradient);

  void setEditMode(bool edit);
  void setShowColorCPs(bool show);
  void setShowAlphaCPs(bool show);
  void setShowIntensityCPs(bool show);
  void setShowCoords(bool top, bool bottom);

  void FrameExtents();
  void ClearSelectedCP();
  void SelectCP(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP);

signals:
  void GradientClicked();
  void addColorCp(float posX, const ezColorGammaUB& color);
  void addAlphaCp(float posX, ezUInt8 value);
  void addIntensityCp(float posX, float intensity);
  void moveColorCpToPos(ezInt32 index, float newPosX);
  void moveAlphaCpToPos(ezInt32 index, float newPosX);
  void moveIntensityCpToPos(ezInt32 index, float newPosX);
  void deleteColorCp(ezInt32 index);
  void deleteAlphaCp(ezInt32 index);
  void deleteIntensityCp(ezInt32 index);
  void selectionChanged(ezInt32 colorCP, ezInt32 alphaCP, ezInt32 intensityCP);
  void beginOperation();
  void endOperation(bool commit);
  void triggerPickColor();

private:
  enum class Area
  {
    None = 0,
    Gradient = 1,
    ColorCPs = 2,
    AlphaCPs = 3,
    IntensityCPs = 4,
  };


  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseReleaseEvent(QMouseEvent* event);
  virtual void mouseDoubleClickEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event) override;

  void UpdateMouseCursor(QMouseEvent* event);

  virtual void wheelEvent(QWheelEvent* event) override;

  void ClampDisplayExtents(float zoomCenter = 0.5f);

  virtual void keyPressEvent(QKeyEvent* event) override;

  void PaintColorGradient(QPainter& p) const;
  void PaintCpBackground(QPainter& p, const QRect& area) const;
  void PaintColorCpArea(QPainter& p);
  void PaintAlphaCpArea(QPainter& p);
  void PaintIntensityCpArea(QPainter& p);
  void PaintCoordinateStrips(QPainter& p) const;
  void PaintCoordinateStrip(QPainter& p, const QRect& area) const;
  void PaintCoordinateLines(QPainter& p);

  void PaintControlPoint(QPainter& p, const QRect& area, float posX, const ezColorGammaUB& outlineColor, const ezColorGammaUB& fillColor, bool selected) const;
  void PaintColorCPs(QPainter& p) const;
  void PaintAlphaCPs(QPainter& p) const;

  QRect GetColorCpArea() const;
  QRect GetAlphaCpArea() const;
  QRect GetIntensityCpArea() const;
  QRect GetGradientArea() const;
  QRect GetCoordAreaTop() const;
  QRect GetCoordAreaBottom() const;

  float WindowToGradientCoord(ezInt32 mouseWindowPosX) const;
  ezInt32 GradientToWindowCoord(float gradientPosX) const;

  ezInt32 FindClosestColorCp(ezInt32 iWindowPosX) const;
  ezInt32 FindClosestAlphaCp(ezInt32 iWindowPosX) const;
  ezInt32 FindClosestIntensityCp(ezInt32 iWindowPosX) const;

  bool HoversControlPoint(const QPoint& windowPos) const;
  bool HoversControlPoint(const QPoint& windowPos, ezInt32& iHoverColorCp, ezInt32& iHoverAlphaCp, ezInt32& iHoverIntensityCp) const;
  Area HoversInteractiveArea(const QPoint& windowPos) const;

  void EvaluateAt(ezInt32 windowPos, ezColorGammaUB& rgba, float& intensity) const;

  float ComputeCoordinateDisplayStep() const;

  const ezColorGradient* m_pColorGradientData;

  bool m_bEditMode;
  bool m_bShowColorCPs;
  bool m_bShowAlphaCPs;
  bool m_bShowIntensityCPs;
  bool m_bDraggingCP;
  bool m_bTempMode;
  bool m_bShowCoordsTop;
  bool m_bShowCoordsBottom;

  float m_fDisplayExtentMinX;
  float m_fDisplayExtentMaxX;

  ezInt32 m_iSelectedColorCP;
  ezInt32 m_iSelectedAlphaCP;
  ezInt32 m_iSelectedIntensityCP;

  QPoint m_LastMousePosition;
  QPixmap m_AlphaPattern;
};
