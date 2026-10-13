#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Memory/FrameAllocator.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/DynamicBuffer.h>

namespace
{
  // Binning similar to https://github.com/sebbbi/OffsetAllocator
  constexpr ezUInt32 s_uiMantissaBits = 3;
  constexpr ezUInt32 s_uiMantissaValue = 1u << s_uiMantissaBits; // 8
  constexpr ezUInt32 s_uiMantissaMask = s_uiMantissaValue - 1;

  /// Bin for inserting a free node of this size (round down).
  ezUInt32 BinFloor(ezUInt32 uiSize)
  {
    if (uiSize < s_uiMantissaValue)
      return uiSize; // sizes 0..7 map 1:1 (like denormals)

    const ezUInt32 uiShift = ezMath::FirstBitHigh(uiSize) - s_uiMantissaBits;
    const ezUInt32 uiMantissa = (uiSize >> uiShift) & s_uiMantissaMask;
    return ((uiShift + 1) << s_uiMantissaBits) | uiMantissa;
  }

  /// Smallest bin where every node is guaranteed to fit this size (round up).
  ezUInt32 BinCeil(ezUInt32 uiSize)
  {
    if (uiSize < s_uiMantissaValue)
      return uiSize;

    const ezUInt32 uiShift = ezMath::FirstBitHigh(uiSize) - s_uiMantissaBits;
    const ezUInt32 uiBin = BinFloor(uiSize);
    const bool bHasLowBits = (uiSize & ((1u << uiShift) - 1)) != 0;
    return uiBin + (bHasLowBits ? 1 : 0); // a mantissa overflow moves into the next exponent on its own
  }
} // namespace

////////////////////////////////////////////////////////////////////////

ezGALDynamicBuffer::~ezGALDynamicBuffer()
{
  Deinitialize();
}

void ezGALDynamicBuffer::Initialize(const ezGALBufferCreationDescription& desc, ezStringView sDebugName)
{
  EZ_ASSERT_DEV(desc.m_uiStructSize > 0, "Struct size must be greater than 0");

  m_Desc = desc;
  m_uiCapacity = desc.m_uiTotalSize / desc.m_uiStructSize;
  EZ_ASSERT_DEV(m_uiCapacity <= s_uiMaxCount, "Dynamic buffer '{}': Initial size of {} elements is too large, the maximum is {}", sDebugName, m_uiCapacity, s_uiMaxCount);
  m_sDebugName = sDebugName;

  // Sets up m_Data and one free node for the full capacity
  Clear();
}

void ezGALDynamicBuffer::Deinitialize()
{
  // Only called from the destructor, which frees the containers. Clear() would allocate m_Data again.
  if (m_hBufferForRendering != m_hBufferForUpload)
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBufferForRendering);
  }

  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hBufferForUpload);
}

void ezGALDynamicBuffer::Clear()
{
  m_Data.SetCountUninitialized(m_Desc.m_uiTotalSize);

  // Temp memory comes from the frame allocator, so it doesn't need to be freed
  m_bHasTempData = false;

  m_Nodes.Clear();
  m_uiFirstReleasedNodeIndex = ezInvalidIndex;
  m_OffsetToNodeIndex.Clear();

  for (ezUInt32& uiBinHead : m_FreeNodeBins)
  {
    uiBinHead = ezInvalidIndex;
  }

  m_uiFirstNodeIndex = ezInvalidIndex;
  m_uiLastNodeIndex = ezInvalidIndex;
  m_DirtyRange.Reset();

  if (m_uiCapacity > 0)
  {
    const ezUInt32 uiNodeIndex = AllocateNode();
    m_Nodes[uiNodeIndex].m_uiCount = m_uiCapacity;
    m_uiFirstNodeIndex = uiNodeIndex;
    m_uiLastNodeIndex = uiNodeIndex;
    AddToBin(uiNodeIndex);
  }
}

