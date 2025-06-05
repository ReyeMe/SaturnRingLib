VDP2 can perform various color calculations between scroll Screen layers to 
combine their colors. It is a powerful feature, however as with most things on the Saturn
using it to its fullest potential requires some understanding of its quirks and limitations.
Its not quite as simple as just assigning layers an opacity.

Unfortunately, while the VDP2 manual attempts to describe these behaviors in diagrams,
they can be difficult to follow, especially since they are in black and white!

The VDP2 Color Calculation Sample is designed as an interactive version of some of these 
diagrams, in the hopes that playing around with the settings and veiwing the results in 
real time can give users a more intuitive understanding of the options available.

Below is a set of guided experiments that you can follow along with to highlight
some of the less obvious behaviors of VDP2 color calculation


*********************************************************************************
Demo 1) Top Color Ratios with no Extension

This is the default Color Calculation mode that VDP2 initializes with. When selected, a
Scroll Screen with color calculation turned ON can blend its color with the pixels behind it
in what is essentially a weighted average. The weighting is called the Color Ratio in 
VDP2 documentaion, but in SRL we represent it with a Fixed Point value between 0.0 and 1.0
called the opacity (In the demo menu this displays as a Percentage from 0 to 100%).

In SRL a ScrollScreen will have Color calculation turned on whenever its corresponding
interface is set with an opacity < 1.0 (100% opaque). But this does not mean that a layer
will always be able to display the exact opacity value provided by the user. Hardware limitations
can change how a layer displays its opacity depending on its priority. Specifically, only pixels
that have the highest priority at a screen coordinate will use the opacity. Effectively
this means that color calculation can only "penetrate" one layer deep at any screen coordinate.

Experiments:
 
i) -Start up the sample and examine the image opacities that the demo loads with. Notice that
 When all 3 shapes overlap, only the top 2 (triangle and square) contribute to display.

ii)-Turn the opacity of the Triangle all the way Down to Zero. You will notice that 
 even though it is completely transparent, in areas where it overlaps lower priority layers
 it still prevents the other shape's color calculation, creating an "opaque shadow". This is 
 because a pixel opacity of 0 is not the same as true transparent pixels. The pixel color data 
 is still present in the top level so it forces the pixels beneath it to become 2nd level pixels,
 which never recieve color calculation in non-extended mode. Only turning the display of
 the triangle scroll off completely could prevent this (or setting the scrolls priority to 0).
  
iii)Note that in areas where the shapes don't overlap, they calculate their opacity value against
 the background color.


*********************************************************************************************
Demo 2) 2nd Color Ratios with no Extention

When color calcuation mode is set to UseColorRatios2nd the colors are calculated much 
like before, except now the opacity value of a top layer pixel is determined by the 
opacity taken from the layer beneath it.

Experiments:

i) -press R trigger once to switch to Color Ratios 2nd mode. try adjusting the opacity 
of the triangle and note that the value no longer has any effect on the display. The top layer on 
the screen will never use its own opacity in this mode. 
(the one exception is when triangle is at 100% opacity, but this is due to 
SRL automatically turning the color calculation off at this level)

ii) -Try adjusting the opacity of the circle. notice that it now controls the 
opacities of both the square and triangle in places where they overlap it alone, however 
in the region where all 3 overlap it is still not visible.   

iii)- Note how opacities no longer calculate against the background in areas where the shapes
don't overlap. The background does not have an opacity value ascociated with it, so the shapes 
in front of it can't inherit its opacity.

**********************************************************************************************
Demo 3) Color Addition with no Extension

When Color calculation is set to Use Color Addition, all layers with color calculation turned
on (opacity< 1.0) add the color channels of 2nd layer pixels to the channels of the top layer
pixels instead of using the specific opacity set by the user.  A black pixel adds no color, 
while brighter pixels add more color. 

Experiments:

i) Press R to switch to Color Addition Mode. Note that all of the shapes appear brighter than
 they did with Color Ratios. Color Addition can not darken the resulting image. 
  
  (Why are the shapes brighter even when they don't overlap? While the background may be dark,
  it is actually not set to pure black in this demo. Since its RGB components still contain 
  small nonzero values, these are added to the top layer, making the shapes brighter even though
  the background is darker)

ii) Try adjusting the opacities- Note that opacities < 100% have color addition turned on, but their 
exact opacity level no longer effects the resulting calculation.

iii) Note that the circle is still not visible in the area where all 3 shapes overlap. Color Addition
still only applies between the top 2 layers present at any screen coordinate
   
     
**********************************************************************************************
Demo 4) Color Calculation Extension

All of the modes above can also be used with Extended color cacluation. Extended color calculation
is an option that allows color calculation between up to 3 layers simultaneously instead of
just the top 2. This mode is not always available depending on the color depth of the scrollscreen
layers involved and the CRAM mode selected. An exhaustive list of the conditions required for
extended color calcuation can be found in the VDP2 Users Manual.
When this mode is available and pixels of the 2nd highest priority have color calculation turned 
on, they will apply a 50% opacity calculation with the layers beneath before the combined result is 
caculated against the opacity of the top layer. This effectively allows color calculation to 
penetrate 2 layers deep instead of just 1. 

Experiments:

i)- Press R trigger to switch back to color ratio top mode, then press Z to toggle extended 
 color calculation ON. Notice that in the area where all 3 shapes overlap, the circle is now
 visible behind the others.

ii)- adjust the opacity of the square, Note that in the areas where the square
 is behind the triangle, the opacity is not affected by the adjustment. This is because color 
 calculation on the 2nd layer is always fixed at 0.5 opacity when turned on. Even in this mode,
 only the top layer calculates with the user selected opacities.

iii)- press Z to toggle color extension a few times. Notice that areas in which 2 of the shapes 
overlap appear uniformly darker with extension turned on. This is because the bottom 
shape in these regions is now recieving 50% opacity calculation against the background color 
 
iv) Press R to cycle back to Color Addition mode with Extension turned on. Toggle extension with Z
as above and notice that here too overlapping regions appear slightly darker with extension turned on.
This is because even in addition mode, the lower layer pixels apply 50% percent opacity with the pixels
beneath them instead of adding



