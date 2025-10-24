# Skeletal Animation System Overview

The skeletal animation system in ezEngine provides a complete solution for animating characters and objects with skeletal meshes.
It is built on the excellent [Ozz Animation library](https://github.com/guillaumeblanc/ozz-animation) and provides a layered architecture that separates data, pose generation, and animation logic.

This system supports:
- Skeletal animation playback
- Multi-clip blending with per-bone weight masks
- 1D and 2D blend spaces (e.g., for locomotion)
- Animation events
- Root motion extraction
- Inverse kinematics (aim IK and two-bone IK)
- Node-based animation graphs for complex animation logic
- Physics integration (ragdolls)

---

## Architecture

The skeletal animation system is organized into **four main layers**:

```
┌─────────────────────────────────────────────────────────┐
│                   AnimGraph Layer                       │
│  (High-level animation logic, blending, state machines) │
│                                                          │
│  ezAnimController, ezAnimGraph, ezAnimGraphInstance     │
│  ezAnimGraphNode subclasses (SampleClip, Blend, etc.)   │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│              Pose Generation Layer                      │
│    (Command-based DAG for efficient pose computation)   │
│                                                          │
│  ezAnimPoseGenerator (commands: Sample, Combine, IK)    │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                 Resource Layer                          │
│          (Skeleton structure and animation data)        │
│                                                          │
│  ezSkeleton, ezSkeletonResource                         │
│  ezAnimationClipResource, ezAnimationPose               │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                Rendering Layer                          │
│              (GPU skinning and visualization)           │
│                                                          │
│  ezSkinnedMeshComponent, ezSkinningState                │
│  ezSkinnedMeshRenderer                                  │
└─────────────────────────────────────────────────────────┘
```

---

## Data Flow

A typical animation frame flows through the system as follows:

```
1. ezAnimController::Update() called
   │
   ├─→ Update all ezAnimGraphInstance objects
   │   │
   │   └─→ Each ezAnimGraphNode::Step() is called
   │       ├─→ Sample animation clips (SampleAnimClipAnimNode)
   │       ├─→ Blend poses (LerpPosesAnimNode)
   │       ├─→ Switch between poses (SwitchPoseAnimNode)
   │       └─→ Output final result (PoseResultAnimNode)
   │
   ├─→ Build command graph in ezAnimPoseGenerator
   │   │
   │   └─→ AnimGraph nodes allocate commands:
   │       ├─→ ezAnimPoseGeneratorCommandSampleTrack
   │       ├─→ ezAnimPoseGeneratorCommandCombinePoses
   │       ├─→ ezAnimPoseGeneratorCommandLocalToModelPose
   │       └─→ ezAnimPoseGeneratorCommandAimIK / TwoBoneIK
   │
   ├─→ ezAnimPoseGenerator::UpdatePose()
   │   │
   │   └─→ Execute commands in dependency order
   │       ├─→ Sample animation clips from resources
   │       ├─→ Combine poses with weight masks
   │       ├─→ Convert local space → model space
   │       └─→ Apply inverse kinematics
   │
   ├─→ Extract and accumulate root motion
   │
   └─→ Final pose available as array of ezMat4 transforms
       │
       └─→ ezSkinningState uploads to GPU
           │
           └─→ ezSkinnedMeshRenderer applies vertex skinning
```

---

## Key Concepts and Terminology

### Skeleton Structure

**Joint / Bone**
- The term 'joint' and 'bone' are often used interchangably
- Generally a node in the skeletal hierarchy
- Has a local transform (relative to parent)
- Can have physics properties (collision, constraints)
- Identified by index or hashed name

**Skeleton**
- Complete hierarchical structure of joints
- Stores rest pose (bind pose) transforms
- Wraps `ozz::animation::Skeleton` internally

### Transform Spaces

**Local Space**
- Transform relative to parent joint
- What animation clips store
- Cheaper to blend and manipulate
- Format: `ozz::math::SoaTransform` (Structure of Arrays, 4 bones per SIMD register)

**Model Space**
- Transform relative to skeleton root
- Used for IK and final rendering
- Computed by concatenating local transforms down the hierarchy
- Format: `ezMat4` (4x4 transformation matrix)

**World Space**
- Transform in world coordinates
- Skeleton root transform applied to model space
- Used for rendering and physics

### Animation Data

**Animation Clip**
- A single animation sequence (walk, run, jump, etc.)
- Contains keyframe data for joint transforms over time
- Can be sampled at any time position
- Stored in `ezAnimationClipResource`

**Pose**
- A snapshot of all joint transforms at a specific moment
- Can be in local space or model space
- Result of sampling and blending animation clips

**Root Motion**
- Movement and rotation of the skeleton root extracted from animation
- Used to move characters in world space
- Prevents "sliding" feet and allows animation-driven movement

### Animation Graph

**ezAnimGraph**
- Node-based graph definition (shared across instances)
- Defines animation logic (sampling, blending, state machines)
- Must call `PrepareForUse()` before creating instances

**ezAnimGraphInstance**
- Runtime instance of a graph for a specific entity
- Holds per-instance state (playback times, blend weights)
- Multiple instances can share one graph definition

**ezAnimController**
- Top-level orchestrator for an animated entity
- Owns AnimGraphInstance(s), pose generator, and clip mappings
- Updates graphs and generates final pose each frame
- Accumulates root motion

**ezAnimGraphNode**
- Base class for all graph nodes
- Implements `Step()` called each frame
- Can have input/output pins of various types
- Can allocate per-instance data

**Pins**
- Typed connections between nodes
- Types: Trigger, Number, Bool, BoneWeights, LocalPose, ModelPose
- Data flows through pins during graph evaluation

### Pose Generation

**Command DAG (Directed Acyclic Graph)**
- Pose generation uses a command pattern
- Commands form a dependency graph
- Only reachable commands are executed
- Each command executes exactly once per frame
- Efficient: avoids redundant work when multiple nodes need same data

**Command Types:**
- `SampleTrack` - Sample an animation clip at a time position
- `RestPose` - Output the skeleton's rest/bind pose
- `CombinePoses` - Blend multiple local poses with weights and masks
- `LocalToModelPose` - Convert local space to model space
- `SampleEventTrack` - Sample animation events without generating pose
- `AimIK` - Point a bone at a target (e.g., head look-at)
- `TwoBoneIK` - Solve two-bone chain (e.g., arm reaching)

---

## Common Use Cases

### Simple Animation Playback

1. Create a skeleton resource (imported from FBX/GLTF)
2. Create animation clip resources (imported from same file)
3. Create an `ezAnimController`
4. Add a simple `ezAnimGraph` with:
   - `SampleAnimClipAnimNode` to play the clip
   - `PoseResultAnimNode` to output the final pose
5. Call `ezAnimController::Update()` each frame
6. Apply pose to `ezSkinnedMeshComponent`

### Blending Two Animations

Use `LerpPosesAnimNode`:
- Input: Two `LocalPose` pins from different clip samplers
- Input: Weight parameter (0-1)
- Output: Blended local pose

### Locomotion with Blend Space

Use `SampleBlendSpace2DAnimNode`:
- Configure with multiple clips (idle, walk, run in different directions)
- Input: Two parameters (e.g., speed and direction)
- Output: Blended pose based on parameter position in 2D space

### Root Motion Character Movement

1. Set `m_fRootMotionAmount` on `SampleAnimClipAnimNode`
2. Extract motion via `ezAnimController::GetRootMotion()`
3. Apply translation and rotation to character controller
4. Send `ezMsgApplyRootMotion` for automated handling

### Inverse Kinematics (IK)

1. Generate base pose (sample and blend animations)
2. Convert to model space (`LocalToModelPose` command)
3. Inject IK via `ezMsgInjectPoseCommands` message
4. Add `AimIK` or `TwoBoneIK` commands to pose generator
5. Final pose includes IK adjustments

### Per-Bone Masking

Use bone weight masks to blend different animations on different body parts:
- Upper body: Attack animation
- Lower body: Locomotion animation

Steps:
1. Create shared bone weights via `ezAnimController::CreateBoneWeights()`
2. Set weights for specific bones (0-1)
3. Pass weights to `CombinePoses` command or `PoseResultAnimNode`

---

## Component Overview

### Scene Components

**ezSkinnedMeshComponent**
- Renders an animated skinned mesh
- Receives pose updates via `ezMsgAnimationPoseUpdated`
- Manages `ezSkinningState` for GPU upload

**ezSkeletonComponent**
- Debug visualization of skeleton
- Renders bones, joints, collision shapes
- Can highlight specific bones

**ezSkeletonPoseComponent**
- Sets static poses on animated meshes
- Options: rest pose, custom pose, disabled

---

## Message System

Components communicate via messages during animation updates:

**ezMsgAnimationPosePreparing**
- Sent before converting local → model space
- Transforms still in SoA format (ozz internal)
- Allows per-bone modifications before final pose

**ezMsgInjectPoseCommands**
- Sent during pose generation to inject additional commands
- Used for inverse kinematics
- Allows components to add IK commands to the pose generator

**ezMsgAnimationPoseUpdated**
- Sent after pose computation (in model space)
- Contains final bone transforms as `ezMat4[]`
- Skinned mesh components listen to this to update rendering

**ezMsgApplyRootMotion**
- Sent when root motion data is available
- Contains translation and rotation for the frame
- Character controllers listen to this for movement

**ezMsgQueryAnimationSkeleton**
- Query message to find skeleton resource
- Animated mesh components respond with their skeleton handle

**ezMsgRetrieveBoneState**
- Query current bone transforms (from ragdoll, etc.)
- Used when external system controls the pose

**ezMsgRopePoseUpdated**
- Specialized message for rope simulation
- Similar to animation pose but for rope segments

---

## Resource Types

**ezSkeletonResource**
- Runtime skeleton data
- Contains joint hierarchy, rest pose, collision geometry
- Wraps `ezSkeleton` (which wraps `ozz::animation::Skeleton`)

**ezEditableSkeleton**
- Editor-friendly representation with full metadata
- Used during import and editing
- Converted to runtime format for game use

**ezAnimationClipResource**
- Runtime animation clip data
- Contains keyframe data compiled into ozz format
- Supports animation events, additive animations, root motion

**ezAnimGraphResource**
- Serialized animation graph
- Contains node definitions and connections
- Includes animation clip name mappings

---

## Performance Considerations

### SIMD Optimization

The system uses `ozz::math::SoaTransform` (Structure of Arrays):
- Stores 4 bone transforms in SIMD registers
- Much faster to blend than Array of Structures
- Conversion to `ezMat4[]` only happens once per frame

### Command DAG Efficiency

Commands are only executed if reachable from final command:
- If a blend weight is 0, that branch isn't evaluated
- Shared commands execute once even if multiple dependents

### Caching

`ezAnimPoseGenerator` caches `ozz::animation::SamplingJob::Context`:
- Reused across frames for the same animation clips
- Speeds up sampling significantly

### Instance Data

Per-node instance data is allocated in one contiguous block:
- Better cache locality
- Single allocation per graph instance
- Nodes access via offset
