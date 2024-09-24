# Computer-Graphics---Simulating-a-Roller-Coaster

## Overview
This project implements a roller coaster simulation using Catmull-Rom splines along with OpenGL core profile shader-based lighting and texture mapping. The simulation features a first-person view that allows users to "ride" the roller coaster in an immersive environment, showcasing realistic spline-based track rendering, Phong shading, and texture mapping.

## Key Features
- **Catmull-Rom Splines**: Smooth, continuous tracks generated using Catmull-Rom splines. The track is defined by a series of control points, creating an elaborate curve for the coaster.
- **Texture Mapping**: Ground texture mapping using a dedicated shader, enhancing the visual realism of the environment.
- **Phong Shading**: Per-pixel lighting on the roller coaster rails, enhancing the detail and immersion of the simulation.
- **First-Person View**: A camera follows the track, allowing users to experience the ride as if they are on the coaster. The camera orientation is continuously updated to align with the track's tangent vector.
- **Track Cross-Section**: A customizable cross-section rendering for the rails, such as square or circular, giving the coaster a more realistic look.

## Implementation Details

### Spline Rendering
- The roller coaster track is created by evaluating Catmull-Rom splines, defined by four control points and a parameter `u` that varies from 0 to 1. 
- Consecutive splines are drawn by connecting points using OpenGL's `GL_LINES`. 
- A stationary camera setup allows viewing from various angles using keyboard and mouse controls.

### First-Person Camera Movement
- The camera moves at a constant speed along the spline, facing forward based on the spline's tangent vector, and includes a continuously adjusted "up" vector for realistic motion.

### Rail Cross-Section
- The track is rendered as a tube using tangent, normal, and binormal vectors, with customizable cross-section shapes. Vertex colors correspond to the triangle normals, enhancing the visual shading.

### Ground Plane
- The ground plane is rendered using a separate texture shader, distinct from the roller coaster's shading pipeline. This separation allows multiple shaders to coexist in the scene.

### Phong Shading
- Phong shading is used to render the roller coaster rails, providing per-pixel lighting that enhances the detail and depth of the rendered objects.

## Technical Specifications
- **OpenGL Version**: Core profile, version 3.2 or higher, with shader-based rendering only.
- **Shaders**: Implemented two separate shaders—one for texture mapping (ground) and one for Phong shading (rails).
- **Performance**: Designed to run interactively at frame rates above 15fps at 1280x720 resolution.

## Animation
- The project includes an animation sequence that captures the coaster's motion through a series of JPEG images. The animation demonstrates the roller coaster's design and the simulation's immersive elements.

## How to Run This Project
1. Compile the program using the provided Makefile or your preferred setup.
2. Launch the executable to start the simulation.
3. Use keyboard and mouse controls to explore the scene and experience the roller coaster from a first-person perspective.

For more details on the implementation and additional customization, please refer to the included source code comments and shader files.


