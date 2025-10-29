#pragma once

#include <RendererCore/Pipeline/Declarations.h>

/// Represents a batch of render data that can be rendered together.
///
/// Render data is grouped into batches to minimize state changes during rendering.
/// Each batch contains render data of the same type, sorted by a sorting key.
/// Provides iterator access to iterate through the typed render data.
class ezRenderDataBatch
{
private:
  struct SortableRenderData
  {
    EZ_DECLARE_POD_TYPE();

    const ezRenderData* m_pRenderData;
    ezUInt64 m_uiSortingKey;
  };

public:
  EZ_DECLARE_POD_TYPE();

  /// Iterator for traversing typed render data within a batch.
  template <typename T>
  class Iterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    /// Advances to the next element.
    void Next();

    /// Returns true if the iterator points to a valid element.
    bool IsValid() const;

    void operator++();

  private:
    friend class ezRenderDataBatch;

    Iterator(const SortableRenderData* pStart, const SortableRenderData* pEnd);

    const SortableRenderData* m_pCurrent;
    const SortableRenderData* m_pEnd;
  };

  ezUInt32 GetCount() const;

  template <typename T>
  const T* GetFirstData() const;

  template <typename T>
  Iterator<T> GetIterator(ezUInt32 uiStartIndex = 0, ezUInt32 uiCount = ezInvalidIndex) const;

private:
  friend class ezExtractedRenderData;
  friend class ezRenderDataBatchList;

  ezArrayPtr<SortableRenderData> m_Data;
};

/// Contains a list of render data batches for a specific render category.
///
/// Used to access all batches that need to be rendered for a particular category.
class ezRenderDataBatchList
{
public:
  ezUInt32 GetBatchCount() const;

  const ezRenderDataBatch& GetBatch(ezUInt32 uiIndex) const;

private:
  friend class ezExtractedRenderData;

  ezArrayPtr<const ezRenderDataBatch> m_Batches;
};

#include <RendererCore/Pipeline/Implementation/RenderDataBatch_inl.h>
