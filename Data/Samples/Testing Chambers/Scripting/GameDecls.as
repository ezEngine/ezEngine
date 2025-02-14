enum ConsumableType
{
    Health = 0,
    //Armor = 1,

    AmmoTypes_Start = 2,
    Ammo_None = 3,
    Ammo_Pistol = 4,
    Ammo_MachineGun = 5,
    Ammo_Shotgun = 6,
    Ammo_Plasma = 7,
    Ammo_Rocket = 8,
    AmmoTypes_End = 9,
}

enum WeaponType
{
    None = 0,
    Pistol = 1,
    Shotgun = 2,
    MachineGun = 3,
    PlasmaRifle = 4,
    RocketLauncher = 5,
};

class AmmoPouch
{
    int AmmoPistol = 20;
    int AmmoShotgun = 10;
    int AmmoMachineGun = 50;
    int AmmoPlasmaRifle = 50;
    int AmmoRocketLauncher = 5;

    int& getAmmoType(ConsumableType type)
    {
        switch (type)
        {
        case ConsumableType::Ammo_Pistol:
            return AmmoPistol;
        case ConsumableType::Ammo_Shotgun:
            return AmmoShotgun;
        case ConsumableType::Ammo_MachineGun:
            return AmmoMachineGun;
        case ConsumableType::Ammo_Plasma:
            return AmmoPlasmaRifle;
        case ConsumableType::Ammo_Rocket:
            return AmmoRocketLauncher;
        }

        throw("Missing Case");
        return AmmoPistol;
    }
}

class MsgAddConsumable : ezAngelScriptMessage
{
    ConsumableType consumableType;
    int amount = 0;
    bool return_consumed = false;
}

class MsgUnlockWeapon : ezAngelScriptMessage
{
    WeaponType weaponType;
    bool return_consumed = false;
}
enum WeaponInteraction
{
    Fire,
    Reload,
    DrawWeapon,
    HolsterWeapon,
    Update,
}

class WeaponInfo
{
    ezGameObjectHandle hObject;
    bool bUnlocked = false;
    int iAmmoInClip = 0;
    ConsumableType eAmmoType = ConsumableType::Ammo_None;
    int iClipSize = 1;
}

class MsgWeaponInteraction : ezAngelScriptMessage
{
    WeaponInteraction interaction;
    ezTriggerState keyState;
    AmmoPouch@ ammoPouch;
    WeaponInfo@ weaponInfo;
}
