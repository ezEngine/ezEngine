#include <ShaderCompilerDXC/ShaderCompilerDXC.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Memory/MemoryUtils.h>
#include <Foundation/Strings/StringConversion.h>

#include <spirv_reflect.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <d3dcompiler.h>
#endif

#include <dxc/dxcapi.h>

template <typename T>
struct ezComPtr
{
public:
  ezComPtr() {}
  ~ezComPtr()
  {
    if (m_ptr != nullptr)
    {
      m_ptr->Release();
      m_ptr = nullptr;
    }
  }

  ezComPtr(const ezComPtr& other)
    : m_ptr(other.m_ptr)
  {
    if (m_ptr)
    {
      m_ptr->AddRef();
    }
  }

  T* operator->() { return m_ptr; }
  T* const operator->() const { return m_ptr; }

  T** put()
  {
    EZ_ASSERT_DEV(m_ptr == nullptr, "Can only put into an empty ezComPtr");
    return &m_ptr;
  }

  bool operator==(nullptr_t)
  {
    return m_ptr == nullptr;
  }

  bool operator!=(nullptr_t)
  {
    return m_ptr != nullptr;
  }

private:
  T* m_ptr = nullptr;
};

ezComPtr<IDxcUtils> s_pDxcUtils;
ezComPtr<IDxcCompiler3> s_pDxcCompiler;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(ShaderCompilerDXC, ShaderCompilerDXCPlugin)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_pDxcUtils.put()));
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_pDxcCompiler.put()));
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pDxcUtils = {};
    s_pDxcCompiler = {};
  }

EZ_END_SUBSYSTEM_DECLARATION;

EZ_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(ezShaderCompilerDXC, 1)
EZ_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStringView ezShaderCompilerDXC::GetProfileName(ezStringView sPlatform, ezGALShaderStage::Enum Stage)
{
  switch (Stage)
  {
    case ezGALShaderStage::VertexShader:
      return "vs_6_0";
    case ezGALShaderStage::HullShader:
      return "hs_6_0";
    case ezGALShaderStage::DomainShader:
      return "ds_6_0";
    case ezGALShaderStage::GeometryShader:
      return "gs_6_0";
    case ezGALShaderStage::PixelShader:
      return "ps_6_0";
    case ezGALShaderStage::ComputeShader:
      return "cs_6_0";
    default:
      break;
  }

  EZ_REPORT_FAILURE("Unknown Platform '{}' or Stage {}", sPlatform, Stage);
  return "";
}

ezResult ezShaderCompilerDXC::Initialize()
{
  if (m_VertexInputMapping.IsEmpty())
  {
    m_VertexInputMapping["in.var.POSITION"] = ezGALVertexAttributeSemantic::Position;
    m_VertexInputMapping["in.var.NORMAL"] = ezGALVertexAttributeSemantic::Normal;
    m_VertexInputMapping["in.var.TANGENT"] = ezGALVertexAttributeSemantic::Tangent;

    m_VertexInputMapping["in.var.COLOR0"] = ezGALVertexAttributeSemantic::Color0;
    m_VertexInputMapping["in.var.COLOR1"] = ezGALVertexAttributeSemantic::Color1;
    m_VertexInputMapping["in.var.COLOR2"] = ezGALVertexAttributeSemantic::Color2;
    m_VertexInputMapping["in.var.COLOR3"] = ezGALVertexAttributeSemantic::Color3;
    m_VertexInputMapping["in.var.COLOR4"] = ezGALVertexAttributeSemantic::Color4;
    m_VertexInputMapping["in.var.COLOR5"] = ezGALVertexAttributeSemantic::Color5;
    m_VertexInputMapping["in.var.COLOR6"] = ezGALVertexAttributeSemantic::Color6;
    m_VertexInputMapping["in.var.COLOR7"] = ezGALVertexAttributeSemantic::Color7;

    m_VertexInputMapping["in.var.TEXCOORD0"] = ezGALVertexAttributeSemantic::TexCoord0;
    m_VertexInputMapping["in.var.TEXCOORD1"] = ezGALVertexAttributeSemantic::TexCoord1;
    m_VertexInputMapping["in.var.TEXCOORD2"] = ezGALVertexAttributeSemantic::TexCoord2;
    m_VertexInputMapping["in.var.TEXCOORD3"] = ezGALVertexAttributeSemantic::TexCoord3;
    m_VertexInputMapping["in.var.TEXCOORD4"] = ezGALVertexAttributeSemantic::TexCoord4;
    m_VertexInputMapping["in.var.TEXCOORD5"] = ezGALVertexAttributeSemantic::TexCoord5;
    m_VertexInputMapping["in.var.TEXCOORD6"] = ezGALVertexAttributeSemantic::TexCoord6;
    m_VertexInputMapping["in.var.TEXCOORD7"] = ezGALVertexAttributeSemantic::TexCoord7;
    m_VertexInputMapping["in.var.TEXCOORD8"] = ezGALVertexAttributeSemantic::TexCoord8;
    m_VertexInputMapping["in.var.TEXCOORD9"] = ezGALVertexAttributeSemantic::TexCoord9;

    m_VertexInputMapping["in.var.BITANGENT"] = ezGALVertexAttributeSemantic::BiTangent;
    m_VertexInputMapping["in.var.BONEINDICES0"] = ezGALVertexAttributeSemantic::BoneIndices0;
    m_VertexInputMapping["in.var.BONEINDICES1"] = ezGALVertexAttributeSemantic::BoneIndices1;
    m_VertexInputMapping["in.var.BONEWEIGHTS0"] = ezGALVertexAttributeSemantic::BoneWeights0;
    m_VertexInputMapping["in.var.BONEWEIGHTS1"] = ezGALVertexAttributeSemantic::BoneWeights1;
  }

  EZ_ASSERT_DEV(s_pDxcUtils != nullptr && s_pDxcCompiler != nullptr, "ShaderCompiler SubSystem init should have initialized library pointers.");
  return EZ_SUCCESS;
}

ezResult ezShaderCompilerDXC::Compile(ezShaderProgramData& inout_Data, ezLogInterface* pLog)
{
  EZ_SUCCEED_OR_RETURN(Initialize());

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_Data.m_uiSourceHash[stage] == 0)
      continue;
    if (inout_Data.m_bWriteToDisk[stage] == false)
    {
      ezLog::Debug("Shader for stage '{}' is already compiled.", ezGALShaderStage::Names[stage]);
      continue;
    }

    const ezStringBuilder sShaderSource = inout_Data.m_sShaderSource[stage];

    if (!sShaderSource.IsEmpty() && sShaderSource.FindSubString("main") != nullptr)
    {
      const ezStringBuilder sSourceFile = inout_Data.m_sSourceFile;

      if (CompileSPIRVShader(sSourceFile, sShaderSource, inout_Data.m_Flags.IsSet(ezShaderCompilerFlags::Debug), GetProfileName(inout_Data.m_sPlatform, (ezGALShaderStage::Enum)stage), "main", inout_Data.m_ByteCode[stage]->m_ByteCode).Succeeded())
      {
        EZ_SUCCEED_OR_RETURN(ReflectShaderStage(inout_Data, (ezGALShaderStage::Enum)stage));
      }
      else
      {
        return EZ_FAILURE;
      }
    }
  }

  return EZ_SUCCESS;
}

void ezShaderCompilerDXC::ConfigureDxcArgs(ezDynamicArray<ezStringWChar>& inout_Args)
{
  inout_Args.PushBack(L"-spirv");
  inout_Args.PushBack(L"-fvk-use-dx-position-w");
  inout_Args.PushBack(L"-fspv-target-env=vulkan1.1");
  // inout_Args.PushBack(L"-fvk-use-dx-layout");
}

ezResult ezShaderCompilerDXC::CompileSPIRVShader(ezStringView sFile, ezStringView sSource, bool bDebug, ezStringView sProfile, ezStringView sEntryPoint, ezDynamicArray<ezUInt8>& out_ByteCode)
{
  out_ByteCode.Clear();

  ezStringView sCompileSource = sSource;
  ezStringBuilder sDebugSource;

  ezDynamicArray<ezStringWChar> args;
  args.PushBack(ezStringWChar(sFile));
  args.PushBack(L"-E");
  args.PushBack(ezStringWChar(sEntryPoint));
  args.PushBack(L"-T");
  args.PushBack(ezStringWChar(sProfile));

  ConfigureDxcArgs(args);

  if (bDebug)
  {
    // In debug mode we need to remove '#line' as any shader debugger won't work with them.
    sDebugSource = sSource;
    sDebugSource.ReplaceAll("#line ", "//ine ");
    sCompileSource = sDebugSource;

    // ezLog::Warning("Vulkan DEBUG shader support not really implemented.");

    args.PushBack(L"-Zi"); // Enable debug information.
    // args.PushBack(L"-Fo"); // Optional. Stored in the pdb.
    // args.PushBack(L"myshader.bin");
    // args.PushBack(L"-Fd"); // The file name of the pdb.
    // args.PushBack(L"myshader.pdb");
  }

  ezComPtr<IDxcBlobEncoding> pSource;
  s_pDxcUtils->CreateBlob(sCompileSource.GetStartPointer(), sCompileSource.GetElementCount(), DXC_CP_UTF8, pSource.put());

  DxcBuffer Source;
  Source.Ptr = pSource->GetBufferPointer();
  Source.Size = pSource->GetBufferSize();
  Source.Encoding = DXC_CP_UTF8;

  ezHybridArray<LPCWSTR, 16> pszArgs;
  pszArgs.SetCount(args.GetCount());
  for (ezUInt32 i = 0; i < args.GetCount(); ++i)
  {
    pszArgs[i] = args[i].GetData();
  }

  ezComPtr<IDxcResult> pResults;
  s_pDxcCompiler->Compile(&Source, pszArgs.GetData(), pszArgs.GetCount(), nullptr, IID_PPV_ARGS(pResults.put()));

  ezComPtr<IDxcBlobUtf8> pErrors;
  pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(pErrors.put()), nullptr);

  HRESULT hrStatus;
  pResults->GetStatus(&hrStatus);
  if (FAILED(hrStatus))
  {
    ezLog::Error("SPIR-V shader compilation failed.");

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      ezLog::Error("{}", ezStringUtf8(pErrors->GetStringPointer()).GetData());
    }

    return EZ_FAILURE;
  }
  else
  {
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
      ezLog::Warning("{}", ezStringUtf8(pErrors->GetStringPointer()).GetData());
    }
  }

  ezComPtr<IDxcBlob> pShader;
  ezComPtr<IDxcBlobWide> pShaderName;
  pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(pShader.put()), pShaderName.put());

  if (pShader == nullptr)
  {
    ezLog::Error("No SPIR-V bytecode was generated.");
    return EZ_FAILURE;
  }

  out_ByteCode.SetCountUninitialized(static_cast<ezUInt32>(pShader->GetBufferSize()));

  ezMemoryUtils::Copy(out_ByteCode.GetData(), reinterpret_cast<ezUInt8*>(pShader->GetBufferPointer()), out_ByteCode.GetCount());

  return EZ_SUCCESS;
}

