#pragma once

class ezGALDevice;

/// \brief Manages a growing ring buffer of one or more buffers.
/// The behaviour of the class changes depending on whether `Initialize` is called with or without a `ezGALBufferUsageFlags::Transient` buffer descriptor:
///  * Transient: You must call `GetNewBuffer` at the start of every frame and whenever your current buffer has run out of space. Use this for transient data that uses every frame.
///  * Persistent: `GetNewBuffer` must be called once after which you can re-use it every frame and the content in it will persist. You can use this fact to allocate the buffer lazily only in case you actually need it. If you feel the need to call `GetNewBuffer` after the first lazy allocation, you should consider switching to a transient buffer or using a resizable buffer instead.
class EZ_RENDERERFOUNDATION_DLL ezGALBufferPool
{
public:
  ezGALBufferPool() = default;
  ~ezGALBufferPool();

  /// \brief Initializes the pool. Will not create any buffers yet. You need to call `GetNewBuffer` first.
  /// \param desc The descriptor of all buffers in the pool.
  /// \param sDebugName A debug name that is prepended to all buffers created from this pool. The final name will look like this: `{sDebugName}-{UniqueID}#{BufferIndex}, where UniqueID is constant for this pool instance and BufferIndex is the creation index of the buffer inside this pool.
  void Initialize(const ezGALBufferCreationDescription& desc, ezStringView sDebugName);

  /// \brief Destroys all buffers in this pool.
  void Deinitialize();

  /// \brief Returns whether the buffer pool is initialized.
  bool IsInitialized() const { return m_EventSubscriptionID != 0; }

  /// \brief Adds a new buffer to the pool and makes it the current buffer. For transient pools this needs to be called at the start of every frame before use.
  /// \return Handle to the new buffer.
  ezGALBufferHandle GetNewBuffer() const;

  /// \brief Returns the current buffer in the pool. For transient pools this will be invalid at the start of every frame until `GetNewBuffer` is called.
  /// \return Handle to the current buffer.
  ezGALBufferHandle GetCurrentBuffer() const;

private:
  /// \brief Returns all used buffers to the free pool at the end of the frame. Except for the current buffer in case of non-transient pools.
  void GALStaticDeviceEventHandler(const ezGALDeviceEvent& e);

private:
  static ezAtomicInteger32 s_iNumber;

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALBufferPool);
  ezGALBufferCreationDescription m_Desc;
  ezString m_sDebugName;
  ezEventSubscriptionID m_EventSubscriptionID = {};
  ezInt32 m_iUniqueID = 0;
  mutable ezHybridArray<ezGALBufferHandle, 1> m_UsedBuffers;
  mutable ezHybridArray<ezGALBufferHandle, 1> m_FreeBuffers;
};
