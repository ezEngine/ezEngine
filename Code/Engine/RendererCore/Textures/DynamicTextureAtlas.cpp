#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/DynamicTextureAtlas.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>

ezDynamicTextureAtlas::ezDynamicTextureAtlas() = default;
ezDynamicTextureAtlas::~ezDynamicTextureAtlas()
{
  Deinitialize();
}

ezResult ezDynamicTextureAtlas::Initialize(const ezGALTextureCreationDescription& textureDesc, ezUInt32 uiAlignment /*= 16*/)
{
  if (m_hTexture.IsInvalidated() == false)
  {
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(ezMath::IsPowerOf2(uiAlignment), "Alignment must be a power of 2.");
  EZ_ASSERT_DEV(textureDesc.m_uiWidth % uiAlignment == 0 && textureDesc.m_uiHeight % uiAlignment == 0, "Texture width and height must be a multiple of the alignment.");
  EZ_ASSERT_DEV(textureDesc.m_ResourceAccess.m_bImmutable == false, "Dynamic texture atlas must be mutable.");

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  m_hTexture = pDevice->CreateTexture(textureDesc);
  if (m_hTexture.IsInvalidated())
    return EZ_FAILURE;

  m_uiAlignment = uiAlignment;

  AddRootNode();

  return EZ_SUCCESS;
}

void ezDynamicTextureAtlas::Deinitialize()
{
  if (!m_hTexture.IsInvalidated())
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    pDevice->DestroyTexture(m_hTexture);
    m_hTexture.Invalidate();
  }

  m_uiAlignment = 0;

  ClearInternal();
}