ezResult ezShaderCompilerDXC::ModifyShaderSource(ezShaderProgramData& inout_data, ezLogInterface* pLog)
{
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    ezShaderParser::ParseShaderResources(inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage]);
  }

  ezHashTable<ezHashedString, ezShaderResourceBinding> bindings;
  EZ_SUCCEED_OR_RETURN(ezShaderParser::MergeShaderResourceBindings(inout_data, bindings, pLog));
  EZ_SUCCEED_OR_RETURN(DefineShaderResourceBindings(inout_data, bindings, pLog));
  EZ_SUCCEED_OR_RETURN(ezShaderParser::SanityCheckShaderResourceBindings(bindings, pLog));

  // Apply shader resource bindings
  ezStringBuilder sNewShaderCode;
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (inout_data.m_sShaderSource[stage].IsEmpty())
      continue;
    ezShaderParser::ApplyShaderResourceBindings(inout_data.m_sPlatform, inout_data.m_sShaderSource[stage], inout_data.m_Resources[stage], bindings, ezMakeDelegate(&ezShaderCompilerDXC::CreateNewShaderResourceDeclaration, this), sNewShaderCode);
    inout_data.m_sShaderSource[stage] = sNewShaderCode;
  }
  return EZ_SUCCESS;
}

ezResult ezShaderCompilerDXC::DefineShaderResourceBindings(const ezShaderProgramData& data, ezHashTable<ezHashedString, ezShaderResourceBinding>& inout_resourceBinding, ezLogInterface* pLog)
{
  // Force material parameter into the material bind group
  for (const ezString& sMaterialParameter : data.m_MaterialParameters)
  {
    ezShaderResourceBinding* pBinding = nullptr;
    if (inout_resourceBinding.TryGetValue(ezTempHashedString(sMaterialParameter.GetView()), pBinding))
    {
      pBinding->m_iBindGroup = EZ_GAL_BIND_GROUP_MATERIAL;
    }
  }

  // Determine which indices are hard-coded in the shader already.
  ezHybridArray<ezHybridBitfield<64>, 4> slotInUseInSet;
  for (auto it : inout_resourceBinding)
  {
    ezInt16& iSet = it.Value().m_iBindGroup;
    if (iSet == -1)
      iSet = 0;

    slotInUseInSet.EnsureCount(iSet + 1);

    if (it.Value().m_iSlot != -1)
    {
      slotInUseInSet[iSet].SetCount(ezMath::Max(slotInUseInSet[iSet].GetCount(), static_cast<ezUInt32>(it.Value().m_iSlot + 1)));
      slotInUseInSet[iSet].SetBit(it.Value().m_iSlot);
    }
  }

  // Create stable oder of resources in each set.
  ezHybridArray<ezHybridArray<ezHashedString, 16>, 4> orderInSet;
  orderInSet.SetCount(slotInUseInSet.GetCount());
  for (ezUInt32 stage = ezGALShaderStage::VertexShader; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (data.m_sShaderSource[stage].IsEmpty())
      continue;

    for (const auto& res : data.m_Resources[stage])
    {
      const ezInt16 iSet = res.m_Binding.m_iBindGroup < 0 ? (ezInt16)0 : res.m_Binding.m_iBindGroup;
      if (!orderInSet[iSet].Contains(res.m_Binding.m_sName))
      {
        orderInSet[iSet].PushBack(res.m_Binding.m_sName);
      }
    }
  }

  // Do we have _AutoSampler in use? Combine them!
  struct TextureAndSamplerTuple
  {
    ezHashTable<ezHashedString, ezShaderResourceBinding>::Iterator itSampler;
    ezHashTable<ezHashedString, ezShaderResourceBinding>::Iterator itTexture;
  };
  ezHybridArray<TextureAndSamplerTuple, 2> autoSamplers;

  if (AllowCombinedImageSamplers())
  {
    for (auto itSampler : inout_resourceBinding)
    {
      if (itSampler.Value().m_ResourceType != ezGALShaderResourceType::Sampler || !itSampler.Key().GetView().EndsWith("_AutoSampler"))
        continue;

      ezStringBuilder sb = itSampler.Key().GetString();
      sb.TrimWordEnd("_AutoSampler");
      auto itTexture = inout_resourceBinding.Find(ezTempHashedString(sb));
      if (!itTexture.IsValid())
        continue;

      if (itSampler.Value().m_iBindGroup != itTexture.Value().m_iBindGroup || itSampler.Value().m_iSlot != itTexture.Value().m_iSlot)
        continue;

      itSampler.Value().m_ResourceType = ezGALShaderResourceType::TextureAndSampler;
      itTexture.Value().m_ResourceType = ezGALShaderResourceType::TextureAndSampler;
      // Sampler will match the slot of the texture at the end
      orderInSet[itSampler.Value().m_iBindGroup].RemoveAndCopy(itSampler.Key());
      autoSamplers.PushBack({itSampler, itTexture});
    }
  }

  // Assign slot to each resource in each set.
  for (ezInt16 iSet = 0; iSet < (ezInt16)slotInUseInSet.GetCount(); ++iSet)
  {
    ezUInt32 uiCurrentSlot = 0;
    for (const auto& sName : orderInSet[iSet])
    {
      ezInt16& iSlot = inout_resourceBinding[sName].m_iSlot;
      if (iSlot != -1)
        continue;
      while (uiCurrentSlot < slotInUseInSet[iSet].GetCount() && slotInUseInSet[iSet].IsBitSet(uiCurrentSlot))
      {
        uiCurrentSlot++;
      }
      iSlot = static_cast<ezInt16>(uiCurrentSlot);
      slotInUseInSet[iSet].SetCount(ezMath::Max(slotInUseInSet[iSet].GetCount(), uiCurrentSlot + 1));
      slotInUseInSet[iSet].SetBit(uiCurrentSlot);
    }
  }

  // Copy texture assignments to the samplers.
  for (TextureAndSamplerTuple& tas : autoSamplers)
  {
    tas.itSampler.Value().m_iSlot = tas.itTexture.Value().m_iSlot;
  }
  return EZ_SUCCESS;
}

