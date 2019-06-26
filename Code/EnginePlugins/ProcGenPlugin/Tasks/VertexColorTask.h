#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>

struct ezMeshBufferResourceDescriptor;

namespace ezProcGenInternal
{
  class VertexColorTask : public ezTask
  {
  public:
    VertexColorTask();
    ~VertexColorTask();

    void Prepare(const ezMeshBufferResourceDescriptor& mbDesc, const ezTransform& transform,
      ezArrayPtr<ezSharedPtr<const VertexColorOutput>> outputs, ezArrayPtr<ezUInt32> outputVertexColors);

  private:
    virtual void Execute() override;

    ezHybridArray<ezSharedPtr<const VertexColorOutput>, 2> m_Outputs;

    struct InputVertex
    {
      EZ_DECLARE_POD_TYPE();

      ezVec3 m_vPosition;
      ezVec3 m_vNormal;
      float m_fIndex;
    };

    ezDynamicArray<InputVertex> m_InputVertices;

    ezDynamicArray<ezColor> m_TempData;
    ezArrayPtr<ezUInt32> m_OutputVertexColors;

    ezExpressionVM m_VM;
  };
} // namespace ezProcGenInternal
