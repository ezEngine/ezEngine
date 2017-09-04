
float2 ComputeSpriteAnimationTexCoord(float2 texCoord, int numSpriteCols, int numSpriteRows, float particleLife)
{
  int numSprites = numSpriteRows * numSpriteCols;
  float spriteLerp = 1.0 - particleLife;
  int idxSprite = (int)(numSprites * spriteLerp);
  int spriteRow = idxSprite / numSpriteCols;
  int spriteCol = (idxSprite - (spriteRow * numSpriteCols));
  float2 texCoordSize = float2(1, 1) / float2(numSpriteCols, numSpriteRows);

  return texCoordSize * float2(spriteCol, spriteRow) + texCoord * texCoordSize;
}
