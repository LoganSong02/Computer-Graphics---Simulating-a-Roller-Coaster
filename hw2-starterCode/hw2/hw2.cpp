/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C/C++ starter code

  Student username: <loganson>
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "texturePipelineProgram.h"
#include "vbo.h"
#include "vao.h"

#include <iostream>
#include <cstring>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f }; 
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 2 Milestone - Yuhang(Logan) Song";

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram * pipelineProgram = nullptr;
texturePipelineProgram textPipelineProgram;
VBO * vboVertices = nullptr;
VBO * vboColors = nullptr;
VAO * vao = nullptr;
vector<float> pos;
vector<float> colorVector;
GLuint texHandle;
GLuint texHandleSun;
GLuint texHandleMoon;
GLuint texHandleGalaxy;
GLint textModelView;
GLint textProjection;
GLuint vboGround; 
GLuint vaoGround;

int u = 0;

// Represents one spline control point.
struct Point 
{
  double x, y, z;
};

// Contains the control points of the spline.
struct Spline 
{
  int numControlPoints;
  Point * points;
} spline;

const double basis[16] = {
    -0.5, 1.0,  -0.5, 0.0,
    1.5,  -2.5, 0.0,  1.0,
    -1.5, 2.0,  0.5,  0.0,
    0.5,  -0.5, 0.0,  0.0
};

struct SplineSegment {
    Point position;
    Point tangent;
    Point normal;
    Point binormal;
};

std::vector<SplineSegment> splineSegments;

