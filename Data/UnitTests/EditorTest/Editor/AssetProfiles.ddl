AssetProfiles
{
	Config %Default
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{10386675348308922682,207938457348376428}}
				string %t{"ezRenderPipelineProfileConfig"}
				uint32 %v{1}
				p
				{
					VarDict %CameraPipelines{}
					string %MainRenderPipeline{"{ c533e113-2a4c-4f42-a546-653c78f5e8a7 }"}
				}
			}
			o
			{
				Uuid %id{uint64{385040311378845408,1178138948935131612}}
				string %t{"ezTextureAssetProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{16384}
				}
			}
			o
			{
				Uuid %id{uint64{6024007684197752254,9388485812360408817}}
				string %t{"ezXRConfig"}
				uint32 %v{2}
				p
				{
					bool %EnableXR{false}
					string %XRRenderPipeline{"{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"}
				}
			}
			o
			{
				Uuid %id{uint64{8341519292606584866,16089769571062246001}}
				string %t{"ezPlatformProfile"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					VarArray %Configs
					{
						Uuid{uint64{10386675348308922682,207938457348376428}}
						Uuid{uint64{385040311378845408,1178138948935131612}}
						Uuid{uint64{6024007684197752254,9388485812360408817}}
					}
					string %Name{"Default"}
					string %TargetPlatform{"Windows"}
				}
			}
		}
	}
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
						Uuid{uint64{10688866045028468411,17021851764261564183}}
					}
					string %Name{"PC"}
					string %TargetPlatform{"Windows"}
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
			o
			{
				Uuid %id{uint64{10688866045028468411,17021851764261564183}}
				string %t{"ezXRConfig"}
				uint32 %v{2}
				p
				{
					bool %EnableXR{false}
					string %XRRenderPipeline{"{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"}
				}
			}
		}
	}
}
