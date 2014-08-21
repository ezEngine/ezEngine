#include <PCH.h>
#include <RTS/Components/FollowPathSteeringComponent.h>
#include <RTS/Level.h>

EZ_BEGIN_COMPONENT_TYPE(FollowPathSteeringComponent, SteeringBehaviorComponent, 1, FollowPathSteeringComponentManager);
EZ_END_COMPONENT_TYPE();

FollowPathSteeringComponent::FollowPathSteeringComponent()
{
  m_pPath = nullptr;
}

void FollowPathSteeringComponent::Update()
{
  for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
  {
    m_fDirectionDesire[i] = 0;
    m_fDirectionWhisker[i] = 5.0f;
  }

  if (m_pPath == nullptr)
  {
    return;
  }

  if (m_pPath->IsEmpty())
  {
    return;
  }

  const ezVec3 vCurUnitPos = GetOwner()->GetLocalPosition();


  ezInt32 iCurPathNode = m_pPath->GetCount() - 1;

  const float fSampleLength = 2.0f;
  float fClosestDistance = ezMath::BasicType<float>::GetInfinity();
  

  ezVec3 vPrevPathPos = (*m_pPath)[iCurPathNode];
  ezVec3 vClosestPathPos = vPrevPathPos;

  while (iCurPathNode > 0)
  {
    --iCurPathNode;
    ezVec3 vCurPathPos = (*m_pPath)[iCurPathNode];

    const ezVec3 vSegment = vPrevPathPos - vCurPathPos;
    ezVec3 vSegmentDir = vSegment;
    float fSegmentLength = vSegmentDir.GetLengthAndNormalize();

    ezVec3 vSamplePos = vPrevPathPos;

    while (fSegmentLength > 0)
    {
      const float fDistSQR = (vSamplePos - vCurUnitPos).GetLengthSquared();

      if (fDistSQR < ezMath::Square(fSampleLength))
        goto done;

      if (fDistSQR < fClosestDistance)
      {
        fClosestDistance = fDistSQR;
        vClosestPathPos = vSamplePos;
      }

      vSamplePos -= vSegmentDir * fSampleLength;
      fSegmentLength -= fSampleLength;
    }

    vPrevPathPos = vCurPathPos;
  }

done:

  ezVec3 vBestDir = vClosestPathPos - vCurUnitPos;
  ezVec3 vBestDirNorm = vBestDir.GetNormalized();

  for (ezInt32 i = 0; i < g_iSteeringDirections; ++i)
  {
    m_fDirectionDesire[i] = SteeringBehaviorComponent::g_vSteeringDirections[i].Dot(vBestDirNorm) * ezMath::Min(5.0f, vBestDir.GetLength());
  }
}