ezDynamicTextureAtlas::AllocationId ezDynamicTextureAtlas::Allocate(ezUInt32 uiWidth, ezUInt32 uiHeight, ezStringView sName, ezRectU16* out_pRect /*= nullptr*/)
{
  if (uiWidth == 0 || uiHeight == 0)
    return AllocationId();

  uiWidth = ezMemoryUtils::AlignSize(uiWidth, m_uiAlignment);
  uiHeight = ezMemoryUtils::AlignSize(uiHeight, m_uiAlignment);

  AllocationId nodeId = FindSuitableNode(uiWidth, uiHeight);
  if (nodeId.IsInvalidated())
    return AllocationId();

  Node* pNode = &m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex);
  EZ_ASSERT_DEV(pNode->GetType() == NodeType::Free, "Implementation error");
  const ezRectU16 allocationRect = ezRectU16(pNode->m_Rect.x, pNode->m_Rect.y, uiWidth, uiHeight);
  const auto currentOrientation = pNode->GetOrientation();
  const auto flippedOrientation = FlipOrientation(currentOrientation);
  const ezUInt16 uiParentIndex = pNode->m_uiParentIndex;
  const ezUInt16 uiNextSiblingIndex = pNode->m_uiNextSiblingIndex;

  ezRectU16 splitRect, leftoverRect;
  const auto orientation = GuillotineRect(pNode->m_Rect, uiWidth, uiHeight, currentOrientation, splitRect, leftoverRect);

  pNode = nullptr;

  // Update the tree
  AllocationId allocationId;
  AllocationId splitId;
  AllocationId leftoverId;

  if (orientation == currentOrientation)
  {
    if (splitRect.HasNonZeroArea())
    {
      splitId = AllocateNode(NodeType::Free, currentOrientation, uiParentIndex, uiNextSiblingIndex, nodeId.m_InstanceIndex, splitRect);

      m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex).m_uiNextSiblingIndex = splitId.m_InstanceIndex;
      if (uiNextSiblingIndex != ezSmallInvalidIndex)
      {
        m_Nodes.GetValueUnchecked(uiNextSiblingIndex).m_uiPrevSiblingIndex = splitId.m_InstanceIndex;
      }
    }

    if (leftoverRect.HasNonZeroArea())
    {
      m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex).m_uiType = NodeType::Container;

      allocationId = AllocateNode(NodeType::Allocation, flippedOrientation, nodeId.m_InstanceIndex, ezSmallInvalidIndex, ezSmallInvalidIndex, allocationRect);
      leftoverId = AllocateNode(NodeType::Free, flippedOrientation, nodeId.m_InstanceIndex, ezSmallInvalidIndex, allocationId.m_InstanceIndex, leftoverRect);

      auto& allocationNode = m_Nodes.GetValueUnchecked(allocationId.m_InstanceIndex);
      allocationNode.m_uiNextSiblingIndex = leftoverId.m_InstanceIndex;
    }
    else
    {
      // No need to split for the leftover area, we can allocate directly in the chosen node
      allocationId = nodeId;

      auto& node = m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex);
      node.m_uiType = NodeType::Allocation;
      node.m_Rect = allocationRect;
    }
  }
  else
  {
    m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex).m_uiType = NodeType::Container;

    if (splitRect.HasNonZeroArea())
    {
      splitId = AllocateNode(NodeType::Free, flippedOrientation, nodeId.m_InstanceIndex, ezSmallInvalidIndex, ezSmallInvalidIndex, splitRect);
    }

    if (leftoverRect.HasNonZeroArea())
    {
      AllocationId containerId = AllocateNode(NodeType::Container, flippedOrientation, nodeId.m_InstanceIndex, splitId.m_InstanceIndex, ezSmallInvalidIndex, ezRectU16::MakeZero());

      auto& splitNode = m_Nodes.GetValueUnchecked(splitId.m_InstanceIndex);
      splitNode.m_uiPrevSiblingIndex = containerId.m_InstanceIndex;

      allocationId = AllocateNode(NodeType::Allocation, currentOrientation, containerId.m_InstanceIndex, ezSmallInvalidIndex, ezSmallInvalidIndex, allocationRect);
      leftoverId = AllocateNode(NodeType::Free, currentOrientation, containerId.m_InstanceIndex, ezSmallInvalidIndex, allocationId.m_InstanceIndex, leftoverRect);

      auto& allocationNode = m_Nodes.GetValueUnchecked(allocationId.m_InstanceIndex);
      allocationNode.m_uiNextSiblingIndex = leftoverId.m_InstanceIndex;
    }
    else
    {
      allocationId = AllocateNode(NodeType::Allocation, flippedOrientation, nodeId.m_InstanceIndex, splitId.m_InstanceIndex, ezSmallInvalidIndex, allocationRect);

      auto& splitNode = m_Nodes.GetValueUnchecked(splitId.m_InstanceIndex);
      splitNode.m_uiPrevSiblingIndex = allocationId.m_InstanceIndex;
    }
  }

  if (!splitId.IsInvalidated())
  {
    m_FreeList.PushBack(splitId);
  }

  if (!leftoverId.IsInvalidated())
  {
    m_FreeList.PushBack(leftoverId);
  }

  {
    auto& allocationNode = m_Nodes.GetValueUnchecked(allocationId.m_InstanceIndex);
    EZ_ASSERT_DEV(allocationNode.GetType() == NodeType::Allocation, "Implementation error");
    allocationNode.m_sName.Assign(sName);

    if (out_pRect != nullptr)
    {
      *out_pRect = allocationRect;
    }
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  CheckTree();
#endif

  return allocationId;
}

