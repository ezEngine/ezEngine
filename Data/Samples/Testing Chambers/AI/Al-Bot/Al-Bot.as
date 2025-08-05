// TODO:
// * decals
// * hear player noises
// * patrol paths
// * better area damage (falloff, don't hit yourself)
// * aim up/down
// * better projectil spawn

enum BotState
{
    Idle, // stand around, do nothing
    Dead,
    ClosingDistance, // move towards player, until can see player and distance is close enough
    ShootTarget, // close enough, target is visible, turn towards it and shoot
    FollowTarget, // can't see target anymore, go to last known location
    Searching,
    Patrol,
}

class Bot : ezAngelScriptClass
{
    int Health = 50;
    float MaxShootDistance = 15.0f;
    bool ShowState = true;
    private BotState _botState = BotState::Idle;
    private bool _hasTargetPosition = false;
    private ezGameObjectHandle _player;
    private ezGameObjectHandle _visiblePlayer;
    private bool _canSeePlayer = false;
    private bool _lastPlayerPosValid = false;
    private ezVec3 _lastPlayerPos;
    private ezVec3 _randomPositionReferencePoint;
    private ezVec3 _vStatusTextPos;
    private int _iSearchAttempt = 0;
    private ezTime _lastIdleAction;

    void DeactivateGO(ezStringView object)
    {
        ezGameObject@ obj = GetOwner().FindChildByName(object, true);
        if (@obj != null)
        {
            obj.SetActiveFlag(false);
        }
    }

    void DeactivateAll()
    {
        DeactivateGO("OnDeathDeactivate");

        // disable the navigation component, so that it doesn't update the position any further
        ezAiNavigationComponent@ navComp;
        if (GetOwner().TryGetComponentOfBaseType(@navComp))
        {
            navComp.Active = false;
        }

        ezJoltHitboxComponent@ hitComp;
        if (GetOwner().TryGetComponentOfBaseType(@hitComp))
        {
            hitComp.Active = false;
        }
    }

    void OnSimulationStarted()
    {
        EnterState_Idle();
        _lastIdleAction = GetWorld().GetClock().GetAccumulatedTime() - ezTime::MakeFromSeconds(8);
    }

    void EnterState_Idle()
    {
        _botState = BotState::Idle;
        _canSeePlayer = false;
        _lastPlayerPosValid = false;
        _lastIdleAction = GetWorld().GetClock().GetAccumulatedTime();
    }

    void EnterState_ClosingDistance(ezLocalBlackboardComponent@ bbComp)
    {
        _botState = BotState::ClosingDistance;
        _hasTargetPosition = false;
        bbComp.SetEntryValue("PlayIdleAction", -1);
    }

    void EnterState_ShootTarget(ezAiNavigationComponent@ navComp, ezLocalBlackboardComponent@ bbComp)
    {
        _botState = BotState::ShootTarget;
        navComp.StopWalking(0.5f);
        bbComp.SetEntryValue("PlayIdleAction", -1);
    }

    void EnterState_FollowTarget(ezAiNavigationComponent@ navComp, ezLocalBlackboardComponent@ bbComp)
    {
        _botState = BotState::FollowTarget;
        bbComp.SetEntryValue("PlayIdleAction", -1);

        if (_lastPlayerPosValid)
        {
            navComp.SetDestination(_lastPlayerPos, false);
        }
        else
        {
            EnterState_Searching(navComp, bbComp);
        }
    }

    void EnterState_Dead()
    {
        _botState = BotState::Dead;
        DeactivateAll();
    }

    void EnterState_Searching(ezAiNavigationComponent@ navComp, ezLocalBlackboardComponent@ bbComp)
    {
        _botState = BotState::Searching;
        _canSeePlayer = false;
        _hasTargetPosition = false;
        _iSearchAttempt = 0;
        bbComp.SetEntryValue("PlayIdleAction", -1);

        if (_lastPlayerPosValid)
        {
            // very first try, just rotate towards we last saw the player
            navComp.TurnTowards(_lastPlayerPos.GetAsVec2());
        }
    }

    void EnterState_Patrol(ezLocalBlackboardComponent@ bbComp)
    {
        _botState = BotState::Patrol;
        _hasTargetPosition = false;
        bbComp.SetEntryValue("PlayIdleAction", -1);
    }

    void UpdateState_Idle(ezLocalBlackboardComponent@ bbComp)
    {
        if (ShowState) 
        {
            ezDebug::Draw3DText("Idle", _vStatusTextPos, ezColor::LightGrey, 32);
        }

        if (_canSeePlayer)
        {
            EnterState_ClosingDistance(bbComp);
            return;
        }

        if (GetWorld().GetClock().GetAccumulatedTime() - _lastIdleAction > ezTime::Seconds(5.0))
        {
            _lastIdleAction = GetWorld().GetClock().GetAccumulatedTime();

            ezUInt32 rnd = GetWorld().GetRandomNumberGenerator().UIntInRange(10);

            if (rnd < 4)
            {
                bbComp.SetEntryValue("PlayIdleAction", rnd % 2); // random idle animation
            }
            else
            {
                EnterState_Patrol(bbComp);
            }
        }
    }

    void UpdateState_ClosingDistance(ezAiNavigationComponent@ navComp, ezLocalBlackboardComponent@ bbComp)
    {
        if (ShowState) 
        {
            ezDebug::Draw3DText("ClosingDistance", _vStatusTextPos, ezColor::Yellow, 32);
        }

        bbComp.SetEntryValue("Shoot", -1);
        navComp.Speed = 3.0;

        if (_canSeePlayer)
        {
            float dist = _lastPlayerPos.GetDistanceTo(GetOwner().GetGlobalPosition());
            if (dist < MaxShootDistance * 0.9f)
            {
                EnterState_ShootTarget(navComp, bbComp);
                return;
            }

            if (_hasTargetPosition && (_randomPositionReferencePoint.GetDistanceTo(_lastPlayerPos) > 2.0f))
            {
                // if the player has moved too far, get a new random point to walk to
                _hasTargetPosition = false;
            }
        }

        if (!_hasTargetPosition)
        {
            _randomPositionReferencePoint = _lastPlayerPos;

            ezVec3 point;
            if (navComp.FindRandomPointAroundCircle(_lastPlayerPos, ezMath::Min(10.0f, MaxShootDistance * 0.5f), point))
            {
                if (point.GetDistanceTo(_lastPlayerPos) < MaxShootDistance * 0.9f)
                {
                    // FindRandomPointAroundCircle will fail, as long as the navmesh isn't loaded around that point
                    navComp.SetDestination(point, false);
                    _hasTargetPosition = true;
                }
                else
                {
                    if (ShowState)
                    {
                        ezDebug::AddPersistentLineSphere(point + ezVec3(0, 0, 1), 0.25f, ezColor::DarkRed, ezTransform::MakeIdentity(), ezTime::MakeFromSeconds(5));
                        ezLog::Info("Random point is too far away, try again.");
                    }
                }
            }

            return;
        }

        switch (navComp.GetState())
        {
        case ezAiNavigationComponentState::Falling:
            return;

        case ezAiNavigationComponentState::Failed:
        case ezAiNavigationComponentState::Fallen:
            // try another point
            EnterState_ClosingDistance(bbComp);
            return;

        case ezAiNavigationComponentState::Idle:
            EnterState_Searching(navComp, bbComp);
            return;
        }
    }

    void UpdateState_FollowTarget(ezAiNavigationComponent@ navComp, ezLocalBlackboardComponent@ bbComp)
    {
        if (ShowState) 
        {
            ezDebug::Draw3DText("FollowTarget", _vStatusTextPos, ezColor::GreenYellow, 32);
        }

        switch (navComp.GetState())
        {
        case ezAiNavigationComponentState::Failed:
        case ezAiNavigationComponentState::Fallen:
        case ezAiNavigationComponentState::Idle:
            EnterState_Searching(navComp, bbComp);
            return;
        }

        if (_canSeePlayer)
        {
            // let the other state decide when to stop
            EnterState_ClosingDistance(bbComp);
            return;
        }

        bbComp.SetEntryValue("Shoot", 0);
        navComp.Speed = 1.5;
    }

