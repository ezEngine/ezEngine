import ez = require("TypeScript/ez")
import _ge = require("Scripting/GameEnums")

export class AmmoPouch {
    ammo: number[] = [];

    constructor() {
        this.ammo[_ge.Consumable.Ammo_Pistol] = 20;
        this.ammo[_ge.Consumable.Ammo_MachineGun] = 50;
        this.ammo[_ge.Consumable.Ammo_Plasma] = 50;
        this.ammo[_ge.Consumable.Ammo_Shotgun] = 10;
        this.ammo[_ge.Consumable.Ammo_Rocket] = 5;
    }
}

export enum GunInteraction {
    Fire,
    Reload,
}

export class MsgGunInteraction extends ez.Message {
    EZ_DECLARE_MESSAGE_TYPE;

    interaction: GunInteraction;
    keyState: ez.TriggerState;
    ammoPouch: AmmoPouch = null;
}

export abstract class Gun extends ez.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    protected singleShotPerTrigger: boolean = false;

    private requireSingleShotReset: boolean = false;

    constructor() {
        super()
    }

    shootSoundComponent: ez.FmodEventComponent = null;

    static RegisterMessageHandlers() {

        ez.TypescriptComponent.RegisterMessageHandler(MsgGunInteraction, "OnMsgGunInteraction");
    }

    OnActivated(): void {

        this.DeselectGun();
    }

    OnSimulationStarted(): void {
        let owner = this.GetOwner();

        let node = owner.FindChildByName("ShootSound", true);
        if (node != null) {
            this.shootSoundComponent = node.TryGetComponentOfBaseType(ez.FmodEventComponent);
        }

    }

    OnMsgGunInteraction(msg: MsgGunInteraction): void {

        if (msg.interaction == GunInteraction.Fire) {

            if (msg.keyState == ez.TriggerState.Deactivated) {
                this.requireSingleShotReset = false;
                return;
            }

            if (this.ammoInClip == 0) {
                // empty gun sound etc.

                this.Reload(msg.ammoPouch);
                return;
            }

            if (this.singleShotPerTrigger) {

                if (msg.keyState == ez.TriggerState.Activated) {

                    if (!this.requireSingleShotReset) {
                        this.requireSingleShotReset = true;
                        this.Fire();
                    }
                }
            }
            else {

                this.Fire();
            }
        }
        else if (msg.interaction == GunInteraction.Reload) {

            if (this.GetAmmoInClip() >= this.GetAmmoClipSize())
                return;

            this.Reload(msg.ammoPouch);
        }
    }

    abstract Fire(): void;

    Reload(ammoPouch: AmmoPouch): void {
        let type = this.GetAmmoType();

        if (type == _ge.Consumable.Ammo_None)
            return;

        let needed = this.GetAmmoClipSize() - this.ammoInClip;
        let take = Math.min(needed, ammoPouch.ammo[type]);

        ammoPouch.ammo[type] -= take;
        this.ammoInClip += take;
    }

    PlayShootSound(): void {

        if (this.shootSoundComponent != null && this.shootSoundComponent.IsValid()) {
            this.shootSoundComponent.StartOneShot();
        }
    }

    protected ammoInClip: number = 0;

    abstract GetAmmoType(): _ge.Consumable;
    abstract GetAmmoClipSize(): number;

    GetAmmoInClip(): number {
        return this.ammoInClip;
    }

    SelectGun(): void {

        let graphics = this.GetOwner().FindChildByName("Graphics", true);

        if (graphics == null)
            return;

        graphics.SetActiveFlag(true);
    }

    DeselectGun(): void {

        let graphics = this.GetOwner().FindChildByName("Graphics", true);

        if (graphics == null)
            return;

        graphics.SetActiveFlag(false);
    }

    RenderCrosshair(): void {
    
        const resolution = ez.Debug.GetResolution();
        let screenCenter = resolution;
        screenCenter.MulNumber(0.5);

        let lines: ez.Debug.Line[] = [];
        
        lines[0] = new ez.Debug.Line();
        lines[0].startX = screenCenter.x;
        lines[0].startY = screenCenter.y - 10;
        lines[0].endX = screenCenter.x;
        lines[0].endY = screenCenter.y + 10;

        lines[1] = new ez.Debug.Line();
        lines[1].startX = screenCenter.x - 10;
        lines[1].startY = screenCenter.y;
        lines[1].endX = screenCenter.x + 10;
        lines[1].endY = screenCenter.y;

        ez.Debug.Draw2DLines(lines);
        
    }
}

