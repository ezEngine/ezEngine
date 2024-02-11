AssetProfiles
{
	Config %Default
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{10386675348308922682,207938457348376428}}
				string %t{"ezCoreRenderProfileConfig"}
				uint32 %v{1}
				p
				{
					uint32 %MaxShadowMapSize{1024}
					uint32 %MinShadowMapSize{64}
					uint32 %ShadowAtlasTextureSize{4096}
				}
			}
			o
			{
				Uuid %id{uint64{385040311378845408,1178138948935131612}}
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
				Uuid %id{uint64{6024007684197752254,9388485812360408817}}
				string %t{"ezTextureAssetProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{16384}
				}
			}
			o
			{
				Uuid %id{uint64{3821783988184046669,15370839493438779908}}
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
						Uuid{uint64{3821783988184046669,15370839493438779908}}
					}
					string %Name{"Default"}
					string %TargetPlatform{"Windows"}
				}
			}
		}
	}
}