    void UpdateState_Searching(ezAiNavigationComponent@ navComp, ezLocalBlackboardComponent@ bbComp)
    {
        if (ShowState) 
        {
            ezDebug::Draw3DText("Searching", _vStatusTextPos, ezColor::Orange, 32);
        }

        if (_canSeePlayer)
        {
            EnterState_ClosingDistance(bbComp);
            return;
        }

        bbComp.SetEntryValue("Shoot", 0); // draw weapon
        navComp.Speed = 1.5;

        switch (navComp.GetState())
        {
        case ezAiNavigationComponentState::Failed:
        case ezAiNavigationComponentState::Fallen:
        case ezAiNavigationComponentState::Idle:
            // failure or nothing to do -> search for another target
            _hasTargetPosition = false;
            break;

        default:
            // busy navigating -> continue
            return;
        }

        if (_iSearchAttempt > 2)
        {
            EnterState_Idle();
            return;
        }

        if (_lastPlayerPosValid)
        {
            // first try the last known player position
            navComp.SetDestination(_lastPlayerPos, true);
            _lastPlayerPosValid = false;
            return;
        }

        if (!_hasTargetPosition)
        {
            ezVec3 point;
            if (navComp.FindRandomPointAroundCircle(GetOwner().GetGlobalPosition(), 10.0f + _iSearchAttempt * 3.0f, point))
            {
                _hasTargetPosition = true;
                navComp.SetDestination(point, true);
                ++_iSearchAttempt;
            }
        }
    }

    void UpdateState_ShootTarget(ezAiNavigationComponent@ navComp, ezLocalBlackboardComponent@ bbComp)
    {
        if (ShowState) 
        {
            ezDebug::Draw3DText("ShootTarget", _vStatusTextPos, ezColor::Red, 32);
        }

        if (bbComp.GetEntryValue_asInt32("Melee") == -2)
        {
            // one frame delay, before doing another melee attack
            // this is necessary for the animation graph to be able to detect that the current value is below 0
            // otherwise it would not see the change to -1 and then to 0 and wouldn't restart the melee animation
            bbComp.SetEntryValue("Melee", -1);
            return;
        }

        if (bbComp.GetEntryValue_asInt32("Melee") >= 0)
        {
            // currently playing melee animation, wait for that to end
            return;
        }

        bbComp.SetEntryValue("Shoot", 0); // draw weapon

        if (!_canSeePlayer)
        {
            EnterState_FollowTarget(navComp, bbComp);
            return;
        }

        float dist = _lastPlayerPos.GetDistanceTo(GetOwner().GetGlobalPosition());
        if (dist > MaxShootDistance * 1.1f)
        {
            EnterState_ClosingDistance(bbComp);
            return;
        }

        navComp.Speed = 1.5;
        navComp.TurnTowards(_lastPlayerPos.GetAsVec2());

        const ezAngle angle = navComp.GetTurnAngleTowards(_lastPlayerPos.GetAsVec2());

        if (ezMath::Abs(angle.GetDegree()) < 20)
        {
            if (dist < 2.5f)
            {
                bbComp.SetEntryValue("Melee", 1); // Melee attack
            }
            else
            {
                bbComp.SetEntryValue("Shoot", 1); // shoot
            }
        }
    }

    void UpdateState_Patrol(ezAiNavigationComponent@ navComp, ezLocalBlackboardComponent@ bbComp)
    {
        if (ShowState) 
        {
            ezDebug::Draw3DText("Patroling", _vStatusTextPos, ezColor::Green, 32);
        }

        if (_canSeePlayer)
        {
            EnterState_ClosingDistance(bbComp);
            return;
        }

        bbComp.SetEntryValue("Shoot", -1); // holster weapon
        navComp.Speed = 1.0; // walk slowly

        if (!_hasTargetPosition)
        {
            ezVec3 point;
            if (navComp.FindRandomPointAroundCircle(GetOwner().GetGlobalPosition(), 20.0f, point))
            {
                _hasTargetPosition = true;
                navComp.SetDestination(point, true);
            }

            return;
        }

        switch (navComp.GetState())
        {
        case ezAiNavigationComponentState::Failed:
        case ezAiNavigationComponentState::Fallen:
        case ezAiNavigationComponentState::Idle:
            // failure or nothing to do -> go idle
            _hasTargetPosition = false;
            EnterState_Idle();
            break;

        default:
            // busy navigating -> continue
            return;
        }
    }

