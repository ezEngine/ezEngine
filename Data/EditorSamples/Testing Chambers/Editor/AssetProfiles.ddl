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
					}
					string %Name{"PC"}
					string %Platform{"ezAssetProfileTargetPlatform::PC"}
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
					}
					string %Name{"PC-low"}
					string %Platform{"ezAssetProfileTargetPlatform::PC"}
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
					uint16 %MaxResolution{32}
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
					}
					string %Name{"Android"}
					string %Platform{"ezAssetProfileTargetPlatform::Android"}
				}
			}
		}
	}
}
