#pragma once
#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

struct ezVulkanDescriptorSetLayoutBinding
{
  enum ResourceType : ezUInt8
  {
    ConstantBuffer,
    ResourceView,
    UAV,
    Sampler,
  };

  EZ_DECLARE_POD_TYPE();
  ezStringView m_sName; ///< Used to match the same descriptor use across multiple stages.
  ezUInt8 m_uiBinding = 0;
  ResourceType m_Type = ResourceType::ConstantBuffer;
  ezUInt16 m_uiDescriptorType = 0; ///< Maps to vk::DescriptorType
  ezUInt32 m_uiDescriptorCount = 1;
  ezUInt32 m_uiWordOffset = 0; ///< Offset of the location in the spirv code where the binding index is located to allow changing it at runtime.
};

struct ezVulkanDescriptorSetLayout
{
  ezUInt32 m_uiSet = 0;
  ezHybridArray<ezVulkanDescriptorSetLayoutBinding, 6> bindings;
};

namespace ezSpirvMetaData
{
  constexpr ezUInt32 s_uiSpirvMetaDataMagicNumber = 0x4B565A45; //EZVK

  enum MetaDataVersion
  {
    Version1 = 1,
  };

  void Write(ezStreamWriter& stream, const ezArrayPtr<ezUInt8>& shaderCode, const ezDynamicArray<ezVulkanDescriptorSetLayout>& sets)
  {
    stream << s_uiSpirvMetaDataMagicNumber;
    stream.WriteVersion(MetaDataVersion::Version1);
    const ezUInt32 uiSize = shaderCode.GetCount();
    stream << uiSize;
    stream.WriteBytes(shaderCode.GetPtr(), uiSize).AssertSuccess();

    const ezUInt8 uiSets = sets.GetCount();
    stream << uiSets;
    for (ezUInt8 i = 0; i < uiSets; i++)
    {
      const ezVulkanDescriptorSetLayout& set = sets[i];
      stream << set.m_uiSet;
      const ezUInt8 uiBindings = set.bindings.GetCount();
      stream << uiBindings;
      for (ezUInt8 j = 0; j < uiBindings; j++)
      {
        const ezVulkanDescriptorSetLayoutBinding& binding = set.bindings[j];
        stream.WriteString(binding.m_sName).AssertSuccess();
        stream << binding.m_uiBinding;
        stream << static_cast<ezUInt8>(binding.m_Type);
        stream << binding.m_uiDescriptorType;
        stream << binding.m_uiDescriptorCount;
        stream << binding.m_uiWordOffset;
      }
    }
  }

  /// \brief Reads Vulkan shader code and meta data from a data buffer. Note that 'data' must be kept alive for the lifetime of the shader as this functions stores views into this memory in its out parameters.
  /// \param data Raw data buffer to read the shader code and meta data from.
  /// \param out_shaderCode Will be filled with a view into data that contains the shader byte code.
  /// \param out_sets Will be filled with shader meta data. Note that this array contains string views into 'data'.
  void Read(const ezArrayPtr<const ezUInt8> data, ezArrayPtr<const ezUInt8>& out_shaderCode, ezDynamicArray<ezVulkanDescriptorSetLayout>& out_sets)
  {
    ezRawMemoryStreamReader stream(data.GetPtr(), data.GetCount());

    ezUInt32 uiMagicNumber;
    stream >> uiMagicNumber;
    EZ_ASSERT_DEV(uiMagicNumber == s_uiSpirvMetaDataMagicNumber, "Vulkan shader does not start with s_uiSpirvMetaDataMagicNumber");
    ezTypeVersion uiVersion = stream.ReadVersion(MetaDataVersion::Version1);
    EZ_ASSERT_DEV(uiVersion == MetaDataVersion::Version1, "Unknown Vulkan shader version '{}'", uiVersion);

    ezUInt32 uiSize = 0;
    stream >> uiSize;
    out_shaderCode = ezArrayPtr<const ezUInt8>(&data[(ezUInt32)stream.GetReadPosition()], uiSize);
    stream.SkipBytes(uiSize);

    ezUInt8 uiSets = 0;
    stream >> uiSets;
    out_sets.Reserve(uiSets);

    for (ezUInt8 i = 0; i < uiSets; i++)
    {
      ezVulkanDescriptorSetLayout& set = out_sets.ExpandAndGetRef();
      stream >> set.m_uiSet;
      ezUInt8 uiBindings = 0;
      stream >> uiBindings;
      set.bindings.Reserve(uiBindings);

      for (ezUInt8 j = 0; j < uiBindings; j++)
      {
        ezVulkanDescriptorSetLayoutBinding& binding = set.bindings.ExpandAndGetRef();

        ezUInt32 uiStringElements = 0;
        stream >> uiStringElements;
        binding.m_sName = ezStringView(reinterpret_cast<const char*>(&data[(ezUInt32)stream.GetReadPosition()]), uiStringElements);
        stream.SkipBytes(uiStringElements);

        stream >> binding.m_uiBinding;
        stream >> reinterpret_cast<ezUInt8&>(binding.m_Type);
        stream >> binding.m_uiDescriptorType;
        stream >> binding.m_uiDescriptorCount;
        stream >> binding.m_uiWordOffset;
      }
    }
  }
} // namespace ezSpirvMetaData