ezUInt32 ezGALDynamicBuffer::Allocate(ezUInt64 uiUserData, ezUInt32 uiCount, ezBitflags<AllocateFlags> allocateFlags)
{
  EZ_LOCK(m_Mutex);
  EZ_ASSERT_DEV(uiCount > 0, "Allocation count must be greater than 0");
  EZ_ASSERT_DEV(uiCount <= s_uiMaxCount, "Allocation count {} is too large, the maximum is {}", uiCount, s_uiMaxCount);

  ezUInt32 uiNodeIndex = FindFreeNode(uiCount);
  if (uiNodeIndex == ezInvalidIndex)
  {
    // FindFreeNode skips the bin of uiCount, so the last node may still fit. Otherwise the buffer grows.
    EnsureFreeNodeAtEnd(uiCount);
    uiNodeIndex = m_uiLastNodeIndex;
  }

  RemoveFromBin(uiNodeIndex);
  SplitNode(uiNodeIndex, uiCount);

  Node& node = m_Nodes[uiNodeIndex];
  const ezUInt32 uiOffset = node.m_uiOffset;

  // Don't resize m_Data here. Other threads may still have mapped pointers into it.
  // If the allocation doesn't fit into m_Data, because the buffer has grown, it gets its own temp memory until the next upload.
  ezUInt8* pTempData = nullptr;
  if ((uiOffset + uiCount) * m_Desc.m_uiStructSize > m_Data.GetCount())
  {
    pTempData = AllocateTempData(uiCount);
  }

  node.m_uiUserData = uiUserData;
  node.m_pTempData = pTempData;
  node.m_uiUsed = true;

  m_OffsetToNodeIndex.Insert(uiOffset, uiNodeIndex);

  if (allocateFlags.IsSet(AllocateFlags::ZeroFill))
  {
    ezUInt32 uiDummyCount = 0;
    auto data = MapForWriting(uiOffset, uiDummyCount);
    ezMemoryUtils::ZeroFill(data.GetPtr(), data.GetCount());
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  CheckSelf();
#endif

  return uiOffset;
}

void ezGALDynamicBuffer::Deallocate(ezUInt32 uiOffset)
{
  EZ_LOCK(m_Mutex);

  ezUInt32 uiNodeIndex = ezInvalidIndex;
  EZ_VERIFY(m_OffsetToNodeIndex.Remove(uiOffset, &uiNodeIndex), "Invalid offset");

  FreeNodeAndMerge(uiNodeIndex);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  CheckSelf();
#endif
}

ezByteArrayPtr ezGALDynamicBuffer::MapForWriting(ezUInt32 uiOffset, ezUInt32& out_uiCount)
{
  EZ_LOCK(m_Mutex);

  ezUInt32 uiNodeIndex = 0;
  EZ_VERIFY(m_OffsetToNodeIndex.TryGetValue(uiOffset, uiNodeIndex), "Invalid offset");

  auto& node = m_Nodes[uiNodeIndex];
  out_uiCount = node.m_uiCount;

  // Mark dirty
  m_DirtyRange.SetToIncludeRange(uiOffset, uiOffset + out_uiCount - 1);

  const ezUInt32 uiByteOffset = uiOffset * m_Desc.m_uiStructSize;
  const ezUInt32 uiByteSize = out_uiCount * m_Desc.m_uiStructSize;

  if (node.m_pTempData != nullptr)
    return ezByteArrayPtr(node.m_pTempData, uiByteSize);

  return m_Data.GetByteArrayPtr().GetSubArray(uiByteOffset, uiByteSize);
}

ezConstByteArrayPtr ezGALDynamicBuffer::MapForReading(ezUInt32 uiOffset, ezUInt32& out_uiCount) const
{
  EZ_LOCK(m_Mutex);

  ezUInt32 uiNodeIndex = 0;
  EZ_VERIFY(m_OffsetToNodeIndex.TryGetValue(uiOffset, uiNodeIndex), "Invalid offset");

  auto& node = m_Nodes[uiNodeIndex];
  out_uiCount = node.m_uiCount;

  const ezUInt32 uiByteOffset = uiOffset * m_Desc.m_uiStructSize;
  const ezUInt32 uiByteSize = out_uiCount * m_Desc.m_uiStructSize;

  if (node.m_pTempData != nullptr)
    return ezConstByteArrayPtr(node.m_pTempData, uiByteSize);

  return m_Data.GetByteArrayPtr().GetSubArray(uiByteOffset, uiByteSize);
}

ezUInt8* ezGALDynamicBuffer::AllocateTempData(ezUInt32 uiCount)
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  // Remember the oldest temp data, UploadChangesForNextFrame checks that it is still valid
  if (!m_bHasTempData)
  {
    m_uiTempDataSwapCount = ezFrameAllocator::GetSwapCount();
  }
#endif

  m_bHasTempData = true;

  return static_cast<ezUInt8*>(ezFrameAllocator::GetCurrentAllocator()->Allocate(uiCount * m_Desc.m_uiStructSize, 16));
}