    void Update() 
    {
        if (_botState == BotState::Dead)
            return;

        ezLocalBlackboardComponent@ bbComp;
        if (!GetOwner().TryGetComponentOfBaseType(@bbComp))
            return;

        ezAiNavigationComponent@ navComp;
        if (!GetOwner().TryGetComponentOfBaseType(@navComp))
            return;

        bbComp.SetEntryValue("Shoot", -1);

        if (_canSeePlayer)
        {
            ezGameObject@ playerObj;
            if (GetWorld().TryGetObject(_visiblePlayer, @playerObj))
            {
                _lastPlayerPos = playerObj.GetGlobalPosition();
                _lastPlayerPosValid = true;
            }
            else
            {
                _visiblePlayer.Invalidate();
                _lastPlayerPosValid = false;
                _canSeePlayer = false;
            }
        }

        // set movement animation speed
        {
            const float fAnimSpeed = 3.0f;

            // tell the walk animation how fast to play
            ezVec3 vVelocity = GetOwner().GetLinearVelocity();
            vVelocity.z = 0.0f;
            float fSpeed = vVelocity.GetLength();
            bbComp.SetEntryValue("MoveForwards", ezMath::Clamp(fSpeed / fAnimSpeed, 0.0f, 1.0f));
        }        

        if (_lastPlayerPosValid && ShowState)
        {
            ezDebug::DrawLine(_lastPlayerPos + ezVec3(0, 0, 1), _lastPlayerPos, ezColor::BlueViolet, ezColor::BlueViolet);
        }

        _vStatusTextPos = GetOwner().GetGlobalPosition() + ezVec3(0, 0, 1);

        switch(_botState)
        {
        case BotState::Idle:
            UpdateState_Idle(bbComp);
            break;

        case BotState::ClosingDistance:
            UpdateState_ClosingDistance(navComp, bbComp);
            break;

        case BotState::ShootTarget:
            UpdateState_ShootTarget(navComp, bbComp);
            break;

        case BotState::FollowTarget:
            UpdateState_FollowTarget(navComp, bbComp);
            break;

        case BotState::Searching:
            UpdateState_Searching(navComp, bbComp);
            break;

        case BotState::Patrol:
            UpdateState_Patrol(navComp, bbComp);
            break;
        }
    }
    
    void OnMsgDamage(ezMsgDamage@ msg) 
    {
        if (_botState == BotState::Dead)
            return;

        ezLocalBlackboardComponent@ bbComp;
        if (!GetOwner().TryGetComponentOfBaseType(@bbComp))
            return;

        Health -= int(msg.Damage);

        if (_visiblePlayer.IsInvalidated() || !_lastPlayerPosValid)
        {
            // if we get damage (of any kind ...) let the bot directly to the player
            // this is cheating a bit, the damage could come from something else

            ezGameObject@ playerObj;
            if (GetWorld().TryGetObjectWithGlobalKey("Player", @playerObj))
            {
                ezAiNavigationComponent@ navComp;
                if (GetOwner().TryGetComponentOfBaseType(@navComp))
                {
                    EnterState_ClosingDistance(bbComp);
                    _hasTargetPosition = true;
                    navComp.SetDestination(playerObj.GetGlobalPosition(), true);
                }
            }
        }

        // play a random hit reaction
        // the animation graph takes care of only playing one of those at the same time
        // and this won't interrupt a playing one
        int iHitReaction = GetWorld().GetRandomNumberGenerator().IntMinMax(0, 1);
        bbComp.SetEntryValue("PlayHitReaction", iHitReaction);

        if (Health <= 0)
        {
            EnterState_Dead();

            // let the animation graph know that the bot died, so that it can play the death animation
            bbComp.SetEntryValue("IsAlive", false);

            // also enable the ragdoll, if we have one
            ezJoltRagdollComponent@ ragComp;
            if (GetOwner().TryGetComponentOfBaseType(@ragComp))
            {
                // enable the ragdoll component to have it take over the animation process
                ragComp.Active = true;
                
                // fade out the death animation over a short time
                ragComp.FadeJointMotorStrength(0.0f, ezTime::MakeFromSeconds(1.6f));

                // add a start impulse, to push the ragdoll procedurally
                ragComp.SetInitialImpulse(msg.GlobalPosition, msg.ImpactDirection * msg.Damage * 50 * 10);
            }
        }
    }

    /////////////////////////////////////////////////////////////
    // Generic Events

