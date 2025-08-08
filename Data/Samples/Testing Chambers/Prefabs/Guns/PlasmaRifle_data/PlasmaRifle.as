#include "../Weapon.as"
#include "../../../Scripts/GameDecls.as"

class PlasmaRifle : WeaponBaseClass
{
    void OnSimulationStarted()
    {
        WeaponBaseClass::OnSimulationStarted();
        
        singleShotPerTrigger = false;
    }

    void FireWeapon(MsgWeaponInteraction@ msg) override
    {
        ezSpawnComponent@ spawn;
        if (!GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(@spawn))
            return;

        if (!spawn.CanTriggerManualSpawn())
            return;

        msg.weaponInfo.iAmmoInClip -= 1;

        spawn.TriggerManualSpawn(true, ezVec3::MakeZero());

        PlayShootSound();
    }
}