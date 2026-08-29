#include <Core/CorePCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldLogLink.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace
{
  /// The link markup can easily be longer than the temp buffer that BuildString() is given,
  /// so the result is written into a thread local buffer instead (same approach as BuildString()
  /// for ezArgErrorCode). All arguments of one format string are built before any of them is used,
  /// so a small ring of buffers is needed instead of a single one.
  ezStringView StoreInThreadLocalBuffer(const ezStringBuilder& sText)
  {
    constexpr ezUInt32 uiNumBuffers = ezFormatString::MaxNumParameters;

    static thread_local ezStringBuilder s_Buffers[uiNumBuffers];
    static thread_local ezUInt32 s_uiNextBuffer = 0;

    ezStringBuilder& sBuffer = s_Buffers[s_uiNextBuffer];
    s_uiNextBuffer = (s_uiNextBuffer + 1) % uiNumBuffers;

    sBuffer = sText;
    return sBuffer.GetView();
  }

  void AppendLink(ezStringBuilder& out_sResult, ezStringView sScheme, ezUInt64 uiHandleData, ezStringView sDisplayText, ezStringView sFallbackName)
  {
    out_sResult.Append("[[");

    if (!sDisplayText.IsEmpty())
    {
      out_sResult.Append(sDisplayText);
    }
    else if (!sFallbackName.IsEmpty())
    {
      out_sResult.Append(sFallbackName);
    }
    else
    {
      out_sResult.AppendFormat("{}", ezArgU(uiHandleData, 1, false, 16));
    }

    out_sResult.Append("|", sScheme);
    out_sResult.AppendFormat("{}", ezArgU(uiHandleData, 1, false, 16));
    out_sResult.Append("]]");
  }

  bool ParseLink(ezStringView sLinkTarget, ezStringView sScheme, ezUInt64& out_uiHandleData)
  {
    sLinkTarget.Trim();

    if (!sLinkTarget.TrimWordStart(sScheme))
      return false;

    return ezConversionUtils::ConvertHexStringToUInt64(sLinkTarget, out_uiHandleData).Succeeded();
  }
} // namespace

ezArgGameObject::ezArgGameObject(const ezGameObject* pObject, ezStringView sDisplayText)
  : m_hObject(pObject != nullptr ? pObject->GetHandle() : ezGameObjectHandle())
  , m_sDisplayText(sDisplayText)
{
}

ezArgComponent::ezArgComponent(const ezComponent* pComponent, ezStringView sDisplayText)
  : m_hComponent(pComponent != nullptr ? pComponent->GetHandle() : ezComponentHandle())
  , m_sDisplayText(sDisplayText)
{
}

void ezWorldLogLinkUtils::AppendGameObjectLink(ezStringBuilder& out_sResult, ezGameObjectHandle hObject, ezStringView sDisplayText)
{
  ezStringView sName;

  if (sDisplayText.IsEmpty() && !hObject.IsInvalidated())
  {
    if (const ezWorld* pWorld = ezWorld::GetWorld(hObject))
    {
      EZ_LOCK(pWorld->GetReadMarker());

      const ezGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject))
      {
        sName = pObject->GetName();
      }
    }
  }

  AppendLink(out_sResult, s_sGameObjectScheme, hObject.GetInternalID().m_Data, sDisplayText, sName);
}

void ezWorldLogLinkUtils::AppendComponentLink(ezStringBuilder& out_sResult, ezComponentHandle hComponent, ezStringView sDisplayText)
{
  ezStringBuilder sName;

  if (sDisplayText.IsEmpty() && !hComponent.IsInvalidated())
  {
    if (const ezWorld* pWorld = ezWorld::GetWorld(hComponent))
    {
      EZ_LOCK(pWorld->GetReadMarker());

      const ezComponent* pComponent = nullptr;
      if (pWorld->TryGetComponent(hComponent, pComponent))
      {
        sName = pComponent->GetDynamicRTTI()->GetTypeName();

        if (const ezGameObject* pOwner = pComponent->GetOwner(); pOwner != nullptr && !pOwner->GetName().IsEmpty())
        {
          sName.AppendFormat(" ({})", pOwner->GetName());
        }
      }
    }
  }

  AppendLink(out_sResult, s_sComponentScheme, hComponent.GetInternalID().m_Data, sDisplayText, sName);
}

bool ezWorldLogLinkUtils::ParseGameObjectLink(ezStringView sLinkTarget, ezGameObjectHandle& out_hObject)
{
  ezUInt64 uiData = 0;
  if (!ParseLink(sLinkTarget, s_sGameObjectScheme, uiData))
    return false;

  out_hObject = ezGameObjectHandle(ezGameObjectId(uiData));
  return true;
}

bool ezWorldLogLinkUtils::ParseComponentLink(ezStringView sLinkTarget, ezComponentHandle& out_hComponent)
{
  ezUInt64 uiData = 0;
  if (!ParseLink(sLinkTarget, s_sComponentScheme, uiData))
    return false;

  out_hComponent = ezComponentHandle(ezComponentId(uiData));
  return true;
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgGameObject& arg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);

  ezStringBuilder sLink;
  ezWorldLogLinkUtils::AppendGameObjectLink(sLink, arg.m_hObject, arg.m_sDisplayText);
  return StoreInThreadLocalBuffer(sLink);
}

ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezArgComponent& arg)
{
  EZ_IGNORE_UNUSED(szTmp);
  EZ_IGNORE_UNUSED(uiLength);

  ezStringBuilder sLink;
  ezWorldLogLinkUtils::AppendComponentLink(sLink, arg.m_hComponent, arg.m_sDisplayText);
  return StoreInThreadLocalBuffer(sLink);
}
