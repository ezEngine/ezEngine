import ez = require("TypeScript/ez")
import _ge = require("Scripting/GameEnums")
import guns = require("Prefabs/Guns/Gun")

export class Pistol extends guns.Gun {


    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()

        this.singleShotPerTrigger = true;
    }

    static RegisterMessageHandlers() {

        guns.Gun.RegisterMessageHandlers();

        //ez.TypescriptComponent.RegisterMessageHandler(ez.MsgSetColor, "OnMsgSetColor");
    }

    nextAmmoPlus1Time: number = 0;

    OnSimulationStarted(): void {

        super.OnSimulationStarted();

        this.SetTickInterval(0);
    }

    Tick(): void {

        const now = ez.Time.GetGameTime();

        if (this.nextAmmoPlus1Time < now) {
            this.ammoInClip = ez.Utils.Clamp(this.ammoInClip + 1, 0, this.GetAmmoClipSize());

            this.nextAmmoPlus1Time = now + ez.Time.Seconds(0.5);
        }
    }

    GetAmmoType(): _ge.Consumable {
        return _ge.Consumable.Ammo_None;
    }

    GetAmmoClipSize(): number {
        return 8;
    }

    Fire(): void {

        let spawn = this.GetOwner().FindChildByName("Spawn").TryGetComponentOfBaseType(ez.SpawnComponent);
        if (spawn.CanTriggerManualSpawn() == false)
            return;

        this.nextAmmoPlus1Time = ez.Time.GetGameTime() + ez.Time.Seconds(1.5);

        this.ammoInClip -= 1;

        spawn.TriggerManualSpawn(false, ez.Vec3.ZeroVector());

        this.PlayShootSound();

    }

    RenderCrosshair(): void {
        // render nothing, have a laser pointer already
    }
}

