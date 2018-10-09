AssetProfiles
{
	Config %PC
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{8085892115830203315,3737221074888337082}}
				string %t{"ezPlatformProfile"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					VarArray %Configs
					{
						Uuid{uint64{11754538365307859884,8572204067211736280}}
						Uuid{uint64{18165925277888737543,17003377905246685588}}
					}
					string %Name{"PC"}
					string %Platform{"ezProfileTargetPlatform::PC"}
				}
			}
			o
			{
				Uuid %id{uint64{11754538365307859884,8572204067211736280}}
				string %t{"ezRenderPipelineProfileConfig"}
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
				Uuid %id{uint64{18165925277888737543,17003377905246685588}}
				string %t{"ezTextureAssetProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{16384}
				}
			}
		}
	}
	Config %PC_LOW
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{14410272918230655588,531936436298327082}}
				string %t{"ezRenderPipelineProfileConfig"}
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
				Uuid %id{uint64{1290659554215405138,3907313149868256798}}
				string %t{"ezPlatformProfile"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					VarArray %Configs
					{
						Uuid{uint64{14410272918230655588,531936436298327082}}
						Uuid{uint64{8663581586575762216,14337410329254690676}}
					}
					string %Name{"PC_LOW"}
					string %Platform{"ezProfileTargetPlatform::PC"}
				}
			}
			o
			{
				Uuid %id{uint64{8663581586575762216,14337410329254690676}}
				string %t{"ezTextureAssetProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{8}
				}
			}
		}
	}
}