void ezShaderCompilerDXC::CreateNewShaderResourceDeclaration(ezStringView sPlatform, ezStringView sDeclaration, const ezShaderResourceBinding& binding, ezStringBuilder& out_sDeclaration)
{
  ezBitflags<ezGALShaderResourceCategory> type = ezGALShaderResourceCategory::MakeFromShaderDescriptorType(binding.m_ResourceType);
  ezStringView sResourcePrefix;

  // The only descriptor that can have more than one shader resource type is TextureAndSampler.
  // There will be two declarations in the HLSL code, the sampler and the texture.
  if (binding.m_ResourceType == ezGALShaderResourceType::TextureAndSampler)
  {
    type = binding.m_TextureType == ezGALShaderTextureType::Unknown ? ezGALShaderResourceCategory::Sampler : ezGALShaderResourceCategory::TextureSRV;
  }

  switch (type.GetValue())
  {
    case ezGALShaderResourceCategory::Sampler:
      sResourcePrefix = "s"_ezsv;
      break;
    case ezGALShaderResourceCategory::ConstantBuffer:
      sResourcePrefix = "b"_ezsv;
      break;
    case ezGALShaderResourceCategory::TextureSRV:
    case ezGALShaderResourceCategory::BufferSRV:
      sResourcePrefix = "t"_ezsv;
      break;
    case ezGALShaderResourceCategory::TextureUAV:
    case ezGALShaderResourceCategory::BufferUAV:
      sResourcePrefix = "u"_ezsv;
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  if (binding.m_ResourceType == ezGALShaderResourceType::TextureAndSampler)
  {
    out_sDeclaration.SetFormat("[[vk::combinedImageSampler]] {} : register({}{}, space{})", sDeclaration, sResourcePrefix, binding.m_iSlot, binding.m_iBindGroup);
  }
  else
  {
    out_sDeclaration.SetFormat("{} : register({}{}, space{})", sDeclaration, sResourcePrefix, binding.m_iSlot, binding.m_iBindGroup);
  }
}

ezResult ezShaderCompilerDXC::FillResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SRV)
  {
    return FillSRVResourceBinding(binding, info);
  }
  else if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_UAV)
  {
    return FillUAVResourceBinding(binding, info);
  }
  else if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_CBV)
  {
    binding.m_ResourceType = ezGALShaderResourceType::ConstantBuffer;
    binding.m_pLayout = ReflectConstantBufferLayout(info.name, info.block);

    return EZ_SUCCESS;
  }
  else if (info.resource_type & SpvReflectResourceType::SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
  {
    binding.m_ResourceType = ezGALShaderResourceType::Sampler;

    if (AllowCombinedImageSamplers())
    {
      if (binding.m_sName.GetString().EndsWith("_AutoSampler"))
      {
        ezStringBuilder sb = binding.m_sName.GetString();
        sb.TrimWordEnd("_AutoSampler");
        binding.m_sName.Assign(sb);
      }
    }

    return EZ_SUCCESS;
  }

  ezLog::Error("Resource '{}': Unsupported resource type.", info.name);
  return EZ_FAILURE;
}

ezGALShaderTextureType::Enum ezShaderCompilerDXC::GetTextureType(const SpvReflectDescriptorBinding& info)
{
  switch (info.image.dim)
  {
    case SpvDim::SpvDim1D:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed > 0)
        {
          return ezGALShaderTextureType::Texture1DArray;
        }
        else
        {
          return ezGALShaderTextureType::Texture1D;
        }
      }

      break;
    }

    case SpvDim::SpvDim2D:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed > 0)
        {
          return ezGALShaderTextureType::Texture2DArray;
        }
        else
        {
          return ezGALShaderTextureType::Texture2D;
        }
      }
      else
      {
        if (info.image.arrayed > 0)
        {
          return ezGALShaderTextureType::Texture2DMSArray;
        }
        else
        {
          return ezGALShaderTextureType::Texture2DMS;
        }
      }

      break;
    }

    case SpvDim::SpvDim3D:
    {
      if (info.image.ms == 0 && info.image.arrayed == 0)
      {
        return ezGALShaderTextureType::Texture3D;
      }

      break;
    }

    case SpvDim::SpvDimCube:
    {
      if (info.image.ms == 0)
      {
        if (info.image.arrayed == 0)
        {
          return ezGALShaderTextureType::TextureCube;
        }
        else
        {
          return ezGALShaderTextureType::TextureCubeArray;
        }
      }

      break;
    }

    case SpvDim::SpvDimBuffer:
      EZ_ASSERT_NOT_IMPLEMENTED;
      // binding.m_TextureType = ezGALShaderTextureType::GenericBuffer;
      return ezGALShaderTextureType::Unknown;

    case SpvDim::SpvDimRect:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return ezGALShaderTextureType::Unknown;

    case SpvDim::SpvDimSubpassData:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return ezGALShaderTextureType::Unknown;

    case SpvDim::SpvDimMax:
      EZ_ASSERT_DEV(false, "Invalid enum value");
      break;

    default:
      break;
  }
  return ezGALShaderTextureType::Unknown;
}

ezResult ezShaderCompilerDXC::FillSRVResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct && ezStringUtils::StartsWith(info.type_description->type_name, "type.StructuredBuffer"))
    {
      binding.m_pLayout = ReflectStructuredBufferLayout(info.name, info.block);
      binding.m_ResourceType = ezGALShaderResourceType::StructuredBuffer;
      return EZ_SUCCESS;
    }
    else if (info.type_description->op == SpvOp::SpvOpTypeStruct && ezStringUtils::StartsWith(info.type_description->type_name, "type.ByteAddressBuffer"))
    {
      binding.m_ResourceType = ezGALShaderResourceType::ByteAddressBuffer;
      return EZ_SUCCESS;
    }
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
  {
    binding.m_ResourceType = ezGALShaderResourceType::Texture;
    binding.m_TextureType = GetTextureType(info);
    return EZ_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
  {
    EZ_ASSERT_DEV(!binding.m_sName.GetString().EndsWith("_AutoSampler"), "Combined image sampler should have taken the name from the image part");
    binding.m_ResourceType = ezGALShaderResourceType::TextureAndSampler;
    binding.m_TextureType = GetTextureType(info);
    return EZ_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_ResourceType = ezGALShaderResourceType::TexelBuffer;
      return EZ_SUCCESS;
    }

    ezLog::Error("Resource '{}': Unsupported texel buffer SRV type.", info.name);
    return EZ_FAILURE;
  }

  ezLog::Error("Resource '{}': Unsupported SRV type.", info.name);
  return EZ_FAILURE;
}

ezResult ezShaderCompilerDXC::FillUAVResourceBinding(ezShaderResourceBinding& binding, const SpvReflectDescriptorBinding& info)
{
  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
  {
    binding.m_ResourceType = ezGALShaderResourceType::TextureRW;
    binding.m_TextureType = GetTextureType(info);
    return EZ_SUCCESS;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
  {
    if (info.image.dim == SpvDim::SpvDimBuffer)
    {
      binding.m_ResourceType = ezGALShaderResourceType::TexelBufferRW;
      return EZ_SUCCESS;
    }

    ezLog::Error("Resource '{}': Unsupported texel buffer UAV type.", info.name);
    return EZ_FAILURE;
  }

  if (info.descriptor_type == SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER)
  {
    if (info.type_description->op == SpvOp::SpvOpTypeStruct && ezStringUtils::StartsWith(info.type_description->type_name, "type.RWStructuredBuffer"))
    {
      binding.m_pLayout = ReflectStructuredBufferLayout(info.name, info.block);
      binding.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;
      return EZ_SUCCESS;
    }
    else if (info.type_description->op == SpvOp::SpvOpTypeStruct && ezStringUtils::StartsWith(info.type_description->type_name, "type.RWByteAddressBuffer"))
    {
      binding.m_ResourceType = ezGALShaderResourceType::ByteAddressBufferRW;
      return EZ_SUCCESS;
    }
    else if (info.type_description->op == SpvOp::SpvOpTypeStruct && ezStringUtils::StartsWith(info.type_description->type_name, "type.AppendStructuredBuffer"))
    {
      // #TODO_VULKAN AppendStructuredBuffer support
      binding.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;
      return EZ_SUCCESS;
    }
    else if (info.type_description->op == SpvOp::SpvOpTypeStruct && ezStringUtils::StartsWith(info.type_description->type_name, "type.ConsumeStructuredBuffer"))
    {
      // #TODO_VULKAN ConsumeStructuredBuffer support
      binding.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;
      return EZ_SUCCESS;
    }
    else if (info.image.dim == SpvDim::SpvDim1D)
    {
      // #TODO_VULKAN AppendStructuredBuffer / ConsumeStructuredBuffer counter support
      binding.m_ResourceType = ezGALShaderResourceType::StructuredBufferRW;
      return EZ_SUCCESS;
    }
  }

  ezLog::Error("Resource '{}': Unsupported UAV type.", info.name);
  return EZ_FAILURE;
}

ezGALResourceFormat::Enum GetEZFormat(SpvReflectFormat format)
{
  switch (format)
  {
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_UINT:
      return ezGALResourceFormat::RUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SINT:
      return ezGALResourceFormat::RInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32_SFLOAT:
      return ezGALResourceFormat::RFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_UINT:
      return ezGALResourceFormat::RGUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SINT:
      return ezGALResourceFormat::RGInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32_SFLOAT:
      return ezGALResourceFormat::RGFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_UINT:
      return ezGALResourceFormat::RGBUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SINT:
      return ezGALResourceFormat::RGBInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
      return ezGALResourceFormat::RGBFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
      return ezGALResourceFormat::RGBAUInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
      return ezGALResourceFormat::RGBAInt;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
      return ezGALResourceFormat::RGBAFloat;
    case SpvReflectFormat::SPV_REFLECT_FORMAT_UNDEFINED:
    default:
      return ezGALResourceFormat::Invalid;
  }
}

ezResult ezShaderCompilerDXC::ReflectShaderStage(ezShaderProgramData& inout_Data, ezGALShaderStage::Enum Stage)
{
  EZ_LOG_BLOCK("ReflectShaderStage", inout_Data.m_sSourceFile);

  ezGALShaderByteCode* pShader = inout_Data.m_ByteCode[Stage];
  const auto& bytecode = pShader->m_ByteCode;

  SpvReflectShaderModule module;

  if (spvReflectCreateShaderModule(bytecode.GetCount(), bytecode.GetData(), &module) != SPV_REFLECT_RESULT_SUCCESS)
  {
    ezLog::Error("Extracting shader reflection information failed.");
    return EZ_FAILURE;
  }

  EZ_SCOPE_EXIT(spvReflectDestroyShaderModule(&module));

  //
  auto& vertexInputAttributes = pShader->m_ShaderVertexInput;
  if (Stage == ezGALShaderStage::VertexShader)
  {
    ezUInt32 uiNumVars = 0;
    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve number of input variables.");
      return EZ_FAILURE;
    }
    ezDynamicArray<SpvReflectInterfaceVariable*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateInputVariables(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve input variables.");
      return EZ_FAILURE;
    }

    vertexInputAttributes.Reserve(vars.GetCount());

    for (ezUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      SpvReflectInterfaceVariable* pVar = vars[i];
      if (pVar->name != nullptr)
      {
        ezShaderVertexInputAttribute& attr = vertexInputAttributes.ExpandAndGetRef();
        attr.m_uiLocation = static_cast<ezUInt8>(pVar->location);

        ezGALVertexAttributeSemantic::Enum* pVAS = m_VertexInputMapping.GetValue(pVar->name);
        EZ_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input sematic found: {}", pVar->name);
        attr.m_eSemantic = *pVAS;
        attr.m_eFormat = GetEZFormat(pVar->format);
        EZ_ASSERT_DEV(pVAS != nullptr, "Unknown vertex input format found: {}", pVar->format);
      }
    }
  }
  else if (Stage == ezGALShaderStage::HullShader)
  {
    pShader->m_uiTessellationPatchControlPoints = module.entry_points[0].output_vertices;
  }

  // descriptor bindings
  {
    ezUInt32 uiNumVars = 0;
    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve number of descriptor bindings.");
      return EZ_FAILURE;
    }

    ezDynamicArray<SpvReflectDescriptorBinding*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumerateDescriptorBindings(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve descriptor bindings.");
      return EZ_FAILURE;
    }

    for (ezUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& info = *vars[i];

      ezLog::Info("Bound Resource: '{}' at slot {} (Count: {})", info.name, info.binding, info.count);

      ezShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_iBindGroup = static_cast<ezInt16>(info.set);
      shaderResourceBinding.m_iSlot = static_cast<ezInt16>(info.binding);
      shaderResourceBinding.m_uiArraySize = info.count;
      shaderResourceBinding.m_sName.Assign(info.name);
      shaderResourceBinding.m_Stages = ezGALShaderStageFlags::MakeFromShaderStage(Stage);

      if (FillResourceBinding(shaderResourceBinding, info).Failed())
        continue;

      EZ_ASSERT_DEV(shaderResourceBinding.m_ResourceType != ezGALShaderResourceType::Unknown, "FillResourceBinding should have failed.");

      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }

  // Push Constants
  {
    ezUInt32 uiNumVars = 0;
    if (spvReflectEnumeratePushConstantBlocks(&module, &uiNumVars, nullptr) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve number of descriptor bindings.");
      return EZ_FAILURE;
    }

    if (uiNumVars > 1)
    {
      ezLog::Error("Only one push constant block is supported right now.");
      return EZ_FAILURE;
    }

    ezDynamicArray<SpvReflectBlockVariable*> vars;
    vars.SetCount(uiNumVars);

    if (spvReflectEnumeratePushConstantBlocks(&module, &uiNumVars, vars.GetData()) != SPV_REFLECT_RESULT_SUCCESS)
    {
      ezLog::Error("Failed to retrieve descriptor bindings.");
      return EZ_FAILURE;
    }

    for (ezUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& info = *vars[i];

      ezStringBuilder sName = info.name;
      sName.TrimWordStart("type.PushConstant.");
      sName.TrimWordEnd("_PushConstants");

      ezLog::Info("Push Constants: '{}', Offset: {}, Size: {}", sName, info.offset, info.padded_size);

      if (info.offset != 0)
      {
        ezLog::Error("The push constant block '{}' has an offset of '{}', only a zero offset is supported right now. This should be the case if only one block exists", sName, info.offset);
        return EZ_FAILURE;
      }

      ezShaderResourceBinding shaderResourceBinding;
      shaderResourceBinding.m_ResourceType = ezGALShaderResourceType::PushConstants;
      shaderResourceBinding.m_iBindGroup = -1;
      shaderResourceBinding.m_iSlot = -1;
      shaderResourceBinding.m_uiArraySize = 1;

      shaderResourceBinding.m_sName.Assign(sName);
      shaderResourceBinding.m_Stages = ezGALShaderStageFlags::MakeFromShaderStage(Stage);
      shaderResourceBinding.m_pLayout = ReflectConstantBufferLayout(info.name, info);
      inout_Data.m_ByteCode[Stage]->m_ShaderResourceBindings.PushBack(shaderResourceBinding);
    }
  }

  return EZ_SUCCESS;
}