// calculate the cross product
Point crossProduct(const Point& a, const Point& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// normalize a vector
Point normalize(const Point& v) {
    double length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return {1.0 * v.x / length, 1.0 * v.y / length, 1.0 * v.z / length};
}

void loadSpline(char * argv) 
{
  FILE * fileSpline = fopen(argv, "r");
  if (fileSpline == NULL) 
  {
    printf ("Cannot open file %s.\n", argv);
    exit(1);
  }

  // Read the number of spline control points.
  fscanf(fileSpline, "%d\n", &spline.numControlPoints);
  printf("Detected %d control points.\n", spline.numControlPoints);

  // Allocate memory.
  spline.points = (Point *) malloc(spline.numControlPoints * sizeof(Point));
  // Load the control points.
  for(int i=0; i<spline.numControlPoints; i++)
  {
    if (fscanf(fileSpline, "%lf %lf %lf", 
           &spline.points[i].x, 
	   &spline.points[i].y, 
	   &spline.points[i].z) != 3)
    {
      printf("Error: incorrect number of control points in file %s.\n", argv);
      exit(1);
    }
  }
}

// Multiply C = A * B, where A is a m x p matrix, and B is a p x n matrix.
// All matrices A, B, C must be pre-allocated (say, using malloc or similar).
// The memory storage for C must *not* overlap in memory with either A or B. 
// That is, you **cannot** do C = A * C, or C = C * B. However, A and B can overlap, and so C = A * A is fine, as long as the memory buffer for A is not overlaping in memory with that of C.
// Very important: All matrices are stored in **column-major** format.
// Example. Suppose 
//      [ 1 8 2 ]
//  A = [ 3 5 7 ]
//      [ 0 2 4 ]
//  Then, the storage in memory is
//   1, 3, 0, 8, 5, 2, 2, 7, 4. 
void MultiplyMatrices(int m, int p, int n, const double * A, const double * B, double * C)
{
  for(int i=0; i<m; i++)
  {
    for(int j=0; j<n; j++)
    {
      double entry = 0.0;
      for(int k=0; k<p; k++)
        entry += A[k * m + i] * B[j * p + k];
      C[m * j + i] = entry;
    }
  }
}

void MultiplyMatricesForFloat(int m, int p, int n, const float * A, const float * B, float * C)
{
  for(int i=0; i<m; i++)
  {
    for(int j=0; j<n; j++)
    {
      double entry = 0.0;
      for(int k=0; k<p; k++)
        entry += A[k * m + i] * B[j * p + k];
      C[m * j + i] = entry;
    }
  }
}

Point calculateSplinePoint(double u, Point p0, Point p1, Point p2, Point p3) {
    double U[4] = {u*u*u, u*u, u, 1};
    double control[12] = {p0.x, p1.x, p2.x, p3.x, p0.y, p1.y, p2.y, p3.y, p0.z, p1.z, p2.z, p3.z};

    double UtimesBasis[4] = {0, 0, 0, 0};
    MultiplyMatrices(1, 4, 4, U, basis, UtimesBasis);
    double onePoint[3] = {0, 0, 0};
    MultiplyMatrices(1, 4, 3, UtimesBasis, control, onePoint);

    return {onePoint[0], onePoint[1], onePoint[2]};
}

Point calculateTangent(double u, Point p0, Point p1, Point p2, Point p3) {
    double U[4] = {3*u*u, 2*u, 1, 0}; // Derivative of the U vector for cubic spline
    double control[12] = {p0.x, p1.x, p2.x, p3.x, p0.y, p1.y, p2.y, p3.y, p0.z, p1.z, p2.z, p3.z};

    double UtimesdBasis[4] = {0, 0, 0, 0};
    MultiplyMatrices(1, 4, 4, U, basis, UtimesdBasis);

    double tangentComponents[3] = {0, 0, 0};
    MultiplyMatrices(1, 4, 3, UtimesdBasis, control, tangentComponents);

    return {tangentComponents[0], tangentComponents[1], tangentComponents[2]};
}

void constructSplineSegments(const Spline& spline) {
    splineSegments.clear();
    // Assuming you already have a method to calculate tangent, and the initial normal and binormal
    Point initialV = {0.0, 0.0, 1.0}; // Initial normal, could be arbitrary
    // Point prevBinormal = crossProduct(prevNormal, firstTangent); // Initialize based on the first tangent
    // prevBinormal = normalize(prevBinormal);
    // prevNormal = crossProduct(firstTangent, prevBinormal); // Re-normalize to ensure orthogonality
    Point prevBinormal = {0, 0, 0};

    for (int i = 0; i < spline.numControlPoints - 3; i++) {
        for (double u = 0; u <= 1; u += 0.001) { // Adjust step size as needed
            SplineSegment segment;

            segment.position = calculateSplinePoint(u, spline.points[i], spline.points[i+1], spline.points[i+2], spline.points[i+3]);
            segment.tangent = calculateTangent(u, spline.points[i], spline.points[i+1], spline.points[i+2], spline.points[i+3]);
            segment.tangent = normalize(segment.tangent);
            
            if (i != 0 || u != 0) { // For subsequent points, calculate normal and binormal based on previous
                segment.normal = crossProduct(prevBinormal, segment.tangent);
                segment.normal = normalize(segment.normal);
                segment.binormal = crossProduct(segment.tangent, segment.normal);
                segment.binormal = normalize(segment.binormal);
            } else { // For the very first point
                segment.normal = crossProduct(segment.tangent, initialV);
                segment.normal = normalize(segment.normal);
                segment.binormal = crossProduct(segment.tangent, segment.normal);
                segment.binormal = normalize(segment.binormal);
            }
            
            // Prepare for next segment
            // prevNormal = segment.normal;
            prevBinormal = segment.binormal;
            
            splineSegments.push_back(segment);
        }
    }
}

void addTriangle1(Point posA, Point posB, Point posC)
{
  pos.push_back(posA.x); pos.push_back(posA.y); pos.push_back(posA.z);
  pos.push_back(posB.x); pos.push_back(posB.y); pos.push_back(posB.z);
  pos.push_back(posC.x); pos.push_back(posC.y); pos.push_back(posC.z);
  Point vector1;
  Point vector2;
  vector1.x = posB.x - posA.x;
  vector1.y = posB.y - posA.y;
  vector1.z = posB.z - posA.z;
  vector2.x = posC.x - posA.x;
  vector2.y = posC.y - posA.y;
  vector2.z = posC.z - posA.z;

  Point ResultNormal = crossProduct(normalize(vector1), normalize(vector2));
  ResultNormal = normalize(ResultNormal);
  colorVector.push_back(ResultNormal.x); colorVector.push_back(ResultNormal.y); colorVector.push_back(ResultNormal.z);
  colorVector.push_back(ResultNormal.x); colorVector.push_back(ResultNormal.y); colorVector.push_back(ResultNormal.z);
  colorVector.push_back(ResultNormal.x); colorVector.push_back(ResultNormal.y); colorVector.push_back(ResultNormal.z);
  colorVector.push_back(ResultNormal.x); colorVector.push_back(ResultNormal.y); colorVector.push_back(ResultNormal.z);
  colorVector.push_back(ResultNormal.x); colorVector.push_back(ResultNormal.y); colorVector.push_back(ResultNormal.z);
  colorVector.push_back(ResultNormal.x); colorVector.push_back(ResultNormal.y); colorVector.push_back(ResultNormal.z);
}

void addTriangle(Point posA, Point posB, Point posC)
{
  pos.push_back(posA.x); pos.push_back(posA.y); pos.push_back(posA.z);
  pos.push_back(posB.x); pos.push_back(posB.y); pos.push_back(posB.z);
  pos.push_back(posC.x); pos.push_back(posC.y); pos.push_back(posC.z);
}

float* convert(Point& v) {
  float* Result = new float[3]; // Dynamically allocate memory for the array
  Result[0] = static_cast<float>(v.x); // Populate the array
  Result[1] = static_cast<float>(v.y);
  Result[2] = static_cast<float>(v.z);
  return Result; // Return the pointer to the array
}

void produceRailSurface() {
  float alpha = 0.10f;

  for (int i = 0; i < splineSegments.size() - 1; i++) {
    Point point1 = splineSegments[i].position;
    Point point2 = splineSegments[i+1].position;
    Point normal1 = splineSegments[i].normal;
    Point normal2 = splineSegments[i+1].normal;
    Point binormal1 = splineSegments[i].binormal;
    Point binormal2 = splineSegments[i+1].binormal;

    Point v0, v1, v2, v3, v4, v5, v6, v7;

    v0.x = point1.x + alpha * (-normal1.x + binormal1.x);
    v0.y = point1.y + alpha * (-normal1.y + binormal1.y);
    v0.z = point1.z + alpha * (-normal1.z + binormal1.z);

    v1.x = point1.x + alpha * (normal1.x + binormal1.x);
    v1.y = point1.y + alpha * (normal1.y + binormal1.y);
    v1.z = point1.z + alpha * (normal1.z + binormal1.z);

    v2.x = point1.x + alpha * (normal1.x - binormal1.x);
    v2.y = point1.y + alpha * (normal1.y - binormal1.y);
    v2.z = point1.z + alpha * (normal1.z - binormal1.z);

    v3.x = point1.x + alpha * (-normal1.x - binormal1.x);
    v3.y = point1.y + alpha * (-normal1.y - binormal1.y);
    v3.z = point1.z + alpha * (-normal1.z - binormal1.z);

    v4.x = point2.x + alpha * (-normal2.x + binormal2.x);
    v4.y = point2.y + alpha * (-normal2.y + binormal2.y);
    v4.z = point2.z + alpha * (-normal2.z + binormal2.z);

    v5.x = point2.x + alpha * (normal2.x + binormal2.x);
    v5.y = point2.y + alpha * (normal2.y + binormal2.y);
    v5.z = point2.z + alpha * (normal2.z + binormal2.z);

    v6.x = point2.x + alpha * (normal2.x - binormal2.x);
    v6.y = point2.y + alpha * (normal2.y - binormal2.y);
    v6.z = point2.z + alpha * (normal2.z - binormal2.z);

    v7.x = point2.x + alpha * (-normal2.x - binormal2.x);
    v7.y = point2.y + alpha * (-normal2.y - binormal2.y);
    v7.z = point2.z + alpha * (-normal2.z - binormal2.z);

    // top
    addTriangle1(v1, v5, v6);
    addTriangle(v1, v2, v6);
    // right
    addTriangle1(v0, v1, v5);
    addTriangle(v0, v4, v5);
    // left
    addTriangle1(v2, v3, v6);
    addTriangle(v3, v6, v7);
    // bottom
    addTriangle1(v0, v3, v7);
    addTriangle(v0, v4, v7);
 }
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
  // Read the texture image.
  ImageIO img;
  ImageIO::fileFormatType imgFormat;
  ImageIO::errorType err = img.load(imageFilename, &imgFormat);

  if (err != ImageIO::OK) 
  {
    printf("Loading texture from %s failed.\n", imageFilename);
    return -1;
  }

  // Check that the number of bytes is a multiple of 4.
  if (img.getWidth() * img.getBytesPerPixel() % 4) 
  {
    printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
    return -1;
  }

  // Allocate space for an array of pixels.
  int width = img.getWidth();
  int height = img.getHeight();
  unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA

  // Fill the pixelsRGBA array with the image pixels.
  memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
  for (int h = 0; h < height; h++)
    for (int w = 0; w < width; w++) 
    {
      // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
      pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
      pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
      pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
      pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque

      // set the RGBA channels, based on the loaded image
      int numChannels = img.getBytesPerPixel();
      for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
        pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
    }

  // Bind the texture.
  glBindTexture(GL_TEXTURE_2D, textureHandle);

  // Initialize the texture.
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);

  // Generate the mipmaps for this texture.
  glGenerateMipmap(GL_TEXTURE_2D);

  // Set the texture parameters.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Query support for anisotropic texture filtering.
  GLfloat fLargest;
  glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
  printf("Max available anisotropic samples: %f\n", fLargest);
  // Set anisotropic texture filtering.
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);

  // Query for any errors.
  GLenum errCode = glGetError();
  if (errCode != 0) 
  {
    printf("Texture initialization error. Error code: %d.\n", errCode);
    return -1;
  }
  
  // De-allocate the pixel array -- it is no longer needed.
  delete [] pixelsRGBA;

  return 0;
}

// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

void setCameraLookAt(const Point& eyePoint, const Point& focusPoint, const Point& upVector) {
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    
    matrix.LoadIdentity();
    
    matrix.LookAt(eyePoint.x, eyePoint.y, eyePoint.z,
                  focusPoint.x, focusPoint.y, focusPoint.z,
                  upVector.x, upVector.y, upVector.z);
}

void updateCamera(int u) {
    int index = u % splineSegments.size();
    SplineSegment& segment = splineSegments[index];
    SplineSegment& segment2 = splineSegments[index+1];
    
    Point eyePoint = segment.position; // The point where the camera is currently located
    eyePoint.x += 0.5 * segment.normal.x;
    eyePoint.y += 0.5 * segment.normal.y;
    eyePoint.z += 0.5 * segment.normal.z;

    Point focusPoint =  segment2.position;
    focusPoint.x += 0.5 * segment.tangent.x; 
    focusPoint.y += 0.5 * segment.tangent.y; 
    focusPoint.z += 0.5 * segment.tangent.z; 

    Point up = segment.normal; // The camera's up vector

    //cout << eyePoint.x << " " << eyePoint.y << endl;
    // Assuming you have a method to set your camera's position and orientation
    setCameraLookAt(eyePoint, focusPoint, up);
}

bool stop = false;
int screenShotsNum = 0;

