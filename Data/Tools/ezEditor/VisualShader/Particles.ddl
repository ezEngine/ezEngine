Node %ParticleEffect
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs particle effect system-wide data." }

  OutputPin %TotalLifeTime
  {
    string %Type { "float" }
    string %Inline { "TotalEffectLifeTime" }
    string %Tooltip { "Total time since the particle effect started." }
  }
}

Node %Particle
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs per-particle data." }

  OutputPin %Life
  {
    string %Type { "float" }
    string %Color { "Yellow" }
    string %Inline { "G.Input.Life" }
    string %Tooltip { "Particle life from 1.0 (just spawned) to 0.0 (death)." }
  }

  OutputPin %Variation
  {
    string %Type { "float" }
    string %Color { "Orange" }
    string %Inline { "G.Input.Variation" }
    string %Tooltip { "Random variation value per particle." }
  }

  OutputPin %QuadUV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %Inline { "GetParticleQuadUV()" }
    string %Tooltip { "Raw [0-1] UV across the particle, unaffected by flipbook animations and random variations. Use this for textures that should not be chopped up by the flipbook, e.g. a pattern that a flipbook mask is blended with. On trails, X runs across the trail and Y along it." }
  }

  OutputPin %FlipbookUV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %Inline { "GetParticleFlipbookUV()" }
    string %Tooltip { "UV transformed into the current flipbook animation / random variation frame. This is what texture nodes sample with by default." }
  }
}
