Subject 	: CSCI420 - Computer Graphics 
Assignment 2: Simulating a Roller Coaster
Author		: <Yuhang(Logan) Song>
USC ID 		: <8693237616>

How to compile and run:
1. Unzip the file
2. Open the hw2-starterCode folder -> go to hw2 folder -> hw2.cpp is in this folder and contains most of my code for hw2. 
3. cd to the hw2 folder and type "make" in the terminal and run with "./hw2 splines/rollerCoaster.sp"
4. The screenshots will be saved to the folder in "Screenshots" in hw2 folder
ps: other than the hw2.cpp, others files(shader, new pipeline program) I modify are in the hw2-starterCode folder -> openGLHelper folder

Description: In this assignment, we use Catmull-Rom splines along with OpenGL core profile shader-based texture mapping and Phong shading to create a roller coaster simulation.

Core Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Uses OpenGL core profile, version 3.2 or higher - Y

2. Completed all Levels:
  Level 1 : - Y
  level 2 : - Y
  Level 3 : - Y
  Level 4 : - Y
  Level 5 : - Y

3. Rendered the camera at a reasonable speed in a continuous path/orientation - Y

4. Run at interactive frame rate (>15fps at 1280 x 720) - Y

5. Understandably written, well commented code - Y

6. Attached an Animation folder containing not more than 1000 screenshots - Y

7. Attached this ReadMe File - Y

Extra Credit Features: (Answer these Questions with Y/N; you can also insert comments as appropriate)
======================

1. Render a T-shaped rail cross section -

2. Render a Double Rail -

3. Made the track circular and closed it with C1 continuity -

4. Any Additional Scene Elements? (list them here) -

5. Render a sky-box - Y (Other than rendering the ground (i.e. earth in my screenshot), I also render two more faces, i.e. the sun and the moon)

6. Create tracks that mimic real world roller coaster -

7. Generate track from several sequences of splines -

8. Draw splines using recursive subdivision -

9. Render environment in a better manner - 

10. Improved coaster normals -

11. Modify velocity with which the camera moves -

12. Derive the steps that lead to the physically realistic equation of updating u -

Additional Features: (Please document any additional features you may have implemented other than the ones described above)
1. 
2.
3.

Open-Ended Problems: (Please document approaches to any open-ended problems that you have tackled)
1. The most difficult bug for my program during the process is the level 3 surface looks weird initially, but after I changed the logic I put normal into the colorVector. It is solved!!
2. I really enjoy the level4 because I learned how to do texture mapping and when I render the earth at that moment, it amaze me at that moment.
3. Level5 is also interesting when I finish the phong shading and see the result. Love this assignment haha! 

Keyboard/Mouse controls: (Please document Keyboard/Mouse controls if any)
1. ESC means exit
2. spacebar 
3. x (screenshot)

Names of the .cpp files you made changes to:
1. hw2/hw2.cpp
2. openGLHelper/texturePipelineProgram.cpp
3.

Comments : (If any)

