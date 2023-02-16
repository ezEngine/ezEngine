#pragma once

#include <KrautFoundation/Math/Vec3.h>
#include <KrautGenerator/Description/DescriptionEnums.h>

namespace Kraut
{
  using namespace AE_NS_FOUNDATION;

  struct TreeStructureDesc;
  struct TreeStructure;

  struct KRAUT_DLL Physics
  {
    Physics();
    virtual ~Physics();

    virtual void Reset() = 0;

    virtual bool IsLineObstructed(const aeVec3& vPos, const aeVec3& vPosTo, float& out_fLineFraction) const = 0;

    virtual aeVec3 FindLeastObstructedDirection(const aeVec3& vPos, const aeVec3& vPosTo, aeUInt32 uiMaxDeviation, float& out_fDistance) const = 0;

    virtual void CreateBranchCollisionCapsules(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, aeUInt32 uiBranch) = 0;
  };

  struct KRAUT_DLL Physics_EmptyImpl : public Physics
  {
    virtual void Reset() override;

    virtual bool IsLineObstructed(const aeVec3& vPos, const aeVec3& vPosTo, float& out_fLineFraction) const override;

    virtual aeVec3 FindLeastObstructedDirection(const aeVec3& vPos, const aeVec3& vPosTo, aeUInt32 uiMaxDeviation, float& out_fDistance) const override;

    virtual void CreateBranchCollisionCapsules(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, aeUInt32 uiBranch) override;
  };

  struct KRAUT_DLL Physics_BaseImpl : public Physics
  {
    virtual aeVec3 FindLeastObstructedDirection(const aeVec3& vPos, const aeVec3& vPosTo, aeUInt32 uiMaxDeviation, float& out_fDistance) const override;

    virtual void CreateBranchCollisionCapsules(const Kraut::TreeStructureDesc& treeStructureDesc, const Kraut::TreeStructure& treeStructure, aeUInt32 uiBranch) override;

    virtual void AddColliderCapsule(const aeVec3& vPos, const aeVec3& vPosTo, float fRadius) = 0;
  };

} // namespace Kraut