void ezDynamicTextureAtlas::Deallocate(AllocationId& ref_allocationId)
{
  Node* pNode = nullptr;
  if (!m_Nodes.TryGetValue(ref_allocationId, pNode))
  {
    ref_allocationId.Invalidate();
    return;
  }

  ezUInt16 uiNodeIndex = ref_allocationId.m_InstanceIndex;

  EZ_ASSERT_DEV(pNode->GetType() == NodeType::Allocation, "Implementation error");
  pNode->m_uiType = NodeType::Free;
  pNode->m_sName.Clear();

  while (true)
  {
    // Try to merge with the next node
    {
      const ezUInt16 uiNextIndex = pNode->m_uiNextSiblingIndex;
      if (uiNextIndex != ezSmallInvalidIndex && m_Nodes.GetValueUnchecked(uiNextIndex).GetType() == NodeType::Free)
      {
        MergeSiblingNodes(uiNodeIndex, uiNextIndex);
      }
    }

    {
      // Try to merge with the previous node
      const ezUInt16 uiPrevIndex = pNode->m_uiPrevSiblingIndex;
      if (uiPrevIndex != ezSmallInvalidIndex && m_Nodes.GetValueUnchecked(uiPrevIndex).GetType() == NodeType::Free)
      {
        MergeSiblingNodes(uiPrevIndex, uiNodeIndex);
        uiNodeIndex = uiPrevIndex;
        pNode = &m_Nodes.GetValueUnchecked(uiNodeIndex);
      }
    }

    // If this node is now the only child left we collapse it into its parent and try to merge again at the parent level
    const ezUInt16 uiParentIndex = pNode->m_uiParentIndex;
    if (uiParentIndex != ezSmallInvalidIndex &&
        pNode->m_uiNextSiblingIndex == ezSmallInvalidIndex &&
        pNode->m_uiPrevSiblingIndex == ezSmallInvalidIndex)
    {
      auto& parentNode = m_Nodes.GetValueUnchecked(uiParentIndex);
      EZ_ASSERT_DEV(parentNode.GetType() == NodeType::Container, "Implementation error");

      // Replace the parent container with a free node
      parentNode.m_uiType = NodeType::Free;
      parentNode.m_Rect = pNode->m_Rect;

      AllocationId nodeId = m_Nodes.GetIdUnchecked(uiNodeIndex);
      EZ_VERIFY(m_Nodes.Remove(nodeId), "Implementation error");

      // Start again at the parent level
      uiNodeIndex = uiParentIndex;
      pNode = &m_Nodes.GetValueUnchecked(uiNodeIndex);
    }
    else
    {
      // No more merging possible, we are done
      AllocationId nodeId = m_Nodes.GetIdUnchecked(uiNodeIndex);
      if (m_FreeList.Contains(nodeId) == false)
      {
        m_FreeList.PushBack(nodeId);
      }
      break;
    }
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  CheckTree();
#endif

  ref_allocationId.Invalidate();
}

void ezDynamicTextureAtlas::Clear()
{
  ClearInternal();
  AddRootNode();
}

ezRectU16 ezDynamicTextureAtlas::GetAllocationRect(AllocationId id) const
{
  Node* pNode = nullptr;
  if (m_Nodes.TryGetValue(id, pNode))
  {
    EZ_ASSERT_DEV(pNode->GetType() == NodeType::Allocation, "Implementation error");
    return pNode->m_Rect;
  }

  return ezRectU16();
}

void ezDynamicTextureAtlas::DebugDraw(const ezDebugRendererContext& debugContext, float fViewWidth, float fViewHeight, bool bBlackOutFreeAreas /*= true*/) const
{
  if (m_hTexture.IsInvalidated())
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  auto& textureDesc = pDevice->GetTexture(m_hTexture)->GetDescription();

  const float fOffset = 10.0f;
  const float fScale = ezMath::Min((fViewWidth * 0.75f) / textureDesc.m_uiWidth, (fViewHeight * 0.5f) / textureDesc.m_uiHeight);

  const ezRectFloat atlasRect = ezRectFloat(fOffset, fOffset, ezMath::Round(textureDesc.m_uiWidth * fScale), ezMath::Round(textureDesc.m_uiHeight * fScale));
  ezDebugRenderer::Draw2DRectangle(debugContext, atlasRect, 0.0f, ezColor::White, pDevice->GetDefaultResourceView(m_hTexture));

  double fUsedPixel = 0.0;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto& node = it.Value();
    if (node.GetType() == NodeType::Container)
      continue;

    const float fColorIndex = (it.Id().m_InstanceIndex & 0xF) / 16.0f;
    const ezColor color = node.GetType() == NodeType::Free ? ezColor(0.5f, 0.5f, 0.5f, 0.5f) : ezColorScheme::LightUI(fColorIndex);
    const ezRectFloat nodeRect = ezRectFloat(
      ezMath::Round(node.m_Rect.x * fScale + fOffset),
      ezMath::Round(node.m_Rect.y * fScale + fOffset),
      ezMath::Round(node.m_Rect.width * fScale) - 1.0f,
      ezMath::Round(node.m_Rect.height * fScale) - 1.0f);
    ezDebugRenderer::Draw2DLineRectangle(debugContext, nodeRect, 0.0f, color);

    if (bBlackOutFreeAreas && node.GetType() == NodeType::Free)
    {
      ezDebugRenderer::Draw2DRectangle(debugContext, nodeRect, 0.0f, ezColor::Black, ezResourceManager::LoadResource<ezTexture2DResource>("White.color"));
    }

    if (node.GetType() == NodeType::Allocation)
    {
      fUsedPixel += double(node.m_Rect.width) * node.m_Rect.height;
    }

    if (node.m_sName.IsEmpty() == false)
    {
      const ezVec2 nodeRectCenter = nodeRect.GetCenter();
      const ezVec2I32 nodeRectCenterI32 = ezVec2I32(int(ezMath::Round(nodeRectCenter.x)), int(ezMath::Round(nodeRectCenter.y)));

      const float fTextWidth = (node.m_sName.GetView().GetElementCount() + 3) * 8.0f * ezDebugRenderer::GetTextScale();
      const ezUInt32 uiTextSize = ezUInt32(ezMath::Round(ezMath::Clamp((nodeRect.width / fTextWidth) * 16.0f, 6.0f, 16.0f)));

      ezDebugRenderer::Draw2DText(debugContext, node.m_sName.GetView(), nodeRectCenterI32, color, uiTextSize, ezDebugTextHAlign::Center, ezDebugTextVAlign::Center);
    }
  }

  const double fUtilization = fUsedPixel / (double(textureDesc.m_uiWidth) * textureDesc.m_uiHeight) * 100.0;
  const ezVec2I32 pos = ezVec2I32(int(fOffset), int(atlasRect.GetY2() + 8));
  ezDebugRenderer::Draw2DText(debugContext, ezFmt("Atlas Size: {}x{} | Utilization: {}%%", textureDesc.m_uiWidth, textureDesc.m_uiHeight, ezArgF(fUtilization, 2)), pos, ezColor::White);
}

