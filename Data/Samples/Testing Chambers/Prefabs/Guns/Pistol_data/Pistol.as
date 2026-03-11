#include "../Weapon.as"
#include "../../../Scripts/GameDecls.as"

class Pistol : WeaponBaseClass
{
    private ezTime nextAmmoPlus1Time;

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

        ezClock@ clk = GetWorld().GetClock();
        nextAmmoPlus1Time = clk.GetAccumulatedTime() + ezTime::Seconds(0.75);
        msg.weaponInfo.iAmmoInClip -= 1;

        spawn.TriggerManualSpawn(false, ezVec3::MakeZero());

        PlayShootSound();
    }

    void UpdateWeapon(MsgWeaponInteraction@ msg) override
    {
        ezClock@ clk = GetWorld().GetClock();
        if (nextAmmoPlus1Time <= clk.GetAccumulatedTime())
        {
            nextAmmoPlus1Time = clk.GetAccumulatedTime() + ezTime::Seconds(0.75);

            msg.weaponInfo.iAmmoInClip = ezMath::Min(msg.weaponInfo.iAmmoInClip + 1, msg.weaponInfo.iClipSize);
        }
    }
}