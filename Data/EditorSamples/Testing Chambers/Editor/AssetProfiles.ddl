AssetProfiles
{
	Config %PC
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{1809191874792131268,3388666585094239933}}
				string %t{"ezTextureAssetTypeProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{16384}
				}
			}
			o
			{
				Uuid %id{uint64{8085892115830203315,3737221074888337082}}
				string %t{"ezAssetProfile"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					VarArray %AssetTypeConfigs
					{
						Uuid{uint64{1809191874792131268,3388666585094239933}}
						Uuid{uint64{7329234101589370702,5347101007087857899}}
					}
					string %Name{"PC"}
					string %Platform{"ezAssetProfileTargetPlatform::PC"}
				}
			}
			o
			{
				Uuid %id{uint64{7329234101589370702,5347101007087857899}}
				string %t{"ezProjectPipelineProfileConfig"}
				uint32 %v{1}
				p
				{
					VarDict %CameraPipelines
					{
						string %Default{"{ c5f16f65-8940-4283-9f91-b316dfa39e81 }"}
					}
					string %DebugRenderPipeline{"{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }"}
					string %EditorRenderPipeline{"{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"}
					string %MainRenderPipeline{"{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"}
					string %ShadowMapRenderPipeline{"{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"}
				}
			}
		}
	}
	Config %'PC-low'
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{12305719006274145892,347692143459556382}}
				string %t{"ezAssetProfile"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					VarArray %AssetTypeConfigs
					{
						Uuid{uint64{41227172261279351,10218601351483156478}}
						Uuid{uint64{795944941373150882,7876099642035375980}}
					}
					string %Name{"PC-low"}
					string %Platform{"ezAssetProfileTargetPlatform::PC"}
				}
			}
			o
			{
				Uuid %id{uint64{795944941373150882,7876099642035375980}}
				string %t{"ezProjectPipelineProfileConfig"}
				uint32 %v{1}
				p
				{
					VarDict %CameraPipelines{}
					string %DebugRenderPipeline{"{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }"}
					string %EditorRenderPipeline{"{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"}
					string %MainRenderPipeline{"{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"}
					string %ShadowMapRenderPipeline{"{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"}
				}
			}
			o
			{
				Uuid %id{uint64{41227172261279351,10218601351483156478}}
				string %t{"ezTextureAssetTypeProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{8}
				}
			}
		}
	}
	Config %Android
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{7943288890496316512,5041841321163285511}}
				string %t{"ezTextureAssetTypeProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{16384}
				}
			}
			o
			{
				Uuid %id{uint64{9723688914223631,7573764490394580917}}
				string %t{"ezProjectPipelineProfileConfig"}
				uint32 %v{1}
				p
				{
					VarDict %CameraPipelines{}
					string %DebugRenderPipeline{"{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }"}
					string %EditorRenderPipeline{"{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"}
					string %MainRenderPipeline{"{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"}
					string %ShadowMapRenderPipeline{"{ 4f4d9f16-3d47-4c67-b821-a778f11dcaf5 }"}
				}
			}
			o
			{
				Uuid %id{uint64{11592751623319371637,13121262766510098711}}
				string %t{"ezAssetProfile"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					VarArray %AssetTypeConfigs
					{
						Uuid{uint64{7943288890496316512,5041841321163285511}}
						Uuid{uint64{9723688914223631,7573764490394580917}}
					}
					string %Name{"Android"}
					string %Platform{"ezAssetProfileTargetPlatform::Android"}
				}
			}
		}
	}
}
