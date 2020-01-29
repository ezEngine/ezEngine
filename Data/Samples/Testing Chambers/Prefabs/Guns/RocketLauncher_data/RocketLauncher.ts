import ez = require("TypeScript/ez")
import _ge = require("Scripting/GameEnums")
import gun = require("Prefabs/Guns/Gun")

export class RocketLauncher extends gun.Gun {
    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()

        this.singleShotPerTrigger = true;
    }

    static RegisterMessageHandlers() {

        gun.Gun.RegisterMessageHandlers();

        //ez.TypescriptComponent.RegisterMessageHandler(ez.MsgSetColor, "OnMsgSetColor");
    }

    Tick(): void { }

    GetAmmoType(): _ge.Consumable {
        return _ge.Consumable.Ammo_Rocket;
    }

    GetAmmoClipSize(): number {
        return 3;
    }

    Fire(): void {

        let spawn = this.GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(ez.SpawnComponent);
        if (spawn.CanTriggerManualSpawn() == false)
            return;

        this.ammoInClip -= 1;

        spawn.TriggerManualSpawn(true, ez.Vec3.ZeroVector());

        this.PlayShootSound();
    }

}

