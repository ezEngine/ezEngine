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
}
