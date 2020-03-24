#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>

struct ezMeshBufferResourceDescriptor;
class ezVolumeCollection;

namespace ezProcGenInternal
{
  class VertexColorTask final : public ezTask
  {
  public:
    VertexColorTask();
    ~VertexColorTask();

    void Prepare(const ezWorld& world, const ezMeshBufferResourceDescriptor& mbDesc, const ezTransform& transform,
      ezArrayPtr<ezSharedPtr<const VertexColorOutput>> outputs, ezArrayPtr<ezProcVertexColorMapping> outputMappings,
      ezArrayPtr<ezUInt32> outputVertexColors);

  private:
    virtual void Execute() override;

    ezHybridArray<ezSharedPtr<const VertexColorOutput>, 2> m_Outputs;
    ezHybridArray<ezProcVertexColorMapping, 2> m_OutputMappings;

    struct InputVertex
    {
      EZ_DECLARE_POD_TYPE();

      ezVec3 m_vPosition;
      ezVec3 m_vNormal;
      ezColor m_Color;
      float m_fIndex;
    };

    ezDynamicArray<InputVertex> m_InputVertices;

    ezDynamicArray<ezColor> m_TempData;
    ezArrayPtr<ezUInt32> m_OutputVertexColors;

    ezDynamicArray<ezVolumeCollection> m_VolumeCollections;
    ezExpression::GlobalData m_GlobalData;

    ezExpressionVM m_VM;
  };
} // namespace ezProcGenInternal
