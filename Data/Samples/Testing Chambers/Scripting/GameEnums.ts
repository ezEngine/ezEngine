import ez = require("TypeScript/ez")

export enum Consumable {
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

export enum Weapon {
    None = 0,
    Pistol = 1,
    Shotgun = 2,
    MachineGun = 3,
    PlasmaRifle = 4,
    RocketLauncher = 5,
};

export let MaxConsumableAmount: number[] = []

MaxConsumableAmount[Consumable.Health] = 100;
MaxConsumableAmount[Consumable.Ammo_Pistol] = 50;
MaxConsumableAmount[Consumable.Ammo_MachineGun] = 150;
MaxConsumableAmount[Consumable.Ammo_Shotgun] = 40;
MaxConsumableAmount[Consumable.Ammo_Plasma] = 100;
MaxConsumableAmount[Consumable.Ammo_Rocket] = 20;

