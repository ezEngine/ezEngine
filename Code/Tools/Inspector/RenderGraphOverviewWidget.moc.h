#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QWidget>
#include <RendererCore/RenderGraph/RenderGraphInspectionInfo.h>
#include <RendererCore/RenderGraph/RenderGraphPassObserver.h>

class QMouseEvent;
class QPaintEvent;
class QPainter;

/// Visualizes render graph passes, resources, lifetimes, and access points for inspection.
class ezQtRenderGraphOverviewWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ezQtRenderGraphOverviewWidget(QWidget* pParent = nullptr);

  void SetInfo(ezUInt64 uiSelectedGraphId, const ezRenderGraphInspectionInfo& info);
  void SetRequest(const ezRenderGraphObserverRequest& request);
  void Clear();

Q_SIGNALS:
  void AccessSelected(ezUInt16 uiPassIndex, ezUInt16 uiAccessIndex);
  void AccessDeselected();

protected:
  void paintEvent(QPaintEvent*) override;
  void mousePressEvent(QMouseEvent* e) override;
  void mouseMoveEvent(QMouseEvent* e) override;

private:
  /// Maps one displayed texture column to the texture resource it represents.
  struct TextureColumnResource
  {
    ezUInt16 m_uiResolvedIndex = 0xFFFF;
    ezUInt16 m_uiTextureIndex = 0xFFFF;
  };

  /// Maps one displayed buffer column to the buffer resource it represents.
  struct BufferColumnResource
  {
    ezUInt16 m_uiResolvedIndex = 0xFFFF;
    ezUInt16 m_uiBufferIndex = 0xFFFF;
  };

  /// Describes the render graph overview element currently under the mouse cursor.
  struct HoverInfo
  {
    enum class Area
    {
      None,
      PassHeader,
      TextureHeader,
      BufferHeader,
      ResourceCell,
    };

    Area m_Area = Area::None;
    QPoint m_Position;
    ezUInt16 m_uiPassIndex = 0xFFFF;
    ezUInt16 m_uiResourceColumn = 0xFFFF;
    bool m_bIsTexture = true;
  };



  HoverInfo GetHoverInfo(const QPoint& pos) const;
  const ezRenderGraphInspectionInfo::AccessInfo* FindAccess(const HoverInfo& hover, bool bLastMatch = false) const;
  QString MakeAccessesTooltip(const HoverInfo& hover) const;
  ezUInt16 FindTextureLifetimeResource(const HoverInfo& hover) const;
  ezUInt16 FindBufferLifetimeResource(const HoverInfo& hover) const;
  QString MakeHoverTooltip(const HoverInfo& hover) const;
  QString MakeTextureColumnTooltip(ezUInt32 uiResolvedIndex) const;
  QString MakeBufferColumnTooltip(ezUInt32 uiResolvedIndex) const;
  QRect GetCellRect(ezUInt32 uiPassIndex, ezUInt32 uiResourceColumn) const;
  void DrawResourceNames(QPainter& painter);
  void DrawPassNames(QPainter& painter);
  void DrawGrid(QPainter& painter);
  void DrawResourceLifetimes(QPainter& painter);
  QRect GetLifetimeRect(ezUInt32 uiFirstPass, ezUInt32 uiLastPass, ezUInt32 uiResourceColumn) const;
  void DrawAccesses(QPainter& painter);

  static constexpr ezInt32 s_iCellSize = 18;
  static constexpr ezInt32 s_iPassLabelWidth = 200;
  static constexpr ezInt32 s_iHeaderHeight = 40;
  static constexpr ezInt32 s_iContentPadding = 12;
  static constexpr ezInt32 s_iLifetimeGap = 2;
  static constexpr ezInt32 s_iAccessPadding = 4;

  // Input inspection info
  ezUInt64 m_uiSelectedGraphId = 0;
  ezRenderGraphInspectionInfo m_Info;
  QSize m_MinContentSize = QSize(600, 300);

  // Caches
  ezUInt32 m_uiTextureColumnCount = 0;
  ezUInt32 m_uiBufferColumnCount = 0;
  ezUInt32 m_uiTotalResourceColumnCount = 0;
  ezDynamicArray<TextureColumnResource> m_TextureColumnResources; ///< Find transient textures matching the same m_uiResolvedIndex
  ezDynamicArray<BufferColumnResource> m_BufferColumnResources;   ///< Find transient buffers matching the same m_uiResolvedIndex

  // Observed resource
  ezString m_sObservedPassName;
  ezUInt16 m_uiObservedAccessIndex = 0;
};