    void OnMsgGenericEvent(ezMsgGenericEvent@ msg)
    {
        // if the playing (death) animation has a marker to switch to "powered" mode, forward that request to the ragdoll
        if (msg.Message == "AnimMode-Powered")
        {
            ezJoltRagdollComponent@ ragComp;
            if (GetOwner().TryGetComponentOfBaseType(@ragComp))
            {
                ragComp.AnimMode = ezJoltRagdollAnimMode::Powered;
            }

            return;
        }

        // if the playing (death) animation has a marker to switch to "limp" mode, forward that request to the ragdoll
        if (msg.Message == "AnimMode-Limp")
        {
            ezJoltRagdollComponent@ ragComp;
            if (GetOwner().TryGetComponentOfBaseType(@ragComp))
            {
                ragComp.AnimMode = ezJoltRagdollAnimMode::Limp;
            }

            return;
        }

        if (_botState == BotState::Dead)
            return;

        if (msg.Message == "gun.shoot")
        {
            ezGameObject@ muzzleObj = GetOwner().FindChildByName("Muzzle", true);
            if (@muzzleObj != null)
            {
                ezParticleComponent@ particleComp;
                if (muzzleObj.TryGetComponentOfBaseType(@particleComp))
                {
                    particleComp.StartEffect();
                }

                ezFmodEventComponent@ fmodComp;
                if (muzzleObj.TryGetComponentOfBaseType(@fmodComp))
                {
                    fmodComp.StartOneShot();
                }
            }

            ezGameObject@ spawnerObj = GetOwner().FindChildByName("BulletSpawner", true);
            if (@spawnerObj != null)
            {
                ezSpawnComponent@ spawnComp;
                if (spawnerObj.TryGetComponentOfBaseType(@spawnComp))
                {
                    spawnComp.TriggerManualSpawn(true, ezVec3::MakeZero());
                }
            }

            return;
        }

        if (msg.Message == "Melee-Hit")
        {
            ezGameObject@ spawnerObj = GetOwner().FindChildByName("Melee-Damage", true);
            if (@spawnerObj != null)
            {
                ezAreaDamageComponent@ dmgComp;
                if (spawnerObj.TryGetComponentOfBaseType(@dmgComp))
                {
                    dmgComp.ApplyAreaDamage();
                }
                ezSound::PlaySound("{ 32cb0079-08cc-4f6b-a94b-0832e602ee1a }", GetOwner().GetGlobalPosition() + ezVec3(0, 0, 1.5), ezQuat::MakeIdentity(), 1.0f, 1.0f, false);
            }
        }

        if (msg.Message == "walk.foot.l" || msg.Message == "walk.foot.r" || msg.Message == "run.foot.l" || msg.Message == "run.foot.r")
        {
            auto pos = GetOwner().GetGlobalPosition();
            ezPhysics::RaycastSurfaceInteraction(pos + ezVec3(0, 0, 0.1f), ezVec3(0, 0, -0.5f), 0, ezPhysicsShapeType::Static, "{ 0a105955-e069-4523-80ce-7b9867bde687 }", "Footstep");
        }
        
    }

    /////////////////////////////////////////////////////////////
    // Sensor

    void OnMsgSensorDetectedObjectsChanged(ezMsgSensorDetectedObjectsChanged@ msg)
    {
        _canSeePlayer = false;
        _visiblePlayer.Invalidate();

        ezGameObject@ sensorObj = GetOwner().FindChildByName("FeelSensor", true);
        if (@sensorObj != null)
        {
            ezSensorComponent@ sensorComp;
            if (sensorObj.TryGetComponentOfBaseType(@sensorComp))
            {
                if (sensorComp.GetDetectedObjectsCount() > 0)
                {
                    _visiblePlayer = sensorComp.GetDetectedObject(0);
                    _canSeePlayer = true;
                    return;
                }
            }
        }

        @sensorObj = GetOwner().FindChildByName("VisionSensor", true);
        if (@sensorObj != null)
        {
            ezSensorComponent@ sensorComp;
            if (sensorObj.TryGetComponentOfBaseType(@sensorComp))
            {
                if (sensorComp.GetDetectedObjectsCount() > 0)
                {
                    _visiblePlayer = sensorComp.GetDetectedObject(0);
                    _canSeePlayer = true;
                    return;
                }
            }
        }
    }
}
