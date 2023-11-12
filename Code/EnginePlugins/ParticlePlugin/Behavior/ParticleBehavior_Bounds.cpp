#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Bounds.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehaviorFactory_Bounds, 1, ezRTTIDefaultAllocator<ezParticleBehaviorFactory_Bounds>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PositionOffset", m_vPositionOffset),
    EZ_MEMBER_PROPERTY("BoxExtents", m_vBoxExtents)->AddAttributes(new ezDefaultValueAttribute(ezVec3(2, 2, 2))),
    EZ_ENUM_MEMBER_PROPERTY("OutOfBoundsMode", ezParticleOutOfBoundsMode, m_OutOfBoundsMode),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezBoxVisualizerAttribute("BoxExtents", 1.0f, ezColor::LightGreen, nullptr, ezVisualizerAnchor::Center, ezVec3(1.0f), "PositionOffset")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleBehavior_Bounds, 1, ezRTTIDefaultAllocator<ezParticleBehavior_Bounds>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezParticleBehaviorFactory_Bounds::ezParticleBehaviorFactory_Bounds() = default;

const ezRTTI* ezParticleBehaviorFactory_Bounds::GetBehaviorType() const
{
  return ezGetStaticRTTI<ezParticleBehavior_Bounds>();
}

void ezParticleBehaviorFactory_Bounds::CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const
{
  ezParticleBehavior_Bounds* pBehavior = static_cast<ezParticleBehavior_Bounds*>(pObject);

  pBehavior->m_vPositionOffset = m_vPositionOffset;
  pBehavior->m_vBoxExtents = m_vBoxExtents;
  pBehavior->m_OutOfBoundsMode = m_OutOfBoundsMode;
}

enum class BehaviorBoundsVersion
{
  Version_0 = 0,
  Version_1, // added out of bounds mode

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void ezParticleBehaviorFactory_Bounds::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = (int)BehaviorBoundsVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_vPositionOffset;
  inout_stream << m_vBoxExtents;

  // version 1
  inout_stream << m_OutOfBoundsMode;
}

void ezParticleBehaviorFactory_Bounds::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion <= (int)BehaviorBoundsVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_vPositionOffset;
  inout_stream >> m_vBoxExtents;

  if (uiVersion >= 1)
  {
    inout_stream >> m_OutOfBoundsMode;
  }
}

void ezParticleBehavior_Bounds::CreateRequiredStreams()
{
  CreateStream("Position", ezProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void ezParticleBehavior_Bounds::QueryOptionalStreams()
{
  m_pStreamLastPosition = GetOwnerSystem()->QueryStream("LastPosition", ezProcessingStream::DataType::Float3);
}

void ezParticleBehavior_Bounds::Process(ezUInt64 uiNumElements)
{
  EZ_PROFILE_SCOPE("PFX: Bounds");

  const ezSimdTransform trans = ezSimdConversion::ToTransform(GetOwnerSystem()->GetTransform());
  const ezSimdTransform invTrans = trans.GetInverse();

  const ezSimdVec4f boxCenter = ezSimdConversion::ToVec3(m_vPositionOffset);
  const ezSimdVec4f boxExt = ezSimdConversion::ToVec3(m_vBoxExtents);
  const ezSimdVec4f halfExtPos = ezSimdConversion::ToVec3(m_vBoxExtents) * 0.5f;
  const ezSimdVec4f halfExtNeg = -halfExtPos;

  ezProcessingStreamIterator<ezSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);

  if (m_OutOfBoundsMode == ezParticleOutOfBoundsMode::Teleport)
  {
    ezVec3* pLastPosition = nullptr;

    if (m_pStreamLastPosition)
    {
      pLastPosition = m_pStreamLastPosition->GetWritableData<ezVec3>();
    }

    while (!itPosition.HasReachedEnd())
    {
      const ezSimdVec4f globalPosCur = itPosition.Current();
      const ezSimdVec4f localPosCur = invTrans.TransformPosition(globalPosCur) - boxCenter;

      const ezSimdVec4f localPosAdd = localPosCur + boxExt;
      const ezSimdVec4f localPosSub = localPosCur - boxExt;

      ezSimdVec4f localPosNew;
      localPosNew = ezSimdVec4f::Select(localPosCur > halfExtPos, localPosSub, localPosCur);
      localPosNew = ezSimdVec4f::Select(localPosCur < halfExtNeg, localPosAdd, localPosNew);

      localPosNew += boxCenter;
      const ezSimdVec4f globalPosNew = trans.TransformPosition(localPosNew);

      if (m_pStreamLastPosition)
      {
        const ezSimdVec4f posDiff = globalPosNew - globalPosCur;
        *pLastPosition += ezSimdConversion::ToVec3(posDiff);
        ++pLastPosition;
      }

      itPosition.Current() = globalPosNew;
      itPosition.Advance();
    }
  }
  else
  {
    ezUInt32 idx = 0;

    while (!itPosition.HasReachedEnd())
    {
      const ezSimdVec4f globalPosCur = itPosition.Current();
      const ezSimdVec4f localPosCur = invTrans.TransformPosition(globalPosCur) - boxCenter;

      if ((localPosCur > halfExtPos).AnySet() || (localPosCur < halfExtNeg).AnySet())
      {
        m_pStreamGroup->RemoveElement(idx);
      }

      ++idx;
      itPosition.Advance();
    }
  }
}


EZ_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Bounds);