ezSharedPtr<ezShaderConstantBufferLayout> ezShaderCompilerDXC::ReflectStructuredBufferLayout(ezStringView sName, const SpvReflectBlockVariable& block)
{
  EZ_LOG_BLOCK("Structured Buffer Layout", sName);
  SpvReflectBlockVariable& innerBlock = block.members[0];
  ezLog::Debug("Structured Buffer has {} variables, Size is {}", innerBlock.member_count, innerBlock.padded_size);
  return ReflectBufferLayout(innerBlock);
}

ezSharedPtr<ezShaderConstantBufferLayout> ezShaderCompilerDXC::ReflectConstantBufferLayout(ezStringView sName, const SpvReflectBlockVariable& block)
{
  EZ_LOG_BLOCK("Constant Buffer Layout", sName);
  ezLog::Debug("Constant Buffer has {} variables, Size is {}", block.member_count, block.padded_size);
  return ReflectBufferLayout(block);
}

ezSharedPtr<ezShaderConstantBufferLayout> ezShaderCompilerDXC::ReflectBufferLayout(const SpvReflectBlockVariable& block)
{
  ezSharedPtr<ezShaderConstantBufferLayout> pLayout = EZ_DEFAULT_NEW(ezShaderConstantBufferLayout);

  pLayout->m_uiTotalSize = block.padded_size;

  for (ezUInt32 var = 0; var < block.member_count; ++var)
  {
    const auto& svd = block.members[var];

    ezShaderConstant constant;
    constant.m_sName.Assign(svd.name);
    constant.m_uiOffset = svd.offset; // TODO: or svd.absolute_offset ??
    constant.m_uiArrayElements = 1;

    ezUInt32 uiFlags = svd.type_description->type_flags;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_ARRAY;

      if (svd.array.dims_count != 1)
      {
        ezLog::Error("Variable '{}': Multi-dimensional arrays are not supported.", constant.m_sName);
        continue;
      }

      constant.m_uiArrayElements = svd.array.dims[0];
    }

    ezUInt32 uiComponents = 0;

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR;

      uiComponents = svd.numeric.vector.component_count;
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_BOOL;

      // TODO: unfortunately this never seems to be set, 'bool' types are always exposed as 'int'
      EZ_ASSERT_NOT_IMPLEMENTED;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = ezShaderConstant::Type::Bool;
          break;

        default:
          ezLog::Error("Variable '{}': Multi-component bools are not supported.", constant.m_sName);
          continue;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_INT;

      const bool bSigned = svd.type_description->traits.numeric.scalar.signedness != 0;
      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = bSigned ? ezShaderConstant::Type::Int1 : ezShaderConstant::Type::UInt1;
          break;
        case 2:
          constant.m_Type = bSigned ? ezShaderConstant::Type::Int2 : ezShaderConstant::Type::UInt2;
          break;
        case 3:
          constant.m_Type = bSigned ? ezShaderConstant::Type::Int3 : ezShaderConstant::Type::UInt3;
          break;
        case 4:
          constant.m_Type = bSigned ? ezShaderConstant::Type::Int4 : ezShaderConstant::Type::UInt4;
          break;
      }
    }
    else if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;

      switch (uiComponents)
      {
        case 0:
        case 1:
          constant.m_Type = ezShaderConstant::Type::Float1;
          break;
        case 2:
          constant.m_Type = ezShaderConstant::Type::Float2;
          break;
        case 3:
          constant.m_Type = ezShaderConstant::Type::Float3;
          break;
        case 4:
          constant.m_Type = ezShaderConstant::Type::Float4;
          break;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX;

      constant.m_Type = ezShaderConstant::Type::Default;

      const ezUInt32 rows = svd.type_description->traits.numeric.matrix.row_count;
      const ezUInt32 columns = svd.type_description->traits.numeric.matrix.column_count;

      if ((svd.type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT) == 0)
      {
        ezLog::Error("Variable '{}': Only float matrices are supported", constant.m_sName);
        continue;
      }

      if (columns == 3 && rows == 3)
      {
        constant.m_Type = ezShaderConstant::Type::Mat3x3;
      }
      else if (columns == 4 && rows == 4)
      {
        constant.m_Type = ezShaderConstant::Type::Mat4x4;
      }
      else
      {
        ezLog::Error("Variable '{}': {}x{} matrices are not supported", constant.m_sName, rows, columns);
        continue;
      }
    }

    if (uiFlags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT)
    {
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_STRUCT;
      uiFlags &= ~SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK;

      if (svd.size == 48 && svd.member_count == 3 && "r0"_ezsv == svd.members[0].name && "r1"_ezsv == svd.members[1].name && "r2"_ezsv == svd.members[2].name)
      {
        constant.m_Type = ezShaderConstant::Type::Transform;
      }
      else
      {
        constant.m_Type = ezShaderConstant::Type::Struct;
      }
    }

    if (uiFlags != 0)
    {
      ezLog::Error("Variable '{}': Unknown additional type flags '{}'", constant.m_sName, uiFlags);
    }

    if (constant.m_Type == ezShaderConstant::Type::Default)
    {
      ezLog::Error("Variable '{}': Variable type is unknown / not supported", constant.m_sName);
      continue;
    }

    const char* typeNames[] = {
      "Default",
      "Float1",
      "Float2",
      "Float3",
      "Float4",
      "Int1",
      "Int2",
      "Int3",
      "Int4",
      "UInt1",
      "UInt2",
      "UInt3",
      "UInt4",
      "Mat3x3",
      "Mat4x4",
      "Transform",
      "Bool",
      "Struct",
    };

    if (constant.m_uiArrayElements > 1)
    {
      ezLog::Debug("{1} {3}[{2}] {0}", constant.m_sName, ezArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }
    else
    {
      ezLog::Debug("{1} {3} {0}", constant.m_sName, ezArgU(constant.m_uiOffset, 3, true), constant.m_uiArrayElements, typeNames[constant.m_Type]);
    }

    pLayout->m_Constants.PushBack(constant);
  }

  return pLayout;
}