void idleFunc()
{
  // Do some stuff...
  // For example, here, you can save the screenshots to disk (to make the animation).
  // float speed = 0.5f; // Control the speed of the camera
  // u += speed;
  // if (u * 100 > splineSegments.size()) u -= 1.0 * splineSegments.size() / 100; // Loop back to the start for continuous motion
  
  //updateCamera(u); // Update camera based on current u
  if (screenShotsNum >= 525) {
    stop = true;
  }
  if (!stop) {
    char filename[1024];
    sprintf(filename, "Screenshots/screenshoot_%d.jpg", screenShotsNum);

    saveScreenshot(filename);

    screenShotsNum++;
  }
  glutPostRedisplay();
  
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar. 
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 0.1f;
  const float zFar = 1000.0f;
  const float humanFieldOfView = 60.0f;
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the terrain
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        terrainTranslate[0] += mousePosDelta[0] * 0.01f;
        terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        terrainTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the terrain
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        terrainRotate[0] += mousePosDelta[1];
        terrainRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        terrainRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the terrain
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // If CTRL and SHIFT are not pressed, we are in rotate mode.
    default:
      controlState = ROTATE;
    break;
  }

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // Take a screenshot.
      saveScreenshot("screenshot.jpg");
    break;
  }

  glutPostRedisplay();
}

