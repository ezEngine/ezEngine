AssetConfigs
{
	Config %PC
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{4474367200212941373,1380388402243924385}}
				string %t{"ezTextureAssetTypePlatformConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{1024}
				}
			}
			o
			{
				Uuid %id{uint64{8085892115830203315,3737221074888337082}}
				string %t{"ezAssetPlatformConfig"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					string %Name{"PC"}
					string %Platform{"ezAssetTargetPlatform::PC"}
					VarArray %TypeConfigs
					{
						Uuid{uint64{4474367200212941373,1380388402243924385}}
					}
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
				string %t{"ezAssetPlatformConfig"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					string %Name{"PC-low"}
					string %Platform{"ezAssetTargetPlatform::PC"}
					VarArray %TypeConfigs
					{
						Uuid{uint64{15697183066669653192,10479972904091248807}}
					}
				}
			}
			o
			{
				Uuid %id{uint64{15697183066669653192,10479972904091248807}}
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
				Uuid %id{uint64{11592751623319371637,13121262766510098711}}
				string %t{"ezAssetPlatformConfig"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					string %Name{"Android"}
					string %Platform{"ezAssetTargetPlatform::Android"}
					VarArray %TypeConfigs
					{
						Uuid{uint64{3031709582232140993,17942870312578906426}}
					}
				}
			}
			o
			{
				Uuid %id{uint64{3031709582232140993,17942870312578906426}}
				string %t{"ezTextureAssetTypePlatformConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{32}
				}
			}
		}
	}
}
