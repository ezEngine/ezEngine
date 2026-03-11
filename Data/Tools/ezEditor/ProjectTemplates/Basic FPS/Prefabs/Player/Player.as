#include "../../Scripts/Shared.as"

class Player : ezAngelScriptClass
{
    int m_iHealth = 100;

    void Update(ezTime deltaTime)
    {
        if (m_iHealth <= 0)
        {
            ezDebug::DrawInfoText("YOU ARE DEAD", ezDebugTextPlacement::TopCenter, "Player", ezColor::OrangeRed);
            return;
        }
        
        ezStringBuilder text;
        text.SetFormat("Health: {}", m_iHealth);
        ezDebug::DrawInfoText(text, ezDebugTextPlacement::TopLeft, "Player", ezColor::White);

        ezInputComponent@ inputComp;
        if (!GetOwner().TryGetComponentOfBaseType(@inputComp))
            return;
        
        UpdateCharacterMovement(inputComp);
        UpdateInteractions(inputComp);
    }

    void UpdateCharacterMovement(ezInputComponent@ inputComp)
    {
        ezMsgMoveCharacterController moveMsg;

        moveMsg.MoveForwards = inputComp.GetCurrentInputState("move_forwards", false);
        moveMsg.MoveBackwards = inputComp.GetCurrentInputState("move_backwards", false);
        moveMsg.StrafeLeft = inputComp.GetCurrentInputState("move_left", false);
        moveMsg.StrafeRight = inputComp.GetCurrentInputState("move_right", false);
        moveMsg.RotateLeft = inputComp.GetCurrentInputState("turn_left", false);
        moveMsg.RotateRight = inputComp.GetCurrentInputState("turn_right", false);
        moveMsg.Jump = inputComp.GetCurrentInputState("jump", false) > 0.5f;
        moveMsg.Run = inputComp.GetCurrentInputState("run", false) > 0.5f;
        moveMsg.Crouch = inputComp.GetCurrentInputState("crouch", false) > 0.5f;

        GetOwner().SendMessage(moveMsg);

        ezGameObject@ headBoneObj = GetOwner().FindChildByName("HeadBone", true);
        if (@headBoneObj != null)
        {
            ezHeadBoneComponent@ headBoneComp;
            if (headBoneObj.TryGetComponentOfBaseType(@headBoneComp))
            {
                float turnUp = inputComp.GetCurrentInputState("turn_up", false);
                float turnDown = inputComp.GetCurrentInputState("turn_down", false);

                headBoneComp.ChangeVerticalRotation(turnDown - turnUp);
            }
        }
    }

    ezGameObject@ GetCamera()
    {
        return GetOwner().FindChildByName("Camera", true);
    }

    ezJoltGrabObjectComponent@ GetGrabber()
    {
        ezGameObject@ cameraObj = GetCamera();
        if (@cameraObj != null)
        {
            ezJoltGrabObjectComponent@ grabComp;
            if (cameraObj.TryGetComponentOfBaseType(@grabComp))
            {
                return @grabComp;
            }
        }

        return null;
    }

    void UpdateInteractions(ezInputComponent@ inputComp)
    {
        if (inputComp.GetCurrentInputState("shoot", true) != 0.0f)
        {
            ezJoltGrabObjectComponent@ grabComp = GetGrabber();
            if (@grabComp != null)
            {
                if (grabComp.HasObjectGrabbed())
                {
                    grabComp.ThrowGrabbedObject(ezVec3(200, 0, 0));
                    return;
                }
            }

            ezGameObject@ gunObj = GetOwner().FindChildByName("Gun", true);
            if (@gunObj != null)
            {
                ezSpawnComponent@ gunComp;
                if (gunObj.TryGetComponentOfBaseType(@gunComp))
                {
                    if (gunComp.TriggerManualSpawn(false, ezVec3::MakeZero()))
                    {
                        // gun fired once
                        // repeat for "shotgun effect"
                        for (int i = 0; i < 20; ++i)
                        {
                            gunComp.TriggerManualSpawn(true, ezVec3::MakeZero());
                        }

                        // asset GUID of the shotgun sound
                        ezSound::PlaySound("{ 9db69eaf-518a-4832-97b0-dd8233df1b74 }", gunObj.GetGlobalPosition(), ezQuat::MakeIdentity(), 1, 1, true);
                    }
                }
            }
        }

        if (inputComp.GetCurrentInputState("use", true) != 0.0f)
        {
            ezJoltGrabObjectComponent@ grabComp = GetGrabber();
            if (@grabComp != null)
            {
                if (grabComp.HasObjectGrabbed())
                {
                    grabComp.DropGrabbedObject();
                    return;
                }
                else
                {
                    if (grabComp.GrabNearbyObject())
                        return;
                }
            }

            // otherwise try to 'use' the closest object

            ezGameObject@ cameraObj = GetCamera();
            if (@cameraObj != null)
            {            
                ezVec3 vHitPosition;
                ezVec3 vHitNormal;
                ezGameObjectHandle hHitObject;

                if (ezPhysics::Raycast(vHitPosition, vHitNormal, hHitObject, cameraObj.GetGlobalPosition(), cameraObj.GetGlobalDirForwards() * 1.5f, ezPhysics::GetCollisionLayerByName("Interaction Raycast"), ezPhysicsShapeType(ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic)))
                {
                    ezMsgGenericEvent msgUse;
                    msgUse.Message = "use";

                    ezGameObject@ hitObj;
                    if (GetWorld().TryGetObject(hHitObject, @hitObj))
                    {
                        hitObj.SendEventMessage(msgUse, GetOwnerComponent());
                    }
                }
            }
        }
    }

    void OnMsgDamage(ezMsgDamage@ msg)
    {
        m_iHealth -= int(msg.Damage);
    }

    void OnMsgPickup(MsgPickup@ msg)
    {
        if (msg.m_iType == 0) // health
        {
            if (m_iHealth < 100)
            {
                m_iHealth = ezMath::Min(m_iHealth + msg.m_iAmount, 100);
                msg.m_bConsumed = true;
            }
        }
    }
}