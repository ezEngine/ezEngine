#pragma once

#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  #error "uwp util header should only be included in UWP builds!"
#endif

#include <Foundation/Types/Types.h>
#include <Foundation/Basics/Platform/Win/HResultUtils.h>

#include <windows.foundation.h>
#include <windows.foundation.numerics.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL::Wrappers;
using namespace Microsoft::WRL;

namespace ezUwpUtils
{
  /// Helper function to iterate over all elements of a ABI::Windows::Foundation::Collections::IVectorView
  /// \param callback
  ///   Callable of signature bool(UINT index, const ComPtr<Interface>& pElement). Return value of false means discontinue.
  template<typename ElementQueryType, typename ElementType, typename Callback>
  HRESULT ezWinRtIterateIVectorView(const ComPtr<ABI::Windows::Foundation::Collections::IVectorView<ElementType>>& pVectorView, const Callback& callback)
  {
    UINT numElements = 0;
    HRESULT result = pVectorView->get_Size(&numElements);
    if (FAILED(result))
      return result;

    for (UINT i = 0; i < numElements; ++i)
    {
      ElementQueryType pElement;
      result = pVectorView->GetAt(i, &pElement);
      if (FAILED(result))
        return result;

      if (!callback(i, pElement))
        return S_OK;
    }

    return result;
  }

  template<typename T>
  void RetrieveStatics(const WCHAR* szRuntimeClassName, ComPtr<T>& out_Ptr)
  {
    if (FAILED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(szRuntimeClassName).Get(), &out_Ptr)))
    {
      EZ_REPORT_FAILURE("Failed to retrieve activation factor (statics) for '{0}'", ezStringUtf8(szRuntimeClassName).GetData());
      out_Ptr = nullptr;
    }
  }

  ezMat4 EZ_FOUNDATION_DLL ConvertMat4(const ABI::Windows::Foundation::Numerics::Matrix4x4& in);
  ezMat4 EZ_FOUNDATION_DLL ConvertMat4(ABI::Windows::Foundation::__FIReference_1_Windows__CFoundation__CNumerics__CMatrix4x4_t* in);

  ezVec3 EZ_FOUNDATION_DLL ConvertVec3(const ABI::Windows::Foundation::Numerics::Vector3& in);
  void EZ_FOUNDATION_DLL ConvertVec3(const ezVec3& in, ABI::Windows::Foundation::Numerics::Vector3& out);
  ezVec3 EZ_FOUNDATION_DLL ConvertVec3(ABI::Windows::Foundation::__FIReference_1_Windows__CFoundation__CNumerics__CVector3_t* in);

  ezQuat EZ_FOUNDATION_DLL ConvertQuat(const ABI::Windows::Foundation::Numerics::Quaternion& in);
  void EZ_FOUNDATION_DLL ConvertQuat(const ezQuat& in, ABI::Windows::Foundation::Numerics::Quaternion& out);
}