ezDynamicTextureAtlas::AllocationId ezDynamicTextureAtlas::AllocateNode(NodeType::Enum type, Orientation::Enum orientation, ezUInt16 uiParentIndex, ezUInt16 uiNextSiblingIndex, ezUInt16 uiPrevSiblingIndex, const ezRectU16& rect)
{
  static_assert(NodeType::Count <= EZ_BIT(2));
  static_assert(Orientation::Count <= EZ_BIT(1));

  Node node;
  node.m_uiType = type;
  node.m_uiOrientation = orientation;
  node.m_uiChannelMask = 0;
  node.m_uiParentIndex = uiParentIndex;
  node.m_uiNextSiblingIndex = uiNextSiblingIndex;
  node.m_uiPrevSiblingIndex = uiPrevSiblingIndex;
  node.m_Rect = rect;

  return m_Nodes.Insert(node);
}

ezDynamicTextureAtlas::AllocationId ezDynamicTextureAtlas::FindSuitableNode(ezUInt32 uiWidth, ezUInt32 uiHeight)
{
  ezUInt32 uiBestScore = ezInvalidIndex;
  AllocationId bestNodeId;
  ezUInt32 uiBestFreeListIndex = ezInvalidIndex;

  ezUInt32 i = 0;
  while (i < m_FreeList.GetCount())
  {
    const AllocationId freeId = m_FreeList[i];
    Node* pNode = nullptr;
    if (!m_Nodes.TryGetValue(freeId, pNode))
    {
      // Node was removed in the meantime, remove it from the freelist as well
      m_FreeList.RemoveAtAndSwap(i);
      continue;
    }

    if (pNode->m_Rect.width >= uiWidth && pNode->m_Rect.height >= uiHeight)
    {
      ezUInt32 dx = pNode->m_Rect.width - uiWidth;
      ezUInt32 dy = pNode->m_Rect.height - uiHeight;
      if (dx == 0 && dy == 0)
      {
        // perfect fit
        bestNodeId = freeId;
        uiBestFreeListIndex = i;
        break;
      }

      ezUInt32 uiScore = ezMath::Min(dx, dy);
      if (uiScore < uiBestScore)
      {
        uiBestScore = uiScore;
        bestNodeId = freeId;
        uiBestFreeListIndex = i;
      }
    }

    ++i;
  }

  if (!bestNodeId.IsInvalidated())
  {
    m_FreeList.RemoveAtAndSwap(uiBestFreeListIndex);
    return bestNodeId;
  }

  return AllocationId();
}

