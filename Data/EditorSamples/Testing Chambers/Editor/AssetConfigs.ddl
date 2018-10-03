AssetConfigs
{
	Config %PC
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{12157718174399944855,5431803853952683895}}
				string %t{"ezAssetPlatformConfig"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					string %Name{"PC"}
					string %Platform{"ezAssetTargetPlatform::PC"}
					VarArray %TypeConfigs
					{
						Uuid{uint64{6581310130314437510,15319647666873697931}}
					}
				}
			}
			o
			{
				Uuid %id{uint64{6581310130314437510,15319647666873697931}}
				string %t{"ezTextureAssetTypePlatformConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{1024}
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
				Uuid %id{uint64{17806600159751213757,4907556547988560208}}
				string %t{"ezAssetPlatformConfig"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					string %Name{"PC-low"}
					string %Platform{"ezAssetTargetPlatform::PC"}
					VarArray %TypeConfigs
					{
						Uuid{uint64{8635165583106314606,11603576189606357185}}
					}
				}
			}
			o
			{
				Uuid %id{uint64{8635165583106314606,11603576189606357185}}
				string %t{"ezTextureAssetTypePlatformConfig"}
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
				Uuid %id{uint64{17051294142081470596,5566730074197792481}}
				string %t{"ezAssetPlatformConfig"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					string %Name{"Android"}
					string %Platform{"ezAssetTargetPlatform::Android"}
					VarArray %TypeConfigs
					{
						Uuid{uint64{15649052774257250053,12371691208087853481}}
					}
				}
			}
			o
			{
				Uuid %id{uint64{15649052774257250053,12371691208087853481}}
				string %t{"ezTextureAssetTypePlatformConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{512}
				}
			}
		}
	}
}
