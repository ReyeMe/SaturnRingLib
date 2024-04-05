# SGL 3.0.2j

## Additional SGL Functions and New Features
SGL 3.00 adds new features to the current SGL. This document describes the updated SGL function specifications.

## Additionally, the differences from SGL 3.00 to 3.02
1) Optimization of the function slPutPolygonX. Modified the algorithm when using real time gouraud shading.
2) Bug fix in the function slLine1ColSet.
3) Light source calculation when using 16 color look-up table
4) Fixed unfit portion regarding the VirtualGun (conditions under 20 frames/sec)

## Additional structures
```c
typedef struct {
  POINT* pntbl;		/* Vertex position data table */
  Uint32 nbPoint;	/* Number of vertexes */
  POLYGON* pltbl;	/* Polygon definition table */
  Uint32 nbPolygon;	/* Number of polygons */
  ATTR* attbl;		/* Polygon attribute table */
  VECTOR* vntbl;	/* Normal line vector data for vertexes*/
} XPDATA;
```
## Additional macro definitions
```c
#define	UseNearClip (1<<6) /* Execute near clipping */
#define	UseGouraud  (1<<7) /* Real Gouraud */
```
## Additional function specifications
### FUNCTION: slInitGouraud
```c
void slInitGouraud(GOURAUDTBL* gtbl, Uint32 gmax, Uint32 gaddr)
```
__Description:__ This function initializes the real-time Gouraud. *GOURAUDTBL* 
requires an area for the *gmax* vertexes. This area becomes the work area 
during Gouraud table creation. *gaddr* passes the VRAM offset address (/8) of 
VDP1.
### FUNCTION: slPutPolygonX
```c
void slPutPolygonX(XPDATA* xpdata, FIXED* light)
```
__Description:__ Using the current matrix, this function uses rotates or horizontally moves the given 
polygon model, converts the perspective, and executes light source processing. The function 
then displays the model as a polygon and a transformation sprite.

This function uses the real-time Gouraud features.

> __Note:__  Before executing this function, always use the *slInitGouraud* function to initialize the real-time Gouraud.
### FUNCTION: slGouraudTblCopy
```c
void slGouraudTblCopy(void);
```
__Description:__ This function transfers the Gouraud table created in real-time to VDP1. The Gouraud table is created by *slPutPolygonX*.

> __Note:__ This function transfers the Gouraud table without waiting for completion of the drawing. Either register this function to the *slIntFunction* for execution, or wait until the drawing is completed before executing this function. 
### FUNCTION: slSetGouraudTbl
```c
void slSetGouraudTbl(Uint16* tbl);
```
__Description:__ This function specifies the Gouraud table to be referenced. This function is used to output an original texture.

__Example:__
```c
#define GRTBL(r,g,b)  (((b&0x1f)<<10) | ((g&0x1f)<<5) | (r&0x1f))
static Uint16 GourTbl[32] = {
  GRTBL(0,16,16),GRTBL(1,16,16),GRTBL(2,16,16),
  GRTBL(3,16,16),GRTBL(4,16,16),GRTBL(5,16,16),
  GRTBL(6,16,16),GRTBL(7,16,16),GRTBL(8,16,16),
  GRTBL(9,16,16),GRTBL(10,16,16),GRTBL(11,16,16),
  GRTBL(12,16,16),GRTBL(13,16,16),GRTBL(14,16,16),
  GRTBL(15,16,16),GRTBL(16,16,16),GRTBL(17,16,16),
  GRTBL(18,16,16),GRTBL(19,16,16),GRTBL(20,16,16),
  GRTBL(21,16,16),GRTBL(22,16,16),GRTBL(23,16,16),
  GRTBL(24,16,16),GRTBL(25,16,16),GRTBL(26,16,16),
  GRTBL(27,16,16),GRTBL(28,16,16),GRTBL(29,16,16),
  GRTBL(30,16,16),GRTBL(31,16,16),
};
slSetGouraudTbl(GourTbl);
```
### FUNCTION: slSetGouraudColor
```c
void slSetGouraudColor(Uint16 col);
```
__Description:__ This function specifies the light source color.

__Example:__
```c
Uint16 gr_data = ((gr_b)<<10)+((gr_b>>2)<<5)+(gr_g>>2);
slSetGouraudColor(gr_data);
```
### FUNCTION: slSetAmbient
```c
void slSetAmbient(Uint16ambi);
```
__Description:__ This function sets the lowest value of the Gouraud table. With the standard values, R, G, and B each change from 15 to -16. However, suppose that this function is used to set the following: R = -10, G = -5, and B = 0
<br/>In this case, R changes from 15 to -10.  A value of -10 is set to the table that 
usually changes up to -16.