void ezDynamicTextureAtlas::MergeSiblingNodes(ezUInt16 uiNodeIndex, ezUInt16 uiNextSiblingIndex)
{
  auto& node = m_Nodes.GetValueUnchecked(uiNodeIndex);
  auto& next = m_Nodes.GetValueUnchecked(uiNextSiblingIndex);
  EZ_ASSERT_DEV(node.GetType() == NodeType::Free && next.GetType() == NodeType::Free, "Implementation error");

  // Merge the two nodes into one
  node.m_Rect = ezRectU16::MakeUnion(node.m_Rect, next.m_Rect);

  ezUInt16 uiNextNextIndex = next.m_uiNextSiblingIndex;
  node.m_uiNextSiblingIndex = uiNextNextIndex;
  if (uiNextNextIndex != ezSmallInvalidIndex)
  {
    m_Nodes.GetValueUnchecked(uiNextNextIndex).m_uiPrevSiblingIndex = uiNodeIndex;
  }

  // Remove the next node
  AllocationId nextId = m_Nodes.GetIdUnchecked(uiNextSiblingIndex);
  EZ_VERIFY(m_Nodes.Remove(nextId), "Implementation error");
}

ezDynamicTextureAtlas::Orientation::Enum ezDynamicTextureAtlas::GuillotineRect(const ezRectU16& nodeRect, ezUInt32 uiAllocatedWidth, ezUInt32 uiAllocatedHeight, Orientation::Enum defaultOrientation, ezRectU16& out_splitRect, ezRectU16& out_leftoverRect)
{
  if (nodeRect.width == uiAllocatedWidth && nodeRect.height == uiAllocatedHeight)
  {
    out_splitRect = ezRectU16::MakeZero();
    out_leftoverRect = ezRectU16::MakeZero();
    return defaultOrientation;
  }

  // +-----------+-------------+
  // |///////////|             |
  // |/allocated/|             |
  // |///////////|    split    |
  // +-----------+             |
  // |           |             |
  // | leftover  |             |
  // |           |             |
  // +-----------+-------------+
  const ezRectU16 candidateLeftoverRectToBottom = ezRectU16(nodeRect.x, nodeRect.y + uiAllocatedHeight, uiAllocatedWidth, nodeRect.height - uiAllocatedHeight);
  const ezUInt32 uiLeftoverAreaToBottom = ezUInt32(candidateLeftoverRectToBottom.width) * candidateLeftoverRectToBottom.height;

  // +-----------+-------------+
  // |///////////|             |
  // |/allocated/|  leftover   |
  // |///////////|             |
  // +-----------+-------------+
  // |                         |
  // |          split          |
  // |                         |
  // +-------------------------+
  const ezRectU16 candidateLeftoverRectToRight = ezRectU16(nodeRect.x + uiAllocatedWidth, nodeRect.y, nodeRect.width - uiAllocatedWidth, uiAllocatedHeight);
  const ezUInt32 uiLeftoverAreaToRight = ezUInt32(candidateLeftoverRectToRight.width) * candidateLeftoverRectToRight.height;

  if (uiLeftoverAreaToRight > uiLeftoverAreaToBottom)
  {
    out_splitRect = ezRectU16(nodeRect.x + uiAllocatedWidth, nodeRect.y, nodeRect.width - uiAllocatedWidth, nodeRect.height);
    out_leftoverRect = candidateLeftoverRectToBottom;
    return Orientation::Horizontal;
  }
  else
  {
    out_splitRect = ezRectU16(nodeRect.x, nodeRect.y + uiAllocatedHeight, nodeRect.width, nodeRect.height - uiAllocatedHeight);
    out_leftoverRect = candidateLeftoverRectToRight;
    return Orientation::Vertical;
  }
}

