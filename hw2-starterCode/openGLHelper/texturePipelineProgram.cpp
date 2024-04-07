#include "texturePipelineProgram.h"
#include "openGLHeader.h"

#include <cstring>
#include <cstdio>
#include <iostream>

using namespace std;

int texturePipelineProgram::Init(const char* vertexShaderPath) 
{
  // means texture shader 
  //cout << "enter here for texture initialization" << endl;

  if (BuildShadersFromFiles(vertexShaderPath, "texture.vertexShader.glsl", "texture.fragmentShader.glsl") != 0) {
    return 1;
  }

  return 0;
}