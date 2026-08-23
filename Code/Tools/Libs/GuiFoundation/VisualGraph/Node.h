#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>
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
class ezAbstractProperty;

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

  /// Controls how UpdateState() turns values into title text.
  struct TitleFormat
  {
    ezUInt32 m_uiMaxStringLength = 23;       ///< Strings longer than this are truncated. 0 disables truncation.
    ezUInt32 m_uiMaxTitleLength = 0;         ///< The fully rendered title is truncated to this length. 0 disables truncation.
    bool m_bQuoteStrings = true;             ///< Whether string values are enclosed in quotes.
    bool m_bSplitAtDoubleColon = true;       ///< Whether the part of the title before a '::' becomes the subtitle.
    bool m_bBoolsAsTicks = false;            ///< If set, bools turn into [x]/[ ] instead of 'true'/'false'
    bool m_bIgnoreInvalidProperties = false; ///< If set, any invalid property value is ignored, i.e. resolves to "".
  };

  bool TryGetTitleTemplateFromProperty(ezStringView sPropertyName, ezStringBuilder& out_sTemplate);
  bool TryGetTitleTemplateFromAttribute(ezStringBuilder& out_sTemplate);
  void GetDefaultTitleTemplate(ezStringBuilder& out_sTemplate);
  void ResolvePropertyPlaceholder(ezStringView sPlaceholder, const ezVariant& index, bool bOptional, const TitleFormat& format, ezStringBuilder& ref_sOutput);
  void AppendPropertyValue(const ezAbstractProperty* pProp, const ezVariant& index, bool bOptional, const TitleFormat& format, ezStringBuilder& ref_sOutput);
  void SetTitleAndSubtitle(ezStringView sTitle, const TitleFormat& format);

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