void sky() {
  std::vector<float> PosForGround = {
      -100.0f, -100.0f, -100.0f, // Vertex 1
      100.0f, -100.0f, -100.0f,  // Vertex 2
      -100.0f, -100.0f, 100.0f,  // Vertex 3
      100.0f, -100.0f, -100.0f,  // Vertex 4
      -100.0f, -100.0f, 100.0f,  // Vertex 5 
      100.0f, -100.0f, 100.0f    // Vertex 6
  };

  std::vector<float> UVs = {
      0, 1, // UV for Vertex 1
      1, 1, // UV for Vertex 2
      0, 0, // UV for Vertex 3
      1, 1, // UV for Vertex 4
      0, 0, // UV for Vertex 5
      1, 0  // UV for Vertex 6
  };

  //textPipelineProgram = new PipelineProgram();
  textPipelineProgram.Bind();

  // set the texture image
  glActiveTexture(GL_TEXTURE0);
  GLint textureImage = glGetUniformLocation(textPipelineProgram.GetProgramHandle(), "textureImage");
  glUniform1i(textureImage, GL_TEXTURE0 - GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandleSun);

  // create buffer for ground positions and uvs
  glGenBuffers(1, &vboGround);
  glBindBuffer(GL_ARRAY_BUFFER, vboGround);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (PosForGround.size() + UVs.size()), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, PosForGround.size() * sizeof(float), PosForGround.data());
  glBufferSubData(GL_ARRAY_BUFFER, PosForGround.size() * sizeof(float), UVs.size() * sizeof(float), UVs.data());

  // create VAO 
  //textPipelineProgram.Bind();
  glGenVertexArrays(1,&vaoGround);
  glBindVertexArray(vaoGround);
  glBindBuffer(GL_ARRAY_BUFFER, vboGround);

  // set up pointer to required data 
  GLuint indexOfPosition = glGetAttribLocation(textPipelineProgram.GetProgramHandle(), "position");
  glEnableVertexAttribArray(indexOfPosition);
  const void* offset = (const void*) 0;
  glVertexAttribPointer(indexOfPosition, 3, GL_FLOAT, GL_FALSE, 0, offset);

  GLuint indexOfUVs = glGetAttribLocation(textPipelineProgram.GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(indexOfUVs);
  offset = (const void*) (size_t)(sizeof(float) * PosForGround.size());
  glVertexAttribPointer(indexOfUVs, 2, GL_FLOAT, GL_FALSE, 0, offset);

  // set the modelView and projection
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  textPipelineProgram.Bind();
  textPipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  textPipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // draw 
  glBindVertexArray(vaoGround); 
  glDrawArrays(GL_TRIANGLES, 0, PosForGround.size());
  
  glBindVertexArray(0); 
}