void ezGALDynamicBuffer::EnsureFreeNodeAtEnd(ezUInt32 uiCount)
{
  const bool bLastNodeIsFree = m_uiLastNodeIndex != ezInvalidIndex && !m_Nodes[m_uiLastNodeIndex].m_uiUsed;
  const ezUInt32 uiFreeOffset = bLastNodeIsFree ? m_Nodes[m_uiLastNodeIndex].m_uiOffset : m_uiCapacity;
  const ezUInt64 uiMinCapacity = ezUInt64(uiFreeOffset) + uiCount; // 64 bit to detect an overflow below

  if (uiMinCapacity <= m_uiCapacity)
    return;

  constexpr ezUInt64 uiExpGrowthLimit = 16 * 1024 * 1024;

  // m_uiTotalSize is 32 bit and in bytes, and a free node can cover the whole capacity
  const ezUInt64 uiMaxCapacity = ezMath::Min<ezUInt64>(ezMath::MaxValue<ezUInt32>() / m_Desc.m_uiStructSize, s_uiMaxCount);
  EZ_ASSERT_DEV(uiMinCapacity <= uiMaxCapacity, "Dynamic buffer '{}': Can't grow to {} elements, the maximum for a struct size of {} bytes is {}", m_sDebugName, uiMinCapacity, m_Desc.m_uiStructSize, uiMaxCapacity);

  ezUInt64 uiNewCapacity64 = ezMath::Max<ezUInt64>(uiMinCapacity, 256);
  if (uiNewCapacity64 < uiExpGrowthLimit)
  {
    uiNewCapacity64 = ezMath::PowerOfTwo_Ceil(uiNewCapacity64);
  }
  else
  {
    uiNewCapacity64 = ezMemoryUtils::AlignSize(uiNewCapacity64, uiExpGrowthLimit);
  }

  // Rounding up may go over the limit, even if the required capacity is below it
  const ezUInt32 uiNewCapacity = static_cast<ezUInt32>(ezMath::Min(uiNewCapacity64, uiMaxCapacity));
  const ezUInt32 uiOldCapacity = m_uiCapacity;

  // UploadChangesForNextFrame changes the size of m_Data and the GPU buffer to this
  m_Desc.m_uiTotalSize = uiNewCapacity * m_Desc.m_uiStructSize;
  m_uiCapacity = uiNewCapacity;

  // The GPU buffer is created again with the new size, so all data has to be uploaded
  m_DirtyRange.SetToIncludeRange(0, m_uiCapacity - 1);

  // Add a node for the new range at the end. FreeNodeAndMerge joins it with the last node, if that is free.
  const ezUInt32 uiNewNodeIndex = AllocateNode();
  Node& newNode = m_Nodes[uiNewNodeIndex];
  newNode.m_uiOffset = uiOldCapacity;
  newNode.m_uiCount = uiNewCapacity - uiOldCapacity;
  newNode.m_uiPrevNodeIndex = m_uiLastNodeIndex;

  if (m_uiLastNodeIndex != ezInvalidIndex)
  {
    m_Nodes[m_uiLastNodeIndex].m_uiNextNodeIndex = uiNewNodeIndex;
  }
  else
  {
    m_uiFirstNodeIndex = uiNewNodeIndex;
  }

  m_uiLastNodeIndex = uiNewNodeIndex;
  FreeNodeAndMerge(uiNewNodeIndex);
}

ezUInt32 ezGALDynamicBuffer::AllocateNode()
{
  if (m_uiFirstReleasedNodeIndex != ezInvalidIndex)
  {
    const ezUInt32 uiNodeIndex = m_uiFirstReleasedNodeIndex;
    m_uiFirstReleasedNodeIndex = m_Nodes[uiNodeIndex].m_uiNextNodeIndex;
    m_Nodes[uiNodeIndex] = Node();
    return uiNodeIndex;
  }

  m_Nodes.ExpandAndGetRef();
  return m_Nodes.GetCount() - 1;
}

void ezGALDynamicBuffer::ReleaseNode(ezUInt32 uiNodeIndex)
{
  m_Nodes[uiNodeIndex] = Node();
  m_Nodes[uiNodeIndex].m_uiNextNodeIndex = m_uiFirstReleasedNodeIndex;
  m_uiFirstReleasedNodeIndex = uiNodeIndex;
}

