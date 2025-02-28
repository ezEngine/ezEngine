#pragma once

#include <RendererCore/Pipeline/Declarations.h>

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

  template <typename T>
  class Iterator
  {
  public:
    const T& operator*() const;
    const T* operator->() const;

    operator const T*() const;

    void Next();

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