void right() {
  std::vector<float> PosForGround = {
      -20.0f, 20.0f, -20.0f, // Vertex 1
      -20.0f, -20.0f, -20.0f,  // Vertex 2
      -20.0f, 20.0f, 20.0f,  // Vertex 3
      -20.0f, -20.0f, -20.0f,  // Vertex 2
      -20.0f, 20.0f, 20.0f,  // Vertex 3 
      -20.0f, -20.0f, 20.0f    // Vertex 6
  };

  std::vector<float> UVs = {
      0, 1, // UV for Vertex 1
      1, 1, // UV for Vertex 2
      0, 0, // UV for Vertex 3
      1, 1, // UV for Vertex 4
      0, 0, // UV for Vertex 5
      1, 0  // UV for Vertex 6
  };

  //textPipelineProgram = new PipelineProgram();
  textPipelineProgram.Bind();

  // set the texture image
  glActiveTexture(GL_TEXTURE0);
  GLint textureImage = glGetUniformLocation(textPipelineProgram.GetProgramHandle(), "textureImage");
  glUniform1i(textureImage, GL_TEXTURE0 - GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandleMoon);

  // create buffer for ground positions and uvs
  glGenBuffers(1, &vboGround);
  glBindBuffer(GL_ARRAY_BUFFER, vboGround);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (PosForGround.size() + UVs.size()), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, PosForGround.size() * sizeof(float), PosForGround.data());
  glBufferSubData(GL_ARRAY_BUFFER, PosForGround.size() * sizeof(float), UVs.size() * sizeof(float), UVs.data());

  // create VAO 
  //textPipelineProgram.Bind();
  glGenVertexArrays(1,&vaoGround);
  glBindVertexArray(vaoGround);
  glBindBuffer(GL_ARRAY_BUFFER, vboGround);

  // set up pointer to required data 
  GLuint indexOfPosition = glGetAttribLocation(textPipelineProgram.GetProgramHandle(), "position");
  glEnableVertexAttribArray(indexOfPosition);
  const void* offset = (const void*) 0;
  glVertexAttribPointer(indexOfPosition, 3, GL_FLOAT, GL_FALSE, 0, offset);

  GLuint indexOfUVs = glGetAttribLocation(textPipelineProgram.GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(indexOfUVs);
  offset = (const void*) (size_t)(sizeof(float) * PosForGround.size());
  glVertexAttribPointer(indexOfUVs, 2, GL_FLOAT, GL_FALSE, 0, offset);

  // set the modelView and projection
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  textPipelineProgram.Bind();
  textPipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  textPipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // draw 
  glBindVertexArray(vaoGround); 
  glDrawArrays(GL_TRIANGLES, 0, PosForGround.size());
  
  glBindVertexArray(0); 
}