void ezGALDynamicBuffer::AddToBin(ezUInt32 uiNodeIndex)
{
  Node& node = m_Nodes[uiNodeIndex];
  EZ_ASSERT_DEBUG(!node.m_uiUsed && node.m_uiCount > 0, "Implementation error");

  const ezUInt32 uiBin = BinFloor(node.m_uiCount);
  const ezUInt32 uiHeadNodeIndex = m_FreeNodeBins[uiBin];

  node.m_uiPrevNodeIndexInBin = ezInvalidIndex;
  node.m_uiNextNodeIndexInBin = uiHeadNodeIndex;

  if (uiHeadNodeIndex != ezInvalidIndex)
  {
    m_Nodes[uiHeadNodeIndex].m_uiPrevNodeIndexInBin = uiNodeIndex;
  }

  m_FreeNodeBins[uiBin] = uiNodeIndex;
}

void ezGALDynamicBuffer::RemoveFromBin(ezUInt32 uiNodeIndex)
{
  Node& node = m_Nodes[uiNodeIndex];
  EZ_ASSERT_DEBUG(!node.m_uiUsed, "Implementation error");

  const ezUInt32 uiBin = BinFloor(node.m_uiCount);

  if (node.m_uiPrevNodeIndexInBin != ezInvalidIndex)
  {
    m_Nodes[node.m_uiPrevNodeIndexInBin].m_uiNextNodeIndexInBin = node.m_uiNextNodeIndexInBin;
  }
  else
  {
    EZ_ASSERT_DEBUG(m_FreeNodeBins[uiBin] == uiNodeIndex, "Node is not in the bin for its size");
    m_FreeNodeBins[uiBin] = node.m_uiNextNodeIndexInBin;
  }

  if (node.m_uiNextNodeIndexInBin != ezInvalidIndex)
  {
    m_Nodes[node.m_uiNextNodeIndexInBin].m_uiPrevNodeIndexInBin = node.m_uiPrevNodeIndexInBin;
  }

  node.m_uiPrevNodeIndexInBin = ezInvalidIndex;
  node.m_uiNextNodeIndexInBin = ezInvalidIndex;
}

void ezGALDynamicBuffer::SplitNode(ezUInt32 uiNodeIndex, ezUInt32 uiCount)
{
  EZ_ASSERT_DEBUG(m_Nodes[uiNodeIndex].m_uiCount >= uiCount, "Node is too small");
  if (m_Nodes[uiNodeIndex].m_uiCount == uiCount)
    return;

  const ezUInt32 uiRestNodeIndex = AllocateNode(); // can reallocate m_Nodes, so get the references afterwards
  Node& node = m_Nodes[uiNodeIndex];
  Node& restNode = m_Nodes[uiRestNodeIndex];

  restNode.m_uiOffset = node.m_uiOffset + uiCount;
  restNode.m_uiCount = node.m_uiCount - uiCount;
  restNode.m_uiPrevNodeIndex = uiNodeIndex;
  restNode.m_uiNextNodeIndex = node.m_uiNextNodeIndex;

  if (node.m_uiNextNodeIndex != ezInvalidIndex)
  {
    m_Nodes[node.m_uiNextNodeIndex].m_uiPrevNodeIndex = uiRestNodeIndex;
  }
  else
  {
    m_uiLastNodeIndex = uiRestNodeIndex;
  }

  node.m_uiNextNodeIndex = uiRestNodeIndex;
  node.m_uiCount = uiCount;

  AddToBin(uiRestNodeIndex);
}

