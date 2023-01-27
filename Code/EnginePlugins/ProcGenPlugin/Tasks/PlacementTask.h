#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

class ezPhysicsWorldModuleInterface;
class ezVolumeCollection;

namespace ezProcGenInternal
{
  class PlacementTask final : public ezTask
  {
  public:
    PlacementTask(PlacementData* pData, const char* szName);
    ~PlacementTask();

    void Clear();

    ezArrayPtr<const PlacementPoint> GetInputPoints() const { return m_InputPoints; }
    ezArrayPtr<const PlacementTransform> GetOutputTransforms() const { return m_OutputTransforms; }

  private:
    virtual void Execute() override;

    void FindPlacementPoints();
    void ExecuteVM();

    ezProcessingStream MakeInputStream(const ezHashedString& sName, ezUInt32 uiOffset, ezProcessingStream::DataType dataType = ezProcessingStream::DataType::Float)
    {
      return ezProcessingStream(sName, m_InputPoints.GetByteArrayPtr().GetSubArray(uiOffset), dataType, sizeof(PlacementPoint));
    }

    ezProcessingStream MakeOutputStream(const ezHashedString& sName, ezUInt32 uiOffset, ezProcessingStream::DataType dataType = ezProcessingStream::DataType::Float)
    {
      return ezProcessingStream(sName, m_InputPoints.GetByteArrayPtr().GetSubArray(uiOffset), dataType, sizeof(PlacementPoint));
    }

    PlacementData* m_pData = nullptr;

    ezDynamicArray<PlacementPoint, ezAlignedAllocatorWrapper> m_InputPoints;
    ezDynamicArray<PlacementTransform, ezAlignedAllocatorWrapper> m_OutputTransforms;
    ezDynamicArray<float> m_Density;
    ezDynamicArray<ezUInt32> m_ValidPoints;

    ezExpressionVM m_VM;
  };
} // namespace ezProcGenInternal