void ground() {
  std::vector<float> PosForGround = {
      100.0f, 100.0f, 100.0f, // Vertex 1
      -100.0f, 100.0f, 100.0f,  // Vertex 2
      100.0f, 100.0f, -100.0f,  // Vertex 3
      -100.0f, 100.0f, 100.0f,  // Vertex 4
      100.0f, 100.0f, -100.0f,  // Vertex 5 
      -100.0f, 100.0f, -100.0f    // Vertex 6
  };

  std::vector<float> UVs = {
      0, 1, // UV for Vertex 1
      1, 1, // UV for Vertex 2
      0, 0, // UV for Vertex 3
      1, 1, // UV for Vertex 4
      0, 0, // UV for Vertex 5
      1, 0  // UV for Vertex 6
  };

  //textPipelineProgram = new PipelineProgram();
  textPipelineProgram.Bind();

  // set the texture image
  glActiveTexture(GL_TEXTURE0);
  GLint textureImage = glGetUniformLocation(textPipelineProgram.GetProgramHandle(), "textureImage");
  glUniform1i(textureImage, GL_TEXTURE0 - GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandle);

  // create buffer for ground positions and uvs
  glGenBuffers(1, &vboGround);
  glBindBuffer(GL_ARRAY_BUFFER, vboGround);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (PosForGround.size() + UVs.size()), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, PosForGround.size() * sizeof(float), PosForGround.data());
  glBufferSubData(GL_ARRAY_BUFFER, PosForGround.size() * sizeof(float), UVs.size() * sizeof(float), UVs.data());

  // create VAO 
  //textPipelineProgram.Bind();
  glGenVertexArrays(1,&vaoGround);
  glBindVertexArray(vaoGround);
  glBindBuffer(GL_ARRAY_BUFFER, vboGround);

  // set up pointer to required data 
  GLuint indexOfPosition = glGetAttribLocation(textPipelineProgram.GetProgramHandle(), "position");
  glEnableVertexAttribArray(indexOfPosition);
  const void* offset = (const void*) 0;
  glVertexAttribPointer(indexOfPosition, 3, GL_FLOAT, GL_FALSE, 0, offset);

  GLuint indexOfUVs = glGetAttribLocation(textPipelineProgram.GetProgramHandle(), "texCoord");
  glEnableVertexAttribArray(indexOfUVs);
  offset = (const void*) (size_t)(sizeof(float) * PosForGround.size());
  glVertexAttribPointer(indexOfUVs, 2, GL_FLOAT, GL_FALSE, 0, offset);

  // set the modelView and projection
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  textPipelineProgram.Bind();
  textPipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  textPipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // draw 
  glBindVertexArray(vaoGround); 
  glDrawArrays(GL_TRIANGLES, 0, PosForGround.size());
  
  glBindVertexArray(0); 
}

void phongShading() {
  pipelineProgram->Bind();
  float view[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(view);

  float lightDirection[4] = {0.0f, 1.0f, 0.0f, 0.0f}; // the “Sun” at noon
  float viewLightDirection[3]; // light direction in the view space
  float tempOutput[4];

  MultiplyMatricesForFloat(4, 4, 1, view, lightDirection, tempOutput);
  viewLightDirection[0] = tempOutput[0];
  viewLightDirection[1] = tempOutput[1];
  viewLightDirection[2] = tempOutput[2];

  //cout << viewLightDirection[0] << viewLightDirection[1] << viewLightDirection[2] << endl;

  GLint viewLightDirectionLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "viewLightDirection");
  glUniform3fv(viewLightDirectionLocation, 1, viewLightDirection);

  GLint LaLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "La");
  float La[4] = {0.8, 0.7, 0.6, 1.0f};
  glUniform4fv(LaLocation, 1, La);

  GLint LdLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ld");
  float Ld[4] = {0.8, 0.7, 0.6, 1.0f};
  glUniform4fv(LdLocation, 1, Ld);

  GLint LsLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "Ls");
  float Ls[4] = {0.8, 0.7, 0.6, 1.0f};
  glUniform4fv(LsLocation, 1, Ls); 

  GLint kaLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ka");
  float ka[4] = {0.8, 0.2, 0.2, 1.0f};
  glUniform4fv(kaLocation, 1, ka);

  GLint kdLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "kd");
  float kd[4] = {0.8, 0.2, 0.2, 1.0f};
  glUniform4fv(kdLocation, 1, kd);

  GLint ksLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "ks");
  float ks[4] = {0.8, 0.2, 0.2, 1.0f};
  glUniform4fv(ksLocation, 1, ks);

  GLint alphaLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "alpha");
  glUniform1f(alphaLocation, 1.0f);

  GLint normalMatrixLocation = glGetUniformLocation(pipelineProgram->GetProgramHandle(), "normalMatrix");
  float n[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(n); // get normal matrix
  glUniformMatrix4fv(normalMatrixLocation, 1, GL_FALSE, n);

  //glutSwapBuffers();
}

