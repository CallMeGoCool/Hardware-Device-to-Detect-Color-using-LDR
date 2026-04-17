# Hardware-Device-to-Detect-Color-using-LDR
Hardware device to detect color of any object using an RGB light emitter and LDR to check variances of intenstiy of light reflection to determine color.

So devices needed would be a RGB led or if not available you can use 3 different R, G and B leds, like how we did in this project, LDR's, a small boz, an arduino or raspberry pi (whichever's available).

Steps:
1. Take the small box and make a small cutout at a side from where u want to place the object whose color needs to be determined.
2. 2-3 LDR can be placed around the inside edges of a box close to the cutout so that light reflected from the LDR's can be read effectively.
3. Place the RGB led slightly more upwards/higher than the LDR's and ensure that they are at an equal distance drom the LDR to avoid any higher intenrsity region.
4. Upload the code and run it.
5. For raspberry pi users, the code remains similar, however a basic ML model needs to be trained on sets with values of intensity of light reflected from RGB led's and what color is the user actually showing. 