void ezDynamicTextureAtlas::ClearInternal()
{
  m_Nodes.Clear();
  m_FreeList.Clear();
}

void ezDynamicTextureAtlas::AddRootNode()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  auto& textureDesc = pDevice->GetTexture(m_hTexture)->GetDescription();

  AllocationId rootId = AllocateNode(NodeType::Free, Orientation::Vertical, ezSmallInvalidIndex, ezSmallInvalidIndex, ezSmallInvalidIndex, ezRectU16(0, 0, textureDesc.m_uiWidth, textureDesc.m_uiHeight));
  m_FreeList.PushBack(rootId);
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
void ezDynamicTextureAtlas::CheckTree() const
{
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto& node = it.Value();

    ezUInt16 uiNextSiblingIndex = node.m_uiNextSiblingIndex;
    while (uiNextSiblingIndex != ezSmallInvalidIndex)
    {
      auto& siblingNode = m_Nodes.GetValueUnchecked(uiNextSiblingIndex);
      EZ_ASSERT_DEBUG(siblingNode.m_uiOrientation == node.m_uiOrientation, "Implementation error");
      EZ_ASSERT_DEBUG(siblingNode.m_uiParentIndex == node.m_uiParentIndex, "Implementation error");

      CheckNode(uiNextSiblingIndex, node.GetOrientation());

      uiNextSiblingIndex = siblingNode.m_uiNextSiblingIndex;
    }

    if (node.m_uiParentIndex != ezSmallInvalidIndex)
    {
      auto& parentNode = m_Nodes.GetValueUnchecked(node.m_uiParentIndex);
      EZ_ASSERT_DEBUG(parentNode.GetType() == NodeType::Container, "Implementation error");
      EZ_ASSERT_DEBUG(parentNode.GetOrientation() == FlipOrientation(node.GetOrientation()), "Implementation error");
    }
  }
}

void ezDynamicTextureAtlas::CheckNode(ezUInt16 uiNodexIndex, Orientation::Enum parentOrientation) const
{
  auto& node = m_Nodes.GetValueUnchecked(uiNodexIndex);
  if (node.m_uiNextSiblingIndex == ezSmallInvalidIndex)
    return;

  auto& nextNode = m_Nodes.GetValueUnchecked(node.m_uiNextSiblingIndex);
  EZ_ASSERT_DEBUG(nextNode.m_uiPrevSiblingIndex == uiNodexIndex, "Implementation error");

  if (node.GetType() == NodeType::Container)
    return;

  auto& r1 = node.m_Rect;
  auto& r2 = nextNode.m_Rect;
  if (parentOrientation == Orientation::Horizontal)
  {
    EZ_ASSERT_DEBUG(r1.y == r2.y, "Implementation error");
    EZ_ASSERT_DEBUG(r1.GetY2() == r2.GetY2(), "Implementation error");
  }
  else
  {
    EZ_ASSERT_DEBUG(r1.x == r2.x, "Implementation error");
    EZ_ASSERT_DEBUG(r1.GetX2() == r2.GetX2(), "Implementation error");
  }
}
#endif
