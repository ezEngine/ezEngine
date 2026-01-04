#pragma once

#include <Foundation/Containers/IdTable.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezGALTextureCreationDescription;

/// Manages dynamic allocation of rectangular regions within a GPU texture.
///
/// Uses a binary tree structure with guillotine algorithm for efficient 2D bin packing.
/// Useful for atlasing frequently updated textures like runtime generated decals, fonts or UI elements.
/// Supports alignment requirements and can visualize allocations for debugging.
class EZ_RENDERERCORE_DLL ezDynamicTextureAtlas
{
public:
  using AllocationId = ezGenericId<16, 8>;

  ezDynamicTextureAtlas();
  ~ezDynamicTextureAtlas();

  /// Initializes the atlas with the given texture description and alignment.
  ///
  /// The texture must not be immutable. Width and height must be multiples of alignment.
  /// Alignment must be a power of 2. Returns failure if already initialized.
  ezResult Initialize(const ezGALTextureCreationDescription& textureDesc, ezUInt32 uiAlignment = 16);

  /// Destroys the GPU texture and clears all allocations.
  void Deinitialize();

  bool IsInitialized() const { return m_hTexture.IsInvalidated() == false; }

  /// Allocates a rectangular region of the specified size.
  ///
  /// Dimensions are aligned up to the atlas alignment. Returns invalid ID if the allocation fails
  /// due to insufficient space. The name is used for debugging visualization.
  AllocationId Allocate(ezUInt32 uiWidth, ezUInt32 uiHeight, ezStringView sName, ezRectU16* out_pRect = nullptr);

  /// Deallocates a previously allocated region and invalidates the ID.
  void Deallocate(AllocationId& ref_allocationId);

  /// Clears all allocations without destroying the texture.
  void Clear();

  ezGALTextureHandle GetTexture() const { return m_hTexture; }

  /// Returns the rectangle for a given allocation ID.
  ezRectU16 GetAllocationRect(AllocationId id) const;

  /// Renders a debug visualization of the atlas layout.
  ///
  /// Shows allocated regions with their names.
  /// If bBlackOutFreeAreas is true, free space is rendered as black rectangles.
  void DebugDraw(const ezDebugRendererContext& debugContext, float fViewWidth, float fViewHeight, bool bBlackOutFreeAreas = true) const;

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
