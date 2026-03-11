#include "../../Scripts/GameDecls.as"

abstract class WeaponBaseClass : ezAngelScriptClass
{
    protected bool singleShotPerTrigger = false;
    private bool requireSingleShotReset = false;

    void OnSimulationStarted()
    {
        HolsterWeapon();
    }

    void OnMsgGunInteraction(MsgWeaponInteraction@ msg)
    {
        switch(msg.interaction)
        {
        case WeaponInteraction::Fire:
            {
                if (msg.keyState == ezTriggerState::Deactivated)
                {
                    requireSingleShotReset = false;
                    return;
                }

                if (msg.weaponInfo.iAmmoInClip <= 0)
                {
                    FireEmptyWeapon(msg);
                    return;
                }
    
                if (singleShotPerTrigger)
                {
                    if (msg.keyState == ezTriggerState::Activated)
                    {
                        if (!requireSingleShotReset)
                        {
                            requireSingleShotReset = true;
                            FireWeapon(msg);
                        }
                    }
                }
                else
                {
                    FireWeapon(msg);
                }

                break;
            }
        case WeaponInteraction::DrawWeapon:
            DrawWeapon();
            break;
        case WeaponInteraction::HolsterWeapon:
            HolsterWeapon();
            break;

        case WeaponInteraction::Reload:
            if (msg.weaponInfo.iAmmoInClip >= msg.weaponInfo.iClipSize)
                return;
    
            Reload(msg);
            break;

        case WeaponInteraction::Update:
            UpdateWeapon(msg);
            break;
        }
    }

    void FireWeapon(MsgWeaponInteraction@ msg)
    {
        // abstract
    }

    void FireEmptyWeapon(MsgWeaponInteraction@ msg)
    {
        // override this function to make an empty gun sound or something like that
        Reload(msg);
    }

    void PlayShootSound()
    {
        auto owner = GetOwner();

        auto node = owner.FindChildByName("ShootSound", true);
        if (@node != null)
        {
            ezFmodEventComponent@ fmodComp;
            if (node.TryGetComponentOfBaseType(@fmodComp))
            {
                fmodComp.StartOneShot();
            }
        }

        @node = owner.FindChildByName("Muzzleflash", true);
        if (@node != null)
        {
            ezParticleComponent@ particleComp;
            if (node.TryGetComponentOfBaseType(@particleComp))
            {
                particleComp.InterruptEffect();
                particleComp.StartEffect();
            }
        }
    }

    void DrawWeapon()
    {
        auto graphics = GetOwner().FindChildByName("Graphics", true);

        if (@graphics == null)
            return;

        graphics.SetActiveFlag(true);
    }

    void HolsterWeapon()
    {
        auto graphics = GetOwner().FindChildByName("Graphics", true);

        if (@graphics == null)
            return;

        graphics.SetActiveFlag(false);
    }    

    void Reload(MsgWeaponInteraction@ msg)
    {
        if (msg.weaponInfo.eAmmoType == ConsumableType::Ammo_None)
            return;

        int needed = msg.weaponInfo.iClipSize - msg.weaponInfo.iAmmoInClip;
        int take = ezMath::Min(needed, msg.ammoPouch.getAmmoType(msg.weaponInfo.eAmmoType));

        msg.ammoPouch.getAmmoType(msg.weaponInfo.eAmmoType) -= take;
        msg.weaponInfo.iAmmoInClip += take;
    }

    void UpdateWeapon(MsgWeaponInteraction@ msg)
    {
        ezVec2 resolution = ezDebug::GetResolution();
        ezVec2 screenCenter = resolution;
        screenCenter *= 0.5;

        ezColor col = ezColor::White;

        float w = 10.0f;
        ezVec3 start = ezVec3(screenCenter.x, screenCenter.y - w, 0);
        ezVec3 end = ezVec3(screenCenter.x, screenCenter.y + w, 0);
        ezDebug::Draw2DLine(start, end, col, col);
        
        start = ezVec3(screenCenter.x - w, screenCenter.y, 0);
        end = ezVec3(screenCenter.x + w, screenCenter.y, 0);
        ezDebug::Draw2DLine(start, end, col, col);
    }
}
