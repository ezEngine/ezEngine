#include <CoreTest/CoreTestPCH.h>

#include <Foundation/Time/Stopwatch.h>

namespace
{
  struct TextureDesc
  {
    ezUInt32 m_uiWidth = 0;
    ezUInt32 m_uiHeight = 0;
    ezUInt32 m_uiDepth = 1;
    ezUInt32 m_uiMipLevelCount = 1;
    ezUInt32 m_uiArraySize = 1;

    ezUInt8 m_Format = 0;
    ezUInt8 m_SampleCount = 0;
    ezUInt8 m_Type = 0;

    bool m_bAllowShaderResourceView = true;
    bool m_bAllowUAV = false;
    bool m_bAllowRenderTargetView = false;
    bool m_bAllowDynamicMipGeneration = false;

    bool m_ResourceAccess = false;

    void* m_pExisitingNativeObject = nullptr; ///< Can be used to encapsulate existing native textures in objects usable by the GAL
  };


  using TextureId = ezGenericId<18, 14>;
  using TextureViewId = ezGenericId<18, 14>;

  struct TextureHandle
  {
    EZ_DECLARE_HANDLE_TYPE(TextureHandle, TextureId);

    friend class Device;
  };

  struct TextureViewHandle
  {
    EZ_DECLARE_HANDLE_TYPE(TextureViewHandle, TextureId);

    friend class Device;
  };

  struct TextureView
  {
    void* nativePtr = nullptr;
  };

  struct Texture
  {
    TextureDesc m_Desc;

    TextureViewHandle m_hView;
  };

  class Device
  {
  public:
    ~Device()
    {
      for (auto it = m_Textures.GetIterator(); it.IsValid(); ++it)
      {
        EZ_DEFAULT_DELETE(it.Value());
      }
      m_Textures.Clear();

      for (auto it = m_TextureViews.GetIterator(); it.IsValid(); ++it)
      {
        EZ_DEFAULT_DELETE(it.Value());
      }
      m_TextureViews.Clear();
    }

    TextureHandle CreateTexture(const TextureDesc& desc)
    {
      EZ_LOCK(m_Mutex);

      auto pTexture = EZ_DEFAULT_NEW(Texture);
      pTexture->m_Desc = desc;

      pTexture->m_hView = CreateTextureView();

      return TextureHandle(m_Textures.Insert(pTexture));
    }

    TextureViewHandle CreateTextureView()
    {
      EZ_LOCK(m_Mutex);

      auto pView = EZ_DEFAULT_NEW(TextureView);
      pView->nativePtr = (void*)size_t(0x70000 + m_TextureViews.GetCount());

      return TextureViewHandle(m_TextureViews.Insert(pView));
    }

    TextureView* GetDefaultView(TextureHandle hTexture)
    {
      EZ_LOCK(m_Mutex);

      Texture* pTexture = nullptr;
      if (!m_Textures.TryGetValue(hTexture, pTexture))
        return nullptr;

      TextureView* pView = nullptr;
      m_TextureViews.TryGetValue(pTexture->m_hView, pView);
      return pView;
    }

  private:
    ezMutex m_Mutex;
    ezIdTable<TextureId, Texture*> m_Textures;
    ezIdTable<TextureViewId, TextureView*> m_TextureViews;
  };

  //////////////////////////////////////////////////////////////////////////

  struct TextureViewRefCounted : public ezRefCounted
  {
    void* nativePtr = nullptr;
  };

  struct TextureRefCounted : public ezRefCounted
  {
    TextureDesc m_Desc;

    ezSharedPtr<TextureViewRefCounted> m_pView;
  };

  class DeviceRefCounted
  {
  public:
    ezSharedPtr<TextureRefCounted> CreateTexture(const TextureDesc& desc)
    {
      EZ_LOCK(m_Mutex);

      auto pTexture = EZ_DEFAULT_NEW(TextureRefCounted);
      pTexture->m_Desc = desc;

      pTexture->m_pView = CreateTextureView();

      return pTexture;
    }

    ezSharedPtr<TextureViewRefCounted> CreateTextureView()
    {
      EZ_LOCK(m_Mutex);

      auto pView = EZ_DEFAULT_NEW(TextureViewRefCounted);
      pView->nativePtr = (void*)size_t(0x90000 + m_uiTextureViewCounter);

      ++m_uiTextureViewCounter;

      return pView;
    }

  private:
    ezMutex m_Mutex;
    ezUInt32 m_uiTextureViewCounter = 0;
  };

} // namespace

EZ_CREATE_SIMPLE_TEST(World, SharedVsIdTable)
{
  constexpr ezUInt32 uiTextureCount = 100000;

  ezDynamicArray<ezUInt32> temp;
  for (ezUInt32 i = 0; i < uiTextureCount; ++i)
    temp.PushBack(i);

  ezRandom r;
  r.Initialize(123);

  ezDynamicArray<ezUInt32> indices;
  while (temp.IsEmpty() == false)
  {
    ezUInt32 uiIndex = r.UIntInRange(temp.GetCount());
    indices.PushBack(temp[uiIndex]);
    temp.RemoveAtAndSwap(uiIndex);
  }

  TextureDesc desc;
  desc.m_uiWidth = 1024;
  desc.m_uiHeight = 1024;

  {
    DeviceRefCounted device;

    ezDynamicArray<ezSharedPtr<TextureRefCounted>> textures;
    textures.Reserve(uiTextureCount);

    for (ezUInt32 i = 0; i < uiTextureCount; ++i)
    {
      textures.PushBack(device.CreateTexture(desc));
    }

    ezDynamicArray<void*> views;
    views.Reserve(uiTextureCount);

    ezStopwatch timer;

    for (auto uiIndex : indices)
    {
      auto& pView = textures.GetData()[uiIndex]->m_pView;
      views.PushBack(pView->nativePtr);
    }

    ezLog::Error("SharedPtr: {}ms", ezArgF(timer.GetRunningTotal().GetMilliseconds(), 3));
  }

  {
    Device device;

    ezDynamicArray<TextureHandle> textures;
    textures.Reserve(uiTextureCount);

    for (ezUInt32 i = 0; i < uiTextureCount; ++i)
    {
      textures.PushBack(device.CreateTexture(desc));
    }

    ezDynamicArray<void*> views;
    views.Reserve(uiTextureCount);

    ezStopwatch timer;

    for (auto uiIndex : indices)
    {
      auto pView = device.GetDefaultView(textures.GetData()[uiIndex]);
      views.PushBack(pView->nativePtr);
    }

    ezLog::Error("IdTable: {}ms", ezArgF(timer.GetRunningTotal().GetMilliseconds(), 3));
  }  
}
