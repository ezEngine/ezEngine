
def GenerateOBJ(outputFile, numMats):

    outputFile.writelines(f"\n#Vertices\n")

    for q in range(0, numMats):
        
        outputFile.writelines(f"v -1.0 -1.0 {q * 0.05}\n")
        outputFile.writelines(f"v +1.0 -1.0 {q * 0.05}\n")
        outputFile.writelines(f"v +1.0 +1.0 {q * 0.05}\n")
        outputFile.writelines(f"v -1.0 +1.0 {q * 0.05}\n")

    outputFile.writelines(f"\n#TexCoords\n")

    for q in range(0, numMats):
        
        outputFile.writelines("vt 0.0 0.0\n")
        outputFile.writelines("vt 1.0 0.0\n")
        outputFile.writelines("vt 1.0 1.0\n")
        outputFile.writelines("vt 0.0 1.0\n")

    outputFile.writelines(f"\n#Faces\n")

    for q in range(0, numMats):
        outputFile.writelines(f"\nusemtl mat{q}\n")        
        outputFile.writelines(f"f {q*4+1}/{q*4+1} {q*4+2}/{q*4+2} {q*4+3}/{q*4+3} {q*4+4}/{q*4+4}\n")
        outputFile.writelines(f"f {q*4+1}/{q*4+1} {q*4+4}/{q*4+4} {q*4+3}/{q*4+3} {q*4+2}/{q*4+2}\n")

    return

def GenerateMTL(outputFile, numMats):

    for m in range(0, numMats):
        outputFile.writelines(f"\nnewmtl mat{m}\n")
        outputFile.writelines("    Kd 0.5880 0.5880 0.5880\n")
        outputFile.writelines("    map_Kd textures\Pattern.tga\n")

    return

def main():
    numMats = 100

    with open("LargeMesh.obj", "w") as outputOBJ:
        outputOBJ.writelines("mtllib LargeMesh.mtl\n")
        GenerateOBJ(outputOBJ, numMats)

    with open("LargeMesh.mtl", "w") as outputMTL:
        GenerateMTL(outputMTL, numMats)

main()