void ezGALDynamicBuffer::FreeNodeAndMerge(ezUInt32 uiNodeIndex)
{
  // No nodes are allocated below, so this reference stays valid
  Node& node = m_Nodes[uiNodeIndex];
  node.m_uiUsed = false;

  // Merge with free neighbors, so that two free nodes are never next to each other
  const ezUInt32 uiPrevNodeIndex = node.m_uiPrevNodeIndex;
  if (uiPrevNodeIndex != ezInvalidIndex && !m_Nodes[uiPrevNodeIndex].m_uiUsed)
  {
    RemoveFromBin(uiPrevNodeIndex);

    const Node& prevNode = m_Nodes[uiPrevNodeIndex];
    node.m_uiOffset = prevNode.m_uiOffset;
    node.m_uiCount += prevNode.m_uiCount;
    node.m_uiPrevNodeIndex = prevNode.m_uiPrevNodeIndex;

    if (node.m_uiPrevNodeIndex != ezInvalidIndex)
    {
      m_Nodes[node.m_uiPrevNodeIndex].m_uiNextNodeIndex = uiNodeIndex;
    }
    else
    {
      m_uiFirstNodeIndex = uiNodeIndex;
    }

    ReleaseNode(uiPrevNodeIndex);
  }

  const ezUInt32 uiNextNodeIndex = node.m_uiNextNodeIndex;
  if (uiNextNodeIndex != ezInvalidIndex && !m_Nodes[uiNextNodeIndex].m_uiUsed)
  {
    RemoveFromBin(uiNextNodeIndex);

    const Node& nextNode = m_Nodes[uiNextNodeIndex];
    node.m_uiCount += nextNode.m_uiCount;
    node.m_uiNextNodeIndex = nextNode.m_uiNextNodeIndex;

    if (node.m_uiNextNodeIndex != ezInvalidIndex)
    {
      m_Nodes[node.m_uiNextNodeIndex].m_uiPrevNodeIndex = uiNodeIndex;
    }
    else
    {
      m_uiLastNodeIndex = uiNodeIndex;
    }

    ReleaseNode(uiNextNodeIndex);
  }

  AddToBin(uiNodeIndex);
}

ezUInt32 ezGALDynamicBuffer::FindFreeNode(ezUInt32 uiCount) const
{
  // Every node in this bin or a higher one is large enough, so the first node of the first non-empty bin can be used
  for (ezUInt32 uiBin = BinCeil(uiCount); uiBin < s_uiNumBins; ++uiBin)
  {
    if (m_FreeNodeBins[uiBin] != ezInvalidIndex)
      return m_FreeNodeBins[uiBin];
  }

  return ezInvalidIndex;
}

void ezGALDynamicBuffer::UploadChangesForNextFrame()
{
  EZ_LOCK(m_Mutex);

  if (m_DirtyRange.IsValid() == false)
    return;

  // Build the final data buffer. After this, all pointers from earlier Map calls are invalid.
  m_Data.SetCountUninitialized(m_Desc.m_uiTotalSize);
  if (m_bHasTempData)
  {
    EZ_ASSERT_DEV(ezFrameAllocator::GetSwapCount() - m_uiTempDataSwapCount <= 1,
      "Dynamic buffer '{}': Temp data is older than the previous frame and the frame allocator has already reset it. UploadChangesForNextFrame must be called at least once per frame after allocations.",
      m_sDebugName);

    for (auto& node : m_Nodes)
    {
      // Only used nodes have temp data. For free and released nodes, the pointer shares memory with other data.
      if (!node.m_uiUsed || node.m_pTempData == nullptr)
        continue;

      const ezUInt32 uiByteOffset = node.m_uiOffset * m_Desc.m_uiStructSize;
      const ezUInt32 uiByteSize = node.m_uiCount * m_Desc.m_uiStructSize;
      ezMemoryUtils::Copy(&m_Data[uiByteOffset], node.m_pTempData, uiByteSize);

      node.m_pTempData = nullptr;
    }

    // The frame allocator frees the temp memory itself
    m_bHasTempData = false;
  }

  auto pDevice = ezGALDevice::GetDefaultDevice();

  if (m_hBufferForUpload.IsInvalidated() == false && pDevice->GetBuffer(m_hBufferForUpload)->GetDescription().m_uiTotalSize != m_Desc.m_uiTotalSize)
  {
    pDevice->DestroyBuffer(m_hBufferForUpload);
    m_hBufferForUpload.Invalidate();
  }

  if (m_hBufferForUpload.IsInvalidated())
  {
    m_hBufferForUpload = pDevice->CreateBuffer(m_Desc, m_Data);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    pDevice->GetBuffer(m_hBufferForUpload)->SetDebugName(m_sDebugName);
#endif
  }
  else
  {
    const ezUInt32 uiByteOffset = m_DirtyRange.m_uiMin * m_Desc.m_uiStructSize;
    const ezUInt32 uiByteSize = m_DirtyRange.GetCount() * m_Desc.m_uiStructSize;
    auto data = m_Data.GetArrayPtr().GetSubArray(uiByteOffset, uiByteSize);

    pDevice->UpdateBufferForNextFrame(m_hBufferForUpload, data, uiByteOffset);
  }

  m_DirtyRange.Reset();
}

