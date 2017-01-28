#include <ParticlePlugin/PCH.h>
//#include <ParticlePlugin/Initializer/ParticleInitializer_Rise.h>
//#include <Core/World/WorldModule.h>
//#include <GameEngine/Interfaces/PhysicsWorldModule.h>
//#include <ParticlePlugin/System/ParticleSystemInstance.h>
//#include <Core/World/World.h>
//#include <Foundation/Time/Clock.h>
//#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
//
//EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializerFactory_Rise, 1, ezRTTIDefaultAllocator<ezParticleInitializerFactory_Rise>)
//{
//  EZ_BEGIN_PROPERTIES
//  {
//    EZ_MEMBER_PROPERTY("Min Speed", m_fMinRiseSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
//    EZ_MEMBER_PROPERTY("Speed Range", m_fRiseSpeedRange)
//  }
//  EZ_END_PROPERTIES
//}
//EZ_END_DYNAMIC_REFLECTED_TYPE
//
//EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleInitializer_Rise, 1, ezRTTIDefaultAllocator<ezParticleInitializer_Rise>)
//EZ_END_DYNAMIC_REFLECTED_TYPE
//
//ezParticleInitializerFactory_Rise::ezParticleInitializerFactory_Rise()
//{
//  m_fMinRiseSpeed = 1.0f;
//  m_fRiseSpeedRange = 0.0f;
//}
//
//const ezRTTI* ezParticleInitializerFactory_Rise::GetInitializerType() const
//{
//  return ezGetStaticRTTI<ezParticleInitializer_Rise>();
//}
//
//void ezParticleInitializerFactory_Rise::CopyInitializerProperties(ezParticleInitializer* pInitializer0) const
//{
//  ezParticleInitializer_Rise* pInitializer = static_cast<ezParticleInitializer_Rise*>(pInitializer0);
//
//  pInitializer->m_fMinRiseSpeed = m_fMinRiseSpeed;
//  pInitializer->m_fRiseSpeedRange = m_fRiseSpeedRange;
//}
//
//void ezParticleInitializerFactory_Rise::Save(ezStreamWriter& stream) const
//{
//  const ezUInt8 uiVersion = 1;
//  stream << uiVersion;
//
//  stream << m_fMinRiseSpeed;
//  stream << m_fRiseSpeedRange;
//}
//
//void ezParticleInitializerFactory_Rise::Load(ezStreamReader& stream)
//{
//  ezUInt8 uiVersion = 0;
//  stream >> uiVersion;
//
//  stream >> m_fMinRiseSpeed;
//  stream >> m_fRiseSpeedRange;
//}
//
//void ezParticleInitializer_Rise::AfterPropertiesConfigured(bool bFirstTime)
//{
//  m_pPhysicsModule = static_cast<ezPhysicsWorldModuleInterface*>(ezWorldModule::FindModule(GetOwnerSystem()->GetWorld(), ezPhysicsWorldModuleInterface::GetStaticRTTI()));
//}
//
//
//void ezParticleInitializer_Rise::CreateRequiredStreams()
//{
//  CreateStream("Velocity", ezProcessingStream::DataType::Float3, &m_pStreamVelocity);
//}
//
//void ezParticleInitializer_Rise::InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements)
//{
//  const ezVec3 vGravity = m_pPhysicsModule->GetGravity();
//
//  ezProcessingStreamIterator<ezVec3> itVelocity(m_pStreamVelocity, uiNumElements, uiStartIndex);
//
//  if (m_fRiseSpeedRange == 0)
//  {
//    const ezVec3 vVelocity = -vGravity.GetNormalized() * m_fMinRiseSpeed;
//
//    while (!itVelocity.HasReachedEnd())
//    {
//      itVelocity.Current() = vVelocity;
//
//      itVelocity.Advance();
//    }
//  }
//  else
//  {
//    ezRandom& rng = GetRNG();
//
//    const ezVec3 vVelocity = -vGravity.GetNormalized();
//
//    while (!itVelocity.HasReachedEnd())
//    {
//      const float speed = (float)rng.DoubleInRange(m_fMinRiseSpeed, m_fRiseSpeedRange);
//
//      itVelocity.Current() = vVelocity * speed;
//
//      itVelocity.Advance();
//    }
//  }
//}
//
