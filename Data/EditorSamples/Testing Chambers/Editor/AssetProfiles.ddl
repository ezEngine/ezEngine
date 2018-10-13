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
					VarDict %CameraPipelines
					{
						string %'Security Camera'{"{ c5f16f65-8940-4283-9f91-b316dfa39e81 }"}
					}
					string %MainRenderPipeline{"{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"}
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
					VarDict %CameraPipelines
					{
						string %'Security Camera'{"{ c5f16f65-8940-4283-9f91-b316dfa39e81 }"}
					}
					string %MainRenderPipeline{"{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"}
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