void ezGALDynamicBuffer::RunCompactionSteps(ezDynamicArray<ChangedAllocation>& out_changedAllocations, ezUInt32 uiMaxSteps)
{
  EZ_LOCK(m_Mutex);

  out_changedAllocations.Clear();

  // Don't compact while there is temp data. MoveData only works with m_Data.
  if (m_bHasTempData)
    return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_SCOPE_EXIT(CheckSelf());
#endif

  auto MoveData = [&](ezUInt64 uiUserData, ezUInt32 uiOldOffset, ezUInt32 uiNewOffset, ezUInt32 uiCount)
  {
    out_changedAllocations.PushBack(ChangedAllocation{uiUserData, uiNewOffset});

    const ezUInt32 uiOldByteOffset = uiOldOffset * m_Desc.m_uiStructSize;
    const ezUInt32 uiNewByteOffset = uiNewOffset * m_Desc.m_uiStructSize;
    const ezUInt32 uiByteSize = uiCount * m_Desc.m_uiStructSize;
    // moving an allocation forward by less than its own size overlaps
    ezMemoryUtils::CopyOverlapped(&m_Data[uiNewByteOffset], &m_Data[uiOldByteOffset], uiByteSize);

    m_DirtyRange.SetToIncludeRange(uiNewOffset, uiNewOffset + uiCount - 1);

    m_OffsetToNodeIndex.Remove(uiOldOffset);
  };

  // Each step works on the hole with the lowest offset. Holes only move back, so the search continues where the last step ended.
  ezUInt32 uiHoleIndex = m_uiFirstNodeIndex;

  for (ezUInt32 i = 0; i < uiMaxSteps; ++i)
  {
    // A hole is a free node that is not at the end of the buffer. Free nodes are never next to each other, so a used node follows it.
    while (uiHoleIndex != ezInvalidIndex && (m_Nodes[uiHoleIndex].m_uiUsed || uiHoleIndex == m_uiLastNodeIndex))
    {
      uiHoleIndex = m_Nodes[uiHoleIndex].m_uiNextNodeIndex;
    }

    if (uiHoleIndex == ezInvalidIndex)
      return;

    Node& hole = m_Nodes[uiHoleIndex];
    const ezUInt32 uiHoleOffset = hole.m_uiOffset;
    const ezUInt32 uiHoleCount = hole.m_uiCount;

    const ezUInt32 uiLastUsedIndex = m_Nodes[m_uiLastNodeIndex].m_uiUsed ? m_uiLastNodeIndex : m_Nodes[m_uiLastNodeIndex].m_uiPrevNodeIndex;
    Node& lastUsed = m_Nodes[uiLastUsedIndex];

    RemoveFromBin(uiHoleIndex);

    if (lastUsed.m_uiCount == uiHoleCount)
    {
      // The last allocation has exactly the size of the hole. One move fills the hole, and the free space at the end grows.
      MoveData(lastUsed.m_uiUserData, lastUsed.m_uiOffset, uiHoleOffset, uiHoleCount);

      hole.m_uiUserData = lastUsed.m_uiUserData;
      hole.m_pTempData = nullptr;
      hole.m_uiUsed = true;

      FreeNodeAndMerge(uiLastUsedIndex);
    }
    else
    {
      // Otherwise move the allocation that comes directly after the hole to the start of the hole.
      // The two nodes swap their roles, so the order of the list stays the same.
      // The hole then moves back by the size of that allocation, and it may join the next free node.
      const ezUInt32 uiNextIndex = hole.m_uiNextNodeIndex;
      Node& next = m_Nodes[uiNextIndex];
      EZ_ASSERT_DEBUG(next.m_uiUsed, "Implementation error");

      MoveData(next.m_uiUserData, next.m_uiOffset, uiHoleOffset, next.m_uiCount);

      hole.m_uiUserData = next.m_uiUserData;
      hole.m_uiCount = next.m_uiCount;
      hole.m_pTempData = nullptr;
      hole.m_uiUsed = true;

      next.m_uiOffset = uiHoleOffset + hole.m_uiCount;
      next.m_uiCount = uiHoleCount;

      FreeNodeAndMerge(uiNextIndex);
    }

    m_OffsetToNodeIndex.Insert(uiHoleOffset, uiHoleIndex);
  }
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
void ezGALDynamicBuffer::CheckSelf() const
{
  // Disabled by default, because it checks all nodes on every call. Enable it when you change this class.
#  if 0
  ezUInt32 uiPrevNodeIndex = ezInvalidIndex;
  ezUInt32 uiExpectedOffset = 0;
  ezUInt32 uiNumUsedNodes = 0;
  ezUInt32 uiNumFreeNodes = 0;

  for (ezUInt32 uiNodeIndex = m_uiFirstNodeIndex; uiNodeIndex != ezInvalidIndex; uiNodeIndex = m_Nodes[uiNodeIndex].m_uiNextNodeIndex)
  {
    const Node& node = m_Nodes[uiNodeIndex];
    EZ_ASSERT_DEBUG(node.m_uiPrevNodeIndex == uiPrevNodeIndex, "Broken node list");
    EZ_ASSERT_DEBUG(node.m_uiOffset == uiExpectedOffset, "Nodes are not contiguous");
    EZ_ASSERT_DEBUG(node.m_uiCount > 0, "Empty node");

    if (node.m_uiUsed)
    {
      ++uiNumUsedNodes;

      ezUInt32 uiMappedNodeIndex = ezInvalidIndex;
      EZ_ASSERT_DEBUG(m_OffsetToNodeIndex.TryGetValue(node.m_uiOffset, uiMappedNodeIndex) && uiMappedNodeIndex == uiNodeIndex, "Allocation is not in the offset map");

      const ezUInt32 uiByteEndOffset = (node.m_uiOffset + node.m_uiCount) * m_Desc.m_uiStructSize;
      EZ_ASSERT_DEBUG(node.m_pTempData != nullptr || uiByteEndOffset <= m_Data.GetCount(), "Allocation is outside of the regular data");
    }
    else
    {
      ++uiNumFreeNodes;
      EZ_ASSERT_DEBUG(uiPrevNodeIndex == ezInvalidIndex || m_Nodes[uiPrevNodeIndex].m_uiUsed, "Two free nodes are next to each other");
    }

    uiExpectedOffset += node.m_uiCount;
    uiPrevNodeIndex = uiNodeIndex;
  }

  EZ_ASSERT_DEBUG(uiPrevNodeIndex == m_uiLastNodeIndex, "Wrong last node");
  EZ_ASSERT_DEBUG(uiExpectedOffset == m_uiCapacity, "Nodes don't cover the full capacity");
  EZ_ASSERT_DEBUG(uiNumUsedNodes == m_OffsetToNodeIndex.GetCount(), "Offset map contains stale entries");
  ezUInt32 uiNumReleasedNodes = 0;
  for (ezUInt32 uiNodeIndex = m_uiFirstReleasedNodeIndex; uiNodeIndex != ezInvalidIndex; uiNodeIndex = m_Nodes[uiNodeIndex].m_uiNextNodeIndex)
  {
    EZ_ASSERT_DEBUG(!m_Nodes[uiNodeIndex].m_uiUsed && m_Nodes[uiNodeIndex].m_uiCount == 0, "Released node is still in use");
    ++uiNumReleasedNodes;
  }

  EZ_ASSERT_DEBUG(uiNumUsedNodes + uiNumFreeNodes + uiNumReleasedNodes == m_Nodes.GetCount(), "Leaked nodes");

  ezUInt32 uiNumNodesInBins = 0;
  for (ezUInt32 uiBin = 0; uiBin < s_uiNumBins; ++uiBin)
  {
    ezUInt32 uiPrevInBin = ezInvalidIndex;
    for (ezUInt32 uiNodeIndex = m_FreeNodeBins[uiBin]; uiNodeIndex != ezInvalidIndex; uiNodeIndex = m_Nodes[uiNodeIndex].m_uiNextNodeIndexInBin)
    {
      const Node& node = m_Nodes[uiNodeIndex];
      EZ_ASSERT_DEBUG(!node.m_uiUsed, "Used node in a bin");
      EZ_ASSERT_DEBUG(BinFloor(node.m_uiCount) == uiBin, "Node is in the wrong bin");
      EZ_ASSERT_DEBUG(node.m_uiPrevNodeIndexInBin == uiPrevInBin, "Broken bin list");
      uiPrevInBin = uiNodeIndex;
      ++uiNumNodesInBins;
    }
  }

  EZ_ASSERT_DEBUG(uiNumNodesInBins == uiNumFreeNodes, "Free node is not in a bin");
#  endif
}
#endif
