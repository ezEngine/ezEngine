#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <QGraphicsWidget>

// Avoid conflicts with windows.
#ifdef GetObject
#  undef GetObject
#endif

class ezQtVisualGraphPin;
class ezVisualGraphObjectManager;
class QLabel;
class ezDocumentObject;
class QGraphicsTextItem;
class QGraphicsPixmapItem;
class QGraphicsDropShadowEffect;

struct ezQtVisualGraphNodeFlags
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None = 0,
    Moved = EZ_BIT(0),
    UpdateTitle = EZ_BIT(1),
    Default = None
  };

  struct Bits
  {
    StorageType Moved : 1;
    StorageType UpdateTitle : 1;
  };
};

/// Qt graphics item representing a single node in a visual graph.
///
/// Displays the node's title, optional subtitle, icon, and manages its pins.
/// Handles rendering, selection, and user interaction for the node.
/// Derive from this class to customize node appearance for specific graph types.
class EZ_GUIFOUNDATION_DLL ezQtVisualGraphNode : public QGraphicsPathItem
{
public:
  ezQtVisualGraphNode();
  ~ezQtVisualGraphNode();
  virtual int type() const override { return ezQtVisualGraphScene::Node; }

  const ezDocumentObject* GetObject() const { return m_pObject; }
  virtual void InitNode(const ezVisualGraphObjectManager* pManager, const ezDocumentObject* pObject);

  virtual void UpdateGeometry();

  void CreatePins();

  ezQtVisualGraphPin* GetInputPin(const ezVisualGraphPin& pin);
  ezQtVisualGraphPin* GetOutputPin(const ezVisualGraphPin& pin);

  ezBitflags<ezQtVisualGraphNodeFlags> GetFlags() const;
  void ResetFlags();

  void EnableDropShadow(bool bEnable);
  virtual void UpdateState();

  const ezHybridArray<ezQtVisualGraphPin*, 6>& GetInputPins() const { return m_Inputs; }
  const ezHybridArray<ezQtVisualGraphPin*, 6>& GetOutputPins() const { return m_Outputs; }

  void SetActive(bool bActive);

  virtual void ExtendContextMenu(QMenu& ref_menu) {}

protected:
  virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
  virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

  QColor m_HeaderColor;
  QRectF m_HeaderRect;
  QGraphicsTextItem* m_pTitleLabel = nullptr;
  QGraphicsTextItem* m_pSubtitleLabel = nullptr;
  QGraphicsPixmapItem* m_pIcon = nullptr;

private:
  const ezVisualGraphObjectManager* m_pManager = nullptr;
  const ezDocumentObject* m_pObject = nullptr;
  ezBitflags<ezQtVisualGraphNodeFlags> m_DirtyFlags;

  bool m_bIsActive = true;

  QGraphicsDropShadowEffect* m_pShadow = nullptr;

  // Pins
  ezHybridArray<ezQtVisualGraphPin*, 6> m_Inputs;
  ezHybridArray<ezQtVisualGraphPin*, 6> m_Outputs;
};