In other words, the minimum value is inherited by the lowest 6 levels of the 32 
levels. This setting can be used to prevent the specified value from becoming 
too dark and is set as the environment light.

> __Note:__ This function is not effective for tables that are set directly by the *slSetGouraudTbl* function.

__Example:__
```c
slSetAmbient(C_RGB(1,1,1));
```
## Procedure for Using a Real-time Gouraud
The four processes described below are necessary for implementation of a real-
time Gouraud.

* __Add the following three items to the attribute table.__
    1) Specify "__CL_Gouraud__" in MODE.
    2) Specify the storage position of the Gouraud table in __GOURAUD__.
    3) Specify "__UseGouraud__" in OPTION.

        __Example:__
        ```c
        ATTRIBUTE(Single_Plane, SORT_CEN, 0, 0xffa0, 0xe000, CL16Look | MESHoff | CL_Gouraud, sprNoflip, UseGouraud)
        ```

* __Specify the normal line vector.__ <br/>
    Specify the normal vector for each vertex and the polygon mode data table called "__XPDATA__."

    __Example:__
    ```c
    VECTOR gr_point[] = {	/* Normal line vector data for vertexes */
        toFIXED(-0.873),toFIXED(0.436),toFIXED(0.217),
        toFIXED(-0.873),toFIXED(-0.436),toFIXED(0.217),
        // ...
    };

    XPDATA PD_BOX = {		/* Model table specification */
        point_BOX,		    /* Vertex position data table */
        sizeof(point_BOX)/sizeof(POINT),	/* Number of vertexes */
        polygon_BOX,		/* Polygon definition table */
        sizeof(polygon_BOX)/sizeof(POLYGON),	/* Number of polygons */
        attribute_BOX2,		/* Polygon attribute table */
        gr_point			/* Normal line vector data for vertexes */
    };
    ```
* __Initialize the real-time Gouraud program.__
    1) Specify the position and size of the Gouraud table to be used.
    
        __Example:__
        ```c
        slInitGouraud((GOURAUDTBL*)gr_tab, (Uint32)300, 0xe000);
        ```
    2) Register the Gouraud program to the V blank.
    
        __Example:__
        ```c
        slIntFunction(slGouraudTblCopy);
        ```

    3) Specify the light source vector.
    
        __Example:__
        ```c
        FIXED light[XYZ];
        light[X] = toFIXED(1.0);
        light[X] = toFIXED(0.0);
        light[X] = toFIXED(0.0);
        slPutPolygonX(&PD_BOXx, (FIXED*)light);
        ```

    4) Example of actual program usage
        ```c
        static GOURAUDTBL gr_tab[300];

        void ss_main(void)
        {
            FIXED light[XYZ];
            
            // ...
            
            /* Real-time Gouraud program initialization */
            slInitGouraud((GOURAUDTBL*)gr_tab, (Uint32)300, 0xe000);
            slIntFunction(slGouraudTblCopy);
            
            // ...
            
            light[X] = toFIXED(1.0);
            light[Y] = toFIXED(0.0);
            light[Z] = toFIXED(0.0);

            while(1)
            {
                // ...

                slPushMatrix();
                {
                    // ...
                    slPutPolygonX(&PD_BOXx, (FIXED*)light);
                }
                slPopMatrix();
                slSynch();
            }
        }
        ```
## Near Clipping
To execute near clipping, specify "__UseNearClip__" in __OPTION__ for the attribute 
table. "__UseNearClip__" cannot be used together with "__UseClip__."

__Example:__
```c
ATTRIBUTE(Single_Plane, SORT_CEN, 1, No_Palet, No_Gouraud, CL32KRGB | MESHoff, sprNoflip, UseNearClip);
```
## Difference between SGL Version 2.1x and NewSGL
* The light source calculation method for flat shading has changed.
    | Light source calculation method in Version 2.1x: | |
    | ------------------------------ | --------------------------------- |
    | Light source and surface angle | 0------45------90-----135-----180 |
    | Gouraud table                  | 31-----16-------0-------0-------0 |
    
    | Light source calculation method in NewSGL: | |
    | ------------------------------ | --------------------------------- |
    | Light source and surface angle | 0------45------90-----135-----180 |
    | Gouraud table                  | 31-----24------16-------8-------0 |
    
