#include "../Weapon.as"
#include "../../../Scripts/GameDecls.as"

class Shotgun : WeaponBaseClass
{
    void OnSimulationStarted()
    {
        WeaponBaseClass::OnSimulationStarted();
        
        singleShotPerTrigger = true;
    }

    void FireWeapon(MsgWeaponInteraction@ msg) override
    {
        ezSpawnComponent@ spawn;
        if (!GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(@spawn))
            return;

        if (!spawn.CanTriggerManualSpawn())
            return;

        msg.weaponInfo.iAmmoInClip -= 1;

        ezRandom@ rng = GetWorld().GetRandomNumberGenerator();
        
        for (int i = 0; i < 16; ++i) 
        {
            spawn.TriggerManualSpawn(true, ezVec3(rng.DoubleMinMax(-0.05, 0.05), 0, 0));
        }

        PlayShootSound();
    }
}