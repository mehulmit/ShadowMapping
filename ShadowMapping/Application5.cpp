#include "stdafx.h"
#include "CS580HW.h"
#include "Application5.h"
#include "Gz.h"
#include "disp.h"
#include "rend.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define INFILE  "ppot.asc"
#define OUTFILE "output.ppm"


extern int tex_fun(float u, float v, GzColor color); /* image texture function */
extern int ptex_fun(float u, float v, GzColor color); /* procedural texture function */

void shade(GzCoord norm, GzCoord color);

float AAfilter[AAKERNAL_SIZE][3] =
{
    { -0.52, 0.38, 0.128 },
    { 0.41, 0.56, 0.119 },
    { 0.27, 0.08, 0.294 },
    { -0.17, -0.29, 0.249 },
    { 0.58, -0.55, 0.104 },
    { -0.31, -0.71, 0.106 }
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Application5::Application5()
{

}

Application5::~Application5()
{

}

int Application5::Initialize()
{
    GzCamera    camera;
    int         xRes, yRes, dispClass;	/* display parameters */ 

    GzToken     nameListShader[9]; 	    /* shader attribute names */
    GzPointer   valueListShader[9];		/* shader attribute pointers */
    GzToken     nameListLights[10];		/* light info */
    GzPointer   valueListLights[10];
    int         shaderType, interpStyle;
    float       specpower;
    int         status; 
 
    status = 0; 

    // Allocate memory for user input
    m_pUserInput = new GzInput;

    // initialize the display and the renderer 
    m_nWidth = 256;     // frame buffer and display width
    m_nHeight = 256;    // frame buffer and display height

    status |= GzNewFrameBuffer(&m_pFrameBuffer, m_nWidth, m_nHeight);

    status |= GzNewDisplay(&m_pDisplay, GZ_RGBAZ_DISPLAY, m_nWidth, m_nHeight);

    status |= GzGetDisplayParams(m_pDisplay, &xRes, &yRes, &dispClass); 

    status |= GzInitDisplay(m_pDisplay); 

    status |= GzNewRender(&m_pRender, GZ_Z_BUFFER_RENDER, m_pDisplay); 

/* Translation matrix */
GzMatrix    scale = 
{ 
	3.25,	0.0,	0.0,	0.0, 
	0.0,	3.25,	0.0,	-3.25, 
	0.0,	0.0,	3.25,	3.5, 
	0.0,	0.0,	0.0,	1.0 
}; 
 
GzMatrix    rotateX = 
{ 
	1.0,	0.0,	0.0,	0.0, 
	0.0,	.7071,	.7071,	0.0, 
	0.0,	-.7071,	.7071,	0.0, 
	0.0,	0.0,	0.0,	1.0 
}; 
 
GzMatrix    rotateY = 
{ 
	.866,	0.0,	-0.5,	0.0, 
	0.0,	1.0,	0.0,	0.0, 
	0.5,	0.0,	.866,	0.0, 
	0.0,	0.0,	0.0,	1.0 
}; 

#if 1   // Set up app-defined camera if desired, else use camera defaults
    camera.position[X] = 20; 
    camera.position[Y] = 30;
    camera.position[Z] = -40;

    camera.lookat[X] = 0;
    camera.lookat[Y] = 0;
    camera.lookat[Z] = 0;
    camera.worldup[X] = 0;
    camera.worldup[Y] = 1.0;
    camera.worldup[Z] = 0.0;

    camera.FOV = 53.7; // degrees

    status |= GzPutCamera(m_pRender, &camera); 
#endif 

    /* Start Renderer */
    status |= GzBeginRender(m_pRender);

    /* Light */
    GzLight light1 = { {0.7071, 0, -0.7071}, {0.5, 0.5, 0.9} };

    light1.isCastShadows = true;
    light1.position[0] = 20;
    light1.position[1] = 0.0;
    light1.position[2] = -40;
    light1.lookat[0] = 0;
    light1.lookat[1] = 0;
    light1.lookat[2] = 0;
    light1.FOV = 70;
    light1.worldup[0] = 0;
    light1.worldup[1] = 1;
    light1.worldup[2] = 0;

    light1.color[0]     = 0.2;
    light1.color[1]     = 0.7;
    light1.color[2]     = 0.3;

    GzLight light2 = { {0, -0.7071, -0.7071}, {0.9, 0.2, 0.3} };
    light2.isCastShadows = false;

    GzLight light3 = { {0.7071, 0.0, -0.7071}, {0.2, 0.7, 0.3} };
    light3.isCastShadows = false;

    GzLight ambientlight = { {0, 0, 0}, {0.3, 0.3, 0.3} };

    /* Material property */
    GzColor specularCoefficient = { 0.3, 0.3, 0.3 };
    GzColor ambientCoefficient = { 0.1, 0.1, 0.1 };
    GzColor diffuseCoefficient = {0.7, 0.7, 0.7};

    // Tokens associated with light parameters
    nameListLights[0]   = GZ_DIRECTIONAL_LIGHT;
    valueListLights[0]  = (GzPointer)&light1;
    nameListLights[1]   = GZ_DIRECTIONAL_LIGHT;
    valueListLights[1]  = (GzPointer)&light2;
    nameListLights[2]   = GZ_DIRECTIONAL_LIGHT;
    valueListLights[2]  = (GzPointer)&light3;
    status |= GzPutAttribute(m_pRender, 3, nameListLights, valueListLights);

    nameListLights[0]   = GZ_AMBIENT_LIGHT;
    valueListLights[0]  = (GzPointer)&ambientlight;
    status |= GzPutAttribute(m_pRender, 1, nameListLights, valueListLights);

    // Tokens associated with shading 
    nameListShader[0]  = GZ_DIFFUSE_COEFFICIENT;
    valueListShader[0] = (GzPointer)diffuseCoefficient;

    // Select either GZ_COLOR or GZ_NORMALS as interpolation mode
    nameListShader[1]  = GZ_INTERPOLATE;
#if 1
    interpStyle = GZ_NORMALS;         /* Phong shading */
#else
    interpStyle = GZ_COLOR;
#endif

    valueListShader[1] = (GzPointer)&interpStyle;

    nameListShader[2]  = GZ_AMBIENT_COEFFICIENT;
    valueListShader[2] = (GzPointer)ambientCoefficient;
    nameListShader[3]  = GZ_SPECULAR_COEFFICIENT;
    valueListShader[3] = (GzPointer)specularCoefficient;
    nameListShader[4]  = GZ_DISTRIBUTION_COEFFICIENT;
    specpower = 32;
    valueListShader[4] = (GzPointer)&specpower;

    nameListShader[5]  = GZ_TEXTURE_MAP;
#if 1   /* set up null texture function or valid pointer */
    valueListShader[5] = (GzPointer)0;
#else
    valueListShader[5] = (GzPointer)(ptex_fun);	/* or use ptex_fun */
#endif
    status |= GzPutAttribute(m_pRender, 6, nameListShader, valueListShader);

    status |= GzPushMatrix(m_pRender, scale,true);  
    status |= GzPushMatrix(m_pRender, rotateY,true); 
    status |= GzPushMatrix(m_pRender, rotateX,true); 

    if (status)
    {
        exit(GZ_FAILURE);
    }

    if (status)
    {
        return(GZ_FAILURE);
    }
    else
    {
        return(GZ_SUCCESS);
    }
}

int Application5::Render() 
{
    GzDisplay   *antiAliasDisplay[AAKERNAL_SIZE];
    GzRender    *antiAliasRender[AAKERNAL_SIZE];


    GzToken         nameListTriangle[3]; 	/* vertex attribute names */
    GzPointer       valueListTriangle[3]; 	/* vertex attribute pointers */
    GzCoord         vertexList[3];	/* vertex position coordinates */ 
    GzCoord         normalList[3];	/* vertex normals */ 
    GzTextureIndex  uvList[3];		/* vertex texture map indices */ 
    char            dummy[256]; 
    int             status; 

    // Initialize Display
    status |= GzInitDisplay(m_pDisplay); 

    // Tokens associated with triangle vertex values
    nameListTriangle[0] = GZ_POSITION; 
    nameListTriangle[1] = GZ_NORMAL; 
    nameListTriangle[2] = GZ_TEXTURE_INDEX;  

    // I/O File open
    FILE *infile;
    if( (infile  = fopen( INFILE , "r" )) == NULL )
    {
        AfxMessageBox( "The input file was not opened\n" );
        return GZ_FAILURE;
    }

    FILE *outfile;
    if( (outfile  = fopen( OUTFILE , "wb" )) == NULL )
    {
        AfxMessageBox( "The output file was not opened\n" );
        return GZ_FAILURE;
    }

    GzToken     nameListAlias[2] = { GZ_AASHIFTX, GZ_AASHIFTY };
    GzPointer   valueListAlias[2] = {0};

    for(unsigned int index = 0; index < AAKERNAL_SIZE; ++index)
    {
        GzNewDisplay(&antiAliasDisplay[index], m_pDisplay->dispClass, m_pDisplay->xres + 1, m_pDisplay->yres + 1);
        GzInitDisplay(antiAliasDisplay[index]);

        antiAliasRender[index] = (GzRender*) malloc ( sizeof(GzRender) );
        memcpy(antiAliasRender[index], m_pRender, sizeof(GzRender));
        antiAliasRender[index]->display = antiAliasDisplay[index];

        float aliasX = AAfilter[index][0];
        float aliasY = AAfilter[index][1];

        valueListAlias[0] = &aliasX;
        valueListAlias[1] = &aliasY;

        GzPutAttribute(antiAliasRender[index], 2, nameListAlias, valueListAlias);
    }

    // Walk through the list of triangles, set color  and render each triangle
    // Call first time to populate the shadow buffers
    while( fscanf(infile, "%s", dummy) == 1)
    {
        fscanf(infile, "%f %f %f %f %f %f %f %f", 
            &(vertexList[0][0]), &(vertexList[0][1]),  
            &(vertexList[0][2]), 
            &(normalList[0][0]), &(normalList[0][1]), 	
            &(normalList[0][2]), 
            &(uvList[0][0]), &(uvList[0][1]) ); 
            fscanf(infile, "%f %f %f %f %f %f %f %f", 
            &(vertexList[1][0]), &(vertexList[1][1]), 	
            &(vertexList[1][2]), 
            &(normalList[1][0]), &(normalList[1][1]), 	
            &(normalList[1][2]), 
            &(uvList[1][0]), &(uvList[1][1]) ); 
            fscanf(infile, "%f %f %f %f %f %f %f %f", 
            &(vertexList[2][0]), &(vertexList[2][1]), 	
            &(vertexList[2][2]), 
            &(normalList[2][0]), &(normalList[2][1]), 	
            &(normalList[2][2]), 
            &(uvList[2][0]), &(uvList[2][1]) ); 

        /* 
         * Set the value pointers to the first vertex of the
         * triangle, then feed it to the renderer 
         * NOTE: this sequence matches the nameList token sequence
         */ 
        valueListTriangle[0] = (GzPointer)vertexList; 
        valueListTriangle[1] = (GzPointer)normalList; 
        valueListTriangle[2] = (GzPointer)uvList; 

        updateLightBuffers(m_pRender, vertexList);
        GzUpdateShadowBuffers(m_pRender, 3, nameListTriangle, valueListTriangle);
    }

    fclose(infile);
    infile  = fopen( INFILE , "r" );

    //Read the file again for rendering the triangles.
    while( fscanf(infile, "%s", dummy) == 1)
    {
        fscanf(infile, "%f %f %f %f %f %f %f %f", 
            &(vertexList[0][0]), &(vertexList[0][1]),  
            &(vertexList[0][2]), 
            &(normalList[0][0]), &(normalList[0][1]), 	
            &(normalList[0][2]), 
            &(uvList[0][0]), &(uvList[0][1]) ); 
            fscanf(infile, "%f %f %f %f %f %f %f %f", 
            &(vertexList[1][0]), &(vertexList[1][1]), 	
            &(vertexList[1][2]), 
            &(normalList[1][0]), &(normalList[1][1]), 	
            &(normalList[1][2]), 
            &(uvList[1][0]), &(uvList[1][1]) ); 
            fscanf(infile, "%f %f %f %f %f %f %f %f", 
            &(vertexList[2][0]), &(vertexList[2][1]), 	
            &(vertexList[2][2]), 
            &(normalList[2][0]), &(normalList[2][1]), 	
            &(normalList[2][2]), 
            &(uvList[2][0]), &(uvList[2][1]) ); 

        /* 
         * Set the value pointers to the first vertex of the
         * triangle, then feed it to the renderer 
         * NOTE: this sequence matches the nameList token sequence
         */ 
        valueListTriangle[0] = (GzPointer)vertexList; 
        valueListTriangle[1] = (GzPointer)normalList; 
        valueListTriangle[2] = (GzPointer)uvList; 

        for(unsigned int index = 0; index < AAKERNAL_SIZE; ++index)
        {
            GzPutTriangle(antiAliasRender[index], 3, nameListTriangle, valueListTriangle);
        }

        GzPutTriangle(m_pRender, 3, nameListTriangle, valueListTriangle);
    }

#if 1
    unsigned int frameBufferSize = (m_pDisplay->xres + 1) * (m_pDisplay->yres + 1);
    for(unsigned int index = 0; index < frameBufferSize; ++ index)
    {
        m_pDisplay->fbuf[index].red     = 0;
        m_pDisplay->fbuf[index].green   = 0;
        m_pDisplay->fbuf[index].blue    = 0;
        m_pDisplay->fbuf[index].z       = 0;
        m_pDisplay->fbuf[index].alpha   = 0;

        for(unsigned int displayIndex = 0; displayIndex < AAKERNAL_SIZE; ++displayIndex)
        {
            m_pDisplay->fbuf[index].red     += antiAliasDisplay[displayIndex]->fbuf[index].red * AAfilter[displayIndex][2];
            m_pDisplay->fbuf[index].green   += antiAliasDisplay[displayIndex]->fbuf[index].green * AAfilter[displayIndex][2];
            m_pDisplay->fbuf[index].blue    += antiAliasDisplay[displayIndex]->fbuf[index].blue * AAfilter[displayIndex][2];
        }
    }
#endif

    // Write out or update display to file
    GzFlushDisplay2File(outfile, m_pDisplay);

    // Write out or update display to frame buffer
    GzFlushDisplay2FrameBuffer(m_pFrameBuffer, m_pDisplay);

    for(unsigned int displayIndex = 0; displayIndex < AAKERNAL_SIZE; ++displayIndex)
    {
        GzFreeRender(antiAliasRender[displayIndex]);
        antiAliasRender[displayIndex] = NULL;

        GzFreeDisplay(antiAliasDisplay[displayIndex]);
        antiAliasDisplay[displayIndex] = NULL;
    }

    // Close file
    if( fclose( infile ) )
    {
        AfxMessageBox( "The input file was not closed\n" );
    }

    if( fclose( outfile ) )
    {
        AfxMessageBox( "The output file was not closed\n" );
    }
 
    if (status)
    {
        return(GZ_FAILURE);
    }
    else
    {
        return(GZ_SUCCESS);
    }
}

int Application5::Clean()
{
    // Clean up and exit 
    int status = 0;

    status |= GzFreeRender(m_pRender);
    status |= GzFreeDisplay(m_pDisplay);

    if (status)
    {
        return(GZ_FAILURE);
    }
    else
    {
        return(GZ_SUCCESS);
    }
}
