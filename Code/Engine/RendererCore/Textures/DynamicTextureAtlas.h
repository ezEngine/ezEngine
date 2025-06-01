#pragma once

#include <Foundation/Containers/IdTable.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRendererContext.h>

class EZ_RENDERERCORE_DLL ezDynamicTextureAtlas
{
public:
  using AllocationId = ezGenericId<16, 8>;

  ezDynamicTextureAtlas();
  ~ezDynamicTextureAtlas();

  ezResult Initialize(const ezGALTextureCreationDescription& textureDesc, ezUInt32 uiAlignment = 16);
  void Deinitialize();
  bool IsInitialized() const { return m_hTexture.IsInvalidated() == false; }

  AllocationId Allocate(ezUInt32 uiWidth, ezUInt32 uiHeight, ezStringView sName, ezRectU16* out_pRect = nullptr);
  void Deallocate(AllocationId& ref_allocationId);

  void Clear();

  ezGALTextureHandle GetTexture() const { return m_hTexture; }
  ezRectU16 GetAllocationRect(AllocationId id) const;

  void DebugDraw(const ezDebugRendererContext& debugContext, float fViewWidth, float fViewHeight) const;

private:
  struct NodeType
  {
    enum Enum
    {
      Container,
      Allocation,
      Free,

      Count,
    };
  };

  struct Orientation
  {
    enum Enum
    {
      Horizontal,
      Vertical,

      Count,
    };
  };

  EZ_ALWAYS_INLINE static Orientation::Enum FlipOrientation(Orientation::Enum orientation)
  {
    return (orientation == Orientation::Horizontal) ? Orientation::Vertical : Orientation::Horizontal;
  }

  AllocationId AllocateNode(NodeType::Enum type, Orientation::Enum orientation, ezUInt16 uiParentIndex, ezUInt16 uiNextSiblingIndex, ezUInt16 uiPrevSiblingIndex, const ezRectU16& rect);
  AllocationId FindSuitableNode(ezUInt32 uiWidth, ezUInt32 uiHeight);
  void MergeSiblingNodes(ezUInt16 uiNodeIndex, ezUInt16 uiNextSiblingIndex);
  Orientation::Enum GuillotineRect(const ezRectU16& nodeRect, ezUInt32 uiAllocatedWidth, ezUInt32 uiAllocatedHeight, Orientation::Enum defaultOrientation, ezRectU16& out_splitRect, ezRectU16& out_leftoverRect);

  void ClearInternal();
  void AddRootNode();

  ezGALTextureHandle m_hTexture;
  ezUInt32 m_uiAlignment = 0;


  struct Node
  {
    ezHashedString m_sName;
    ezUInt8 m_uiType : 2;
    ezUInt8 m_uiOrientation : 1;
    ezUInt8 m_uiChannelMask = 0; // Not used yet
    ezUInt16 m_uiParentIndex = ezSmallInvalidIndex;
    ezUInt16 m_uiNextSiblingIndex = ezSmallInvalidIndex;
    ezUInt16 m_uiPrevSiblingIndex = ezSmallInvalidIndex;
    ezRectU16 m_Rect;

    EZ_ALWAYS_INLINE NodeType::Enum GetType() const { return static_cast<NodeType::Enum>(m_uiType); }
    EZ_ALWAYS_INLINE Orientation::Enum GetOrientation() const { return static_cast<Orientation::Enum>(m_uiOrientation); }
  };

  ezIdTable<AllocationId, Node> m_Nodes;
  ezDynamicArray<AllocationId> m_FreeList;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  void CheckTree() const;
  void CheckNode(ezUInt16 uiNodeIndex, Orientation::Enum parentOrientation) const;
#endif
};