void displayFunc()
{
  // This function performs the actual rendering.
  // First, clear the screen.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // // Set up the camera position, focus point, and the up vector.
  // matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  // matrix.LoadIdentity();
  // matrix.LookAt(0, 2, 0.2,
  //               0.0, 0, 0,
  //               0.0, 1, 0);

  updateCamera(u); // Update camera based on current u
  u += 19;

  // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
  matrix.Rotate(terrainRotate[0], 1.0f, 0.0f, 0.0f);
  matrix.Rotate(terrainRotate[1], 0.0f, 1.0f, 0.0f);
  matrix.Rotate(terrainRotate[2], 0.0f, 0.0f, 1.0f);
  matrix.Scale(terrainScale[0],terrainScale[1],terrainScale[2]);

  // Read the current modelview and projection matrices from our helper class.
  // The matrices are only read here; nothing is actually communicated to OpenGL yet.
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);
  
  pipelineProgram->Bind();
  pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  phongShading();

  //cout << "Positions size is " << pos.size() << endl;
  int numVertices = pos.size() / 3;

  // Assuming pos is a std::vector<float> containing your vertex positions
  vboVertices = new VBO(numVertices, 3, &pos[0], GL_STATIC_DRAW);

  //vboVertices = new VBO(numVertices, 3, positions, GL_STATIC_DRAW); // 3 values per position
  //vboColors = new VBO(numVertices, 4, colors, GL_STATIC_DRAW); // 4 values per color
  // Assuming colorVector is a std::vector<float> containing your vertex colors
  vboColors = new VBO(numVertices, 3, &colorVector[0], GL_STATIC_DRAW);

  // Create the VAOs. There is a single VAO in this example.
  // Important: this code must be executed AFTER we created our pipeline program, and AFTER we set up our VBOs.
  // A VAO contains the geometry for a single object. There should be one VAO per object.
  // In this homework, "geometry" means vertex positions and colors. In homework 2, it will also include
  // vertex normal and vertex texture coordinates for texture mapping.
  vao = new VAO();

  // Set up the relationship between the "position" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.
  vao->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVertices, "position");

  // Set up the relationship between the "color" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.
  vao->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboColors, "normal");

  // We don't need this data any more, as we have already uploaded it to the VBO. And so we can destroy it, to avoid a memory leak.
  // free(positions);
  // free(colors);

  vao->Bind();
  glDrawArrays(GL_TRIANGLES, 0, numVertices);

  ground();
  sky();
  right();

  // Swap the double-buffers.
  glutSwapBuffers();
}

void initScene(int argc, char *argv[])
{
  // Load spline from the provided filename.
  loadSpline(argv[1]);

  printf("Loaded spline with %d control point(s).\n", spline.numControlPoints);

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  // create an integer handle for the texture
  glGenTextures(1, &texHandle);
  int code = initTexture("earth.jpg", texHandle);
  if (code != 0) {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glGenTextures(1, &texHandleSun);
  code = initTexture("sun.jpg", texHandleSun);
  if (code != 0) {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glGenTextures(1, &texHandleMoon);
  code = initTexture("moon.jpg", texHandleMoon);
  if (code != 0) {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  glGenTextures(1, &texHandleGalaxy);
  code = initTexture("jupiter.jpg", texHandleGalaxy);
  if (code != 0) {
    printf("Error loading the texture image.\n");
    exit(EXIT_FAILURE);
  }

  constructSplineSegments(spline);
  produceRailSurface();

  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;

  pipelineProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
  // Load and set up the pipeline program, including its shaders.
  if (pipelineProgram->BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  } 
  cout << "Successfully built the pipeline program." << endl;

  textPipelineProgram.Init(shaderBasePath);
}

int main(int argc, char *argv[])
{
  if (argc < 2) {  
    cout << "The arguments are incorrect." << endl;
    printf ("Usage: %s <spline file>\n", argv[0]);
    exit(0);
  }

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);  
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // Tells GLUT to use a particular display function to redraw.
  glutDisplayFunc(displayFunc);
  // Perform animation inside idleFunc.
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}

