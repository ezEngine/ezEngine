#pragma once

#include <Core/CoreDLL.h>

class ezImage;

/// \brief Result of polling an asynchronous capture operation.
struct ezCaptureImageResult
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Ready,      ///< The capture has finished and the image is available.
    Pending,    ///< The capture is still in progress, poll again next frame.
    NotStarted, ///< No capture operation is in progress.
    Default = NotStarted
  };
};

/// \brief Base class for window output targets
///
/// A window output target is usually tied tightly to a window (\sa ezWindowBase) and represents the
/// graphics APIs side of the render output.
/// E.g. in a DirectX implementation this would be a swapchain.
///
/// This interface provides the high level functionality that is needed by ezGameApplication to work with
/// the render output.
class EZ_CORE_DLL ezWindowOutputTargetBase
{
public:
  virtual ~ezWindowOutputTargetBase() = default;
  virtual void AcquireImage() = 0;
  virtual void PresentImage(bool bEnableVSync) = 0;

  /// Starts an asynchronous capture of the current back buffer.
  /// Returns EZ_FAILURE if a capture is already in flight or if the operation cannot be started.
  virtual ezResult StartCaptureImage() = 0;

  /// Polls the state of a pending capture. Returns Ready once the image data
  /// has been copied into out_image.
  virtual ezEnum<ezCaptureImageResult> WaitCaptureImage(ezImage& out_image) = 0;
};
