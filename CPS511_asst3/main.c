/*******************************************************************
           Multi-Part Model Construction and Manipulation
********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GLUT/GLUT.h>
#include "Vector3D.h"
#include "QuadMesh.h"

# define M_PI 3.14159265358979323846
const int meshSize = 100;    // Default Mesh Size
const int vWidth = 1500;     // Viewport width in pixels
const int vHeight = 1000;    // Viewport height in pixels

static int height = 10;
static int width = 1;

static GLfloat xW = 0.0;
static GLfloat zW = 22.0;
static GLfloat yW = 6.0;

Vector3D origin;
Vector3D dir1v;
Vector3D dir2v;

static int currentButton;
static int currentState;


// Lighting/shading and material properties for drone - upcoming lecture - just copy for now

// Light properties
static GLfloat light_position0[] = { -6.0F, 12.0F, 0.0F, 1.0F };
static GLfloat light_position1[] = { 6.0F, 12.0F, 0.0F, 1.0F };
static GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
static GLfloat light_ambient[] = { 0.2F, 0.2F, 0.2F, 1.0F };

// Material properties
static GLfloat drone_mat_ambient[] = { 0.8F, 0.8F, 0.8F, 0.8F };
static GLfloat drone_mat_specular[] = { 0.0F, 0.0F, 0.0F, 1.0F };
static GLfloat drone_mat_diffuse[] = { 0.4F, 0.4F, 0.4F, 0.4F };
static GLfloat drone_mat_shininess[] = { 0.0F };

// A quad mesh representing the ground
static QuadMesh groundMesh;

static GLfloat textureMap1[64][64][3];
static GLfloat textureMap2[64][64][3];
static GLfloat textureMap3[64][64][3];
static GLuint tex[3];

// Prototypes for functions in this module
void initOpenGL(int w, int h);
void display(void);
void reshape(int w, int h);
void mouse(int button, int state, int x, int y);
void mouseMotionHandler(int xMouse, int yMouse);
void keyboard(unsigned char key, int x, int y);
void functionKeys(int key, int x, int y);
void timer(int value);
void instructions(void);
Vector3D ScreenToWorld(int x, int y);

static bool isDead = false;
static bool rotate = false;
static bool enemyAlive = true;
static bool shotTaken = false;
static int toggleView = 0;

static double xLookAt = 0.0;
static double zLookAt = 0.0;

static double xDead;
static double yDead;
static double zDead;

static double xPosition = 10.0;
static double yPosition = 4.0;
static double zPosition = 0.0;

static double torpedoX = 10.0;
static double torpedoZ = 0.0;

static double xEnemy = -10.0;
static double yEnemy = 4.0;
static double zEnemy = 0.0;

static int on = 0;
static int dir = 1;
static int currBut;
static unsigned char currKey;
static double degRotate = 0.0;
static double enemyRotate = 180.0;
static double degUpdate = 0.0;
static double subMotionH = 0.0;

MeshVertex* blobs;

// A cylinder for the top of the sub
static GLUquadricObj *top;

// the wings of the submarine
static GLUquadricObj *wing;

void drawDed(void);
void drawDedPropeller(void);
void createShader(void);
void makeTextureMap(void);
void makeTextures(void);
void assignColor(GLfloat col[3], GLfloat r, GLfloat g, GLfloat b);
void shootTorpedo(void);
void catchUser(void);
void drawSub(void);
void drawBody(void);
void drawPropeller(void);
void drawTop(void);
void drawWings(void);

void drawDed(void) {
    glPushMatrix(); //Copy CTM into matrix stack

    glScalef(1.0, 0.2, 0.2); //Scale the submarine's body, CTM=T*R*S

    //Draw's the body of the submarine as a sphere which is stretched using the CTM from the last line
    glutSolidSphere(1.0, 50, 50);
    glPushMatrix();
    glTranslatef(1.0, 0.0, 1.0);
    drawDedPropeller(); //draws the propeller
    glPushMatrix();
    glTranslatef(0.5, 0.0, -1.0);
    drawTop(); //draws the top of the submarine
    glPushMatrix();
    glTranslatef(1.5, 0.0, 0.5);
    drawWings();

    glPopMatrix(); //Pop back default matrix stack CTM=T*R
    glPopMatrix();
    glPopMatrix();
    glPopMatrix();
}
void drawDedPropeller(void) {
    glPushMatrix(); //Push CTM into matrix stack

    //Translate the propeller to be on the end of the submarine CTM=T*R*T1
    glTranslatef(1.0, 0.0, 0.0);

    //Rotate the propeller so it is alligned with the back of the sub, CTM=T*R*T1*R1
    glRotatef(0, 0.0, 0.0, 0.0);


    //Scale the propeller to be proportionate with the submarine body, CTM=T*R*T1*R1*R2*S
    glScalef(0.1, 1.0, 0.1);
    glutSolidCube(2.0); //Draw the propeller
    glPopMatrix(); //Pop default CTM from matrix stack, CTM=T*R

    glPushMatrix(); //Push CTM into matrix stack

    //Translate the propeller to be on the end of the submarine CTM=T*R*T1
    glTranslatef(0.5, 0.0, 0.0);

    //Rotate the propeller so it is alligned with the back of the sub perpendicular to the first one, CTM=T*R*T1*R1
    glRotatef(0, 0.0, 0.0, 0.0);


    //Scale the propeller to be proportionate with the submarine body, CTM=T*R*T1*R1*R2*S
    glScalef(0.1, 0.1, 1.0);
    glutSolidCube(2.0); //Draw the propeller
    glPopMatrix(); //Pop default CTM from matrix stack, CTM=T*R
}

void createShader(void) {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    char** shader = {"varying vec3 N;",
                    "varying vec3 p;",
                    "void main(void)",
                    "{",
                       "p = vec3(gl_ModelViewMatrix * gl_Vertex);",
                       "N = normalize(gl_NormalMatrix * gl_Normal);",
                       "gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;",
                    "}"};
    
    char** shader1 = {"varying vec3 N;",
                    "varying vec3 p;",
                    "void main (void)",
                    "{",
                       "vec3 L = normalize(gl_LightSource[0].position.xyz - p);",
                       "vec3 V = normalize(-p); // we are in Eye Coordinates, so EyePos is (0,0,0)",
                       "vec3 R = normalize(-reflect(L,N));",
                       "vec4 Iamb = gl_FrontLightProduct[0].ambient;",
                       "vec4 Idiff = gl_FrontLightProduct[0].diffuse * max(dot(N,L), 0.0);",
                       "vec4 Ispec = gl_FrontLightProduct[0].specular * pow(max(dot(R,V),0.0),0.3*gl_FrontMaterial.shininess);",
                       "gl_FragColor = gl_FrontLightModelProduct.sceneColor + Iamb + Idiff + Ispec;",
                        "}"};
    
    // Load Shader Sources
    glShaderSource(vertexShader, 1, &shader, 0);
    glShaderSource(fragmentShader, 1, &shader1, 0);
    
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
    
    int maxLength;
    char *vertexInfoLog;
    char *fragmentInfoLog;
    int IsCompiled_VS, IsCompiled_FS;
    
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &IsCompiled_VS);
    if(IsCompiled_VS == false)
    {
       glGetShaderiv(vertexShader, GL_INFO_LOG_LENGTH, &maxLength);
       vertexInfoLog = (char *)malloc(maxLength);
       glGetShaderInfoLog(vertexShader, maxLength, &maxLength, vertexInfoLog);
       free(vertexInfoLog);
       return;
    }
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &IsCompiled_FS);
    if(IsCompiled_FS == false)
    {
       glGetShaderiv(fragmentShader, GL_INFO_LOG_LENGTH, &maxLength);
       fragmentInfoLog = (char *)malloc(maxLength);
       glGetShaderInfoLog(fragmentShader, maxLength, &maxLength, fragmentInfoLog);
       free(fragmentInfoLog);
       return;
    }
    
    GLuint shaderprogram;
    
    shaderprogram = glCreateProgram();
    glAttachShader(shaderprogram, vertexShader);
    glAttachShader(shaderprogram, fragmentShader);
    glBindAttribLocation(shaderprogram, 0, "in_Position");
    glBindAttribLocation(shaderprogram, 1, "in_Color");
    glLinkProgram(shaderprogram);
    
    glUseProgram(shaderprogram);
}

int main(int argc, char** argv)
{
    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(vWidth, vHeight);
    glutInitWindowPosition(200, 30);
    glutCreateWindow("Assignment 3");

    // Initialize GL
    initOpenGL(vWidth, vHeight);

    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotionHandler);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(functionKeys);
    glutTimerFunc(0, *timer, 0);


    // Start event loop, never returns
    glutMainLoop();

    return 0;
}


// Set up OpenGL. For viewport and projection setup see reshape(). */
void initOpenGL(int w, int h)
{
    // Set up and enable lighting
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light_specular);

    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
    glLightfv(GL_LIGHT1, GL_POSITION, light_position1);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_TEXTURE_2D);
    makeTextureMap();
    makeTextures();

    // Other OpenGL setup
    glEnable(GL_DEPTH_TEST);   // Remove hidded surfaces
    glShadeModel(GL_VERTEX_SHADER);   // Use smooth shading, makes boundaries between polygons harder to see
    glClearColor(0.0F, 0.0F, 0.5F, 0.0F);  // Color and depth for glClear
    glClearDepth(1.0f);
    glEnable(GL_NORMALIZE);    // Renormalize normal vectors
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);   // Nicer perspective

    // Set up ground quad mesh
    origin = NewVector3D(-100.0f, 0.0f, 100.0f);
    dir1v = NewVector3D(1.0f, 0.0f, 0.0f);
    dir2v = NewVector3D(0.0f, 0.0f, -1.0f);
    groundMesh = NewQuadMesh(meshSize);
    blobs = InitMeshQM(&groundMesh, meshSize, origin, 256.0, 256.0, dir1v, dir2v);

    Vector3D ambient = NewVector3D(0.0f, 0.05f, 0.0f);
    Vector3D diffuse = NewVector3D(0.4f, 0.8f, 0.4f);
    Vector3D specular = NewVector3D(0.04f, 0.04f, 0.04f);
    SetMaterialQM(&groundMesh, ambient, diffuse, specular, 0.2);

    // Set up the bounding box of the scene
    // Currently unused. You could set up bounding boxes for your objects eventually.
    //Set(&BBox.min, -8.0f, 0.0, -8.0);
    //Set(&BBox.max, 8.0f, 6.0,  8.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

}

// Callback, called whenever GLUT determines that the window should be redisplayed
// or glutPostRedisplay() has been called.
void display(void)
{

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Set up the camera at position (0, 6, 22) looking at the origin, up along positive y axis
    glLoadIdentity();
    
    if(toggleView == 0) {
        //glRotatef(-degRotate, 0.0, 1.0, 0.0);
        gluLookAt(xPosition, yPosition+0.5, zPosition, xLookAt, yPosition, zLookAt, 0.0, 1.0, 0.0);
    } else {
        gluLookAt(xW, yW, zW, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    }
    
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    createShader();

    // Set drone material properties
    /*glMaterialfv(GL_FRONT, GL_AMBIENT, drone_mat_ambient);
    glMaterialfv(GL_FRONT, GL_SPECULAR, drone_mat_specular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, drone_mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SHININESS, drone_mat_shininess);*/

    glPushMatrix(); //Push current CTM into matrix stack
    /*Translate the submarine's xPosition in the X dir, yPosition + 4.0 in the Y dir, zPosition in the Z dir. xPosition, yPosition and zPosition are global variables that are updated based on key presses*/
    glTranslatef(xPosition, yPosition, zPosition); //CTM=T

    /*Rotate the sub around the Y axis by certain degrees in degRotate variable*/
    glRotatef(degRotate, 0.0, 1.0, 0.0); //CTM=T*R
    glBindTexture(GL_TEXTURE_2D, tex[1]);
    drawSub(); //Draw submarine's individual parts using the default CTM defined above
    glPopMatrix(); //Pop the original CTM back in
    
    glPushMatrix();
    glTranslatef(torpedoX, yPosition, torpedoZ);
    //glRotatef(degRotate, 0.0, 1.0, 0.0);
    shootTorpedo();
    glPopMatrix();
    
    glPushMatrix();
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTranslatef(xEnemy, yEnemy, zEnemy);
    glRotatef(enemyRotate, 0.0, 1.0, 0.0);
    if(enemyAlive) drawSub(); else { xEnemy = -(rand()%10); yEnemy = 4.0; zEnemy = 0.0; enemyRotate = rand(); enemyAlive = true; }
    if((xEnemy > xPosition-2 && xEnemy < xPosition+2 && zEnemy > zPosition-2 && zEnemy < zPosition+2) || (torpedoX > xEnemy-0.5 && torpedoX < xEnemy+0.5 && torpedoZ > zEnemy-0.5 && torpedoZ < zEnemy+0.5)) { enemyAlive = false; isDead = true; xDead = xEnemy; yDead = yEnemy; zDead = zEnemy; shotTaken = false; torpedoX = xPosition; torpedoZ = zPosition; }
    if(torpedoX > 20 || torpedoX < -20 || torpedoZ > 19 || torpedoZ < -19) shotTaken = false;
    for (int i = 0; i < (meshSize+1)*(meshSize+1); i++) {
        if((yPosition < blobs[i].position.y && blobs[i].position.x < xPosition+0.7 && blobs[i].position.x > xPosition-0.7 && blobs[i].position.z < zPosition+0.7 && blobs[i].position.z > zPosition-0.7) || yPosition < 1.0) {
            xPosition = 10.0;
            yPosition = 4.0;
            zPosition = 0.0;
            xLookAt = 0.0;
            zLookAt = 0.0;
        }
        if((yEnemy < blobs[i].position.y && blobs[i].position.x < xEnemy+0.7 && blobs[i].position.x > xEnemy-0.7 && blobs[i].position.z < zEnemy+0.7 && blobs[i].position.z > zEnemy-0.7) || yEnemy < 1.0) {
            isDead = true;
            xDead = xEnemy;
            yDead = yEnemy;
            zDead = zEnemy;
            xEnemy = -(rand()%10);
            yEnemy = 4.0;
            zEnemy = 0.0;
            enemyRotate = rand();
        }
    }
    glPopMatrix();
    if(isDead) {
        glPushMatrix();
        if(yDead > 0) {
            yDead-= 0.2;
        }
        glTranslatef(xDead, yDead, zDead);
        drawDed();
        glPopMatrix();
    }
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    DrawMeshQM(&groundMesh, meshSize); // Draw ground mesh
    
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glutSwapBuffers();   // Double buffering, swap buffers
}

void assignColor(GLfloat col[3], GLfloat r, GLfloat g, GLfloat b) {
    col[0] = r;
    col[1] = g;
    col[2] = b;
}


void makeTextureMap(void)
{
    double range;
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++) {
            range = ((double)(rand() % 200 + -100) / (double)1000);
            assignColor(textureMap1[i][j], 0.500+range, 0.500+range, 0.500+range);
        }
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++) {
            range = ((double)(rand() % 200 + -100) / (double)1000);
            assignColor(textureMap2[i][j], 0.0, 0.500 + range, 0.0);
        }
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++) {
            range = ((double)(rand() % 200 + -100) / (double)1000);
            assignColor(textureMap3[i][j], 0.500 + range, 0.0, 0.0);
        }
}


void makeTextures(void)
{

    glGenTextures(2, tex);
    
    //Texture mapping for the Quadmesh
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_FLOAT, textureMap1);

    //Texture mapping for the Player submarine
    glBindTexture(GL_TEXTURE_2D, tex[1]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_FLOAT, textureMap2);

    //Texture mapping for the Player submarine
    glBindTexture(GL_TEXTURE_2D, tex[2]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_FLOAT, textureMap3);

}

void shootTorpedo(void) {
    if(shotTaken) {
        torpedoX -= cosf((-degRotate*M_PI)/180);
        torpedoZ -= sinf((-degRotate*M_PI)/180);
        glutSolidSphere(0.1, 50, 50);
    }
}

// Callback, called at initialization and whenever user resizes the window.
void reshape(int w, int h)
{
    // Set up viewport, projection, then change to modelview matrix mode -
    // display function will then set up camera and do modeling transforms.
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLdouble)w / h, 0.2, 40.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Set up the camera at position (0, 6, 22) looking at the origin, up along positive y axis
    if(toggleView == 0) {
        glRotatef(-degRotate, 0.0, 1.0, 0.0);
        gluLookAt(xPosition, yPosition+0.5, zPosition, xLookAt, yPosition, zLookAt, 0.0, 1.0, 0.0);
    } else {
        gluLookAt(xW, yW, zW, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    }
}

// Callback, handles input from the keyboard, non-arrow keys
void keyboard(unsigned char key, int x, int y)
{
    if (key == 'p') //if p is pressed, toggle on varible for spinning the propeller
        on = !on;
    else if (key == 's'){ //if b is pressed, update variables for moving the submarine back one step
        on = !on;
        dir = -1; //direction set to reverse
        subMotionH += 0.1;
        xPosition += cosf((-degRotate*M_PI)/180)/5;
        zPosition += sinf((-degRotate*M_PI)/180)/5;
        xLookAt = xPosition + cosf((-degRotate*M_PI)/180)*10;
        zLookAt = zPosition + sinf((-degRotate*M_PI)/180)*10;
    }
    else if (key == 'w') { //if f is pressed, update varibles for moving the submarine forward one step
        on = !on;
        dir = 1; //direction set to forward
        subMotionH -= 0.1;
        xPosition -= cosf((-degRotate*M_PI)/180)/5;
        zPosition -= sinf((-degRotate*M_PI)/180)/5;
        xLookAt = xPosition - cosf((-degRotate*M_PI)/180)*10;
        zLookAt = zPosition - sinf((-degRotate*M_PI)/180)*10;
    } else if(key == 'h') {
        instructions();
    } else if(key == 'a') {
        degRotate += 3;
        enemyRotate -= rand() % 10;
        xLookAt = xPosition - cosf((-degRotate*M_PI)/180)*10;
        zLookAt = zPosition - sinf((-degRotate*M_PI)/180)*10;
    } else if(key == 'd') {
        degRotate -= 3;
        enemyRotate += rand() % 10;
        xLookAt = xPosition - cosf((-degRotate*M_PI)/180)*10;
        zLookAt = zPosition - sinf((-degRotate*M_PI)/180)*10;
    } else if(key == 't') {
        if(toggleView == 1) toggleView = 0;
        else toggleView = 1;
    } else if(key == ' ') {
        torpedoX = xPosition;
        torpedoZ = zPosition;
        shotTaken = true;
    }
    glutPostRedisplay();   // Trigger a window redisplay
}

// Callback, handles input from the keyboard, function and arrow keys
void functionKeys(int key, int x, int y)
{
    if (key == GLUT_KEY_UP) //if up key is pressed, increment submarine up vertically
        yPosition += 0.1;
    else if (key == GLUT_KEY_DOWN) //if down key is pressed, increment submarine down vertically
        yPosition -= 0.1;
    else if (key == GLUT_KEY_LEFT) {//if left key is pressed, rotate submarine counter-clockwise
    } else if (key == GLUT_KEY_RIGHT)  {//if right key is pressed, rotate submarine clockwise 10 degrees
    }
    else if (key == GLUT_KEY_F1) //if F1 key is pressed, call instructions function
        instructions();
    glutPostRedisplay();   // Trigger a window redisplay
}


// Mouse button callback - use only if you want to
void mouse(int button, int state, int x, int y)
{
    currentButton = button;
    currentState = state;

    switch (button)
    {
    case GLUT_LEFT_BUTTON:
        if (state == GLUT_DOWN)
        {
        }
        break;
    case GLUT_RIGHT_BUTTON:
        if (state == GLUT_DOWN)
        {
        }
        break;
    default:
        break;
    }

    glutPostRedisplay();   // Trigger a window redisplay
}


// Mouse motion callback - use only if you want to
void mouseMotionHandler(int xMouse, int yMouse)
{
    if (currentButton == GLUT_LEFT_BUTTON)
    {
    }

    glutPostRedisplay();   // Trigger a window redisplay
}


Vector3D ScreenToWorld(int x, int y)
{
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY, winZ;
    GLdouble posX, posY, posZ;
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
    winX = (float)x;
    winY = (float)viewport[3] - (float)y;
    glReadPixels( x, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
    Vector3D result;
    result.x = posX;
    result.y = posY;
    result.z = posZ;
    return result;
}

void timer(int value) {
    if (on)
        if(dir == 1) degUpdate += 5;
        else degUpdate -= 5;
    catchUser();
    glutTimerFunc(16, *timer, 0);
    glutPostRedisplay();
}

void catchUser(void) {
    if(xEnemy > 20) xEnemy = -20;
    else if (xEnemy < -20) xEnemy = 20;
    if(zEnemy > 16) zEnemy = -19;
    else if (zEnemy < -19) zEnemy = 16;
    if(xEnemy < xPosition) { xEnemy-= cosf((-enemyRotate*M_PI)/180)/10; zEnemy-= sinf((-enemyRotate*M_PI)/180)/10; }
    else if(xEnemy > xPosition)  { xEnemy-= cosf((-enemyRotate*M_PI)/180)/10; zEnemy-= sinf((-enemyRotate*M_PI)/180)/10; }
    if(yEnemy > yPosition) { yEnemy-= 0.1; }
    else if(yEnemy < yPosition) { yEnemy+= 0.1; }
    if(zEnemy < zPosition) { xEnemy-= cosf((-enemyRotate*M_PI)/180)/10; zEnemy-= sinf((-enemyRotate*M_PI)/180)/10; }
    else if(zEnemy > zPosition) { xEnemy-= cosf((-enemyRotate*M_PI)/180)/10; zEnemy-= sinf((-enemyRotate*M_PI)/180)/10; }
}

// Calls func to draw the submarine
void drawSub(void) { drawBody(); }

// Creates submarine body and calls func to draw rest of the submarine
void drawBody(void) {
    glPushMatrix(); //Copy CTM into matrix stack

    glScalef(1.0, 0.2, 0.2); //Scale the submarine's body, CTM=T*R*S
    
    //Draw's the body of the submarine as a sphere which is stretched using the CTM from the last line
    glutSolidSphere(1.0, 50, 50);

    drawPropeller(); //draws the propeller
    
    drawTop(); //draws the top of the submarine
    
    drawWings();

    glPopMatrix(); //Pop back default matrix stack CTM=T*R
}

//Draws propeller on the submarine
void drawPropeller(void) {
    glPushMatrix(); //Push CTM into matrix stack

    //Translate the propeller to be on the end of the submarine CTM=T*R*T1
    glTranslatef(1.0, 0.0, 0.0);
    
    //Rotate the propeller so it is alligned with the back of the sub, CTM=T*R*T1*R1
    glRotatef(90, 0.0, 1.0, 0.0);

    //Animates the propeller rotating by the degrees specified in degUpdate variable, CTM=T*R*T1*R1*R2
    glRotatef(degUpdate, 0.0, 0.0, 1.0);
    
    //Scale the propeller to be proportionate with the submarine body, CTM=T*R*T1*R1*R2*S
    glScalef(0.1, 1.0, 0.1);
    glutSolidCube(2.0); //Draw the propeller
    glPopMatrix(); //Pop default CTM from matrix stack, CTM=T*R
   
    glPushMatrix(); //Push CTM into matrix stack

    //Translate the propeller to be on the end of the submarine CTM=T*R*T1
    glTranslatef(1.0, 0.0, 0.0);
    
    //Rotate the propeller so it is alligned with the back of the sub perpendicular to the first one, CTM=T*R*T1*R1
    glRotatef(0, 0.0, 1.0, 0.0);

    //Animates the propeller rotating by the degrees specified in degUpdate variable, CTM=T*R*T1*R1*R2
    glRotatef(degUpdate, 1.0, 0.0, 0.0);
    
    //Scale the propeller to be proportionate with the submarine body, CTM=T*R*T1*R1*R2*S
    glScalef(0.1, 0.1, 1.0);
    glutSolidCube(2.0); //Draw the propeller
    glPopMatrix(); //Pop default CTM from matrix stack, CTM=T*R
}

//Draws the top of the submarine
void drawTop(void){
    glPushMatrix(); //Push CTM into matrix stack

    glTranslatef(-0.4, 0.2, 0.0); //Translate the top of the submarine to the top, CTM=T*R*T3

    glRotatef(-90, 1.0, 0.0, 0.0); //Rotate the top of the submarine to allign with the body, CTM=T*R*T3*R3

    glScalef(0.1, 0.2, 0.8); //Scale the top of the submarine to fit on the body, CTM=T*R*T3*R3*S1
    
    top = gluNewQuadric(); //Create the top part of the submarine
    gluQuadricDrawStyle(top, GLU_LINE);
    gluCylinder(top, 2.0, 2.0, 1.25, 1000, 1000);
    glPopMatrix(); //Pop default CTM from matrix stack, CTM=T*R
}

//Draws the wings of the submarine
void drawWings(void){
    glPushMatrix(); //Push CTM into matrix stack
    glTranslatef(0.0, 0.0, 1.0); //Translate the top of the submarine to the top, CTM=T*R*T3
    glRotatef(-90, 1.0, 0.0, 0.0); //Rotate the top of the submarine to allign with the body, CTM=T*R*T3*R3
    glScalef(0.1, 0.1, 1.0); //Scale the top of the submarine to fit on the body, CTM=T*R*T3*R3*S1
    wing = gluNewQuadric(); //Create the top part of the submarine
    gluQuadricDrawStyle(wing, GLU_LINE);
    gluCylinder(wing, 1.5, 1.5, 0.2, 1000, 1000);
    glPopMatrix(); //Pop default CTM from matrix stack, CTM=T*R
    
    glPushMatrix(); //Push CTM into matrix stack
    glTranslatef(0.0, 0.0, -1.0); //Translate the top of the submarine to the top, CTM=T*R*T3
    glRotatef(-90, 1.0, 0.0, 0.0); //Rotate the top of the submarine to allign with the body, CTM=T*R*T3*R3
    glScalef(0.1, 0.1, 1.0); //Scale the top of the submarine to fit on the body, CTM=T*R*T3*R3*S1
    wing = gluNewQuadric(); //Create the top part of the submarine
    gluQuadricDrawStyle(wing, GLU_LINE);
    gluCylinder(wing, 1.5, 1.5, 0.2, 1000, 1000);
    glPopMatrix();
}

//Prints instructions
void instructions(void) {
    printf("WELCOME\n\n");
    printf("TO TOGGLE VIEW USE 't' KEY\n");
    printf("TO ROTATE SUBMARINE USE 'a' OR 'd' KEYS\n");
    printf("TO MOVE FORWARD AND BACK USE 'w' OR 's' KEYS\n");
    printf("TO MOVE SUBMARINE VERTICALLY USE UP AND DOWN ARROW KEYS\n");
    printf("TO SHOOT TORPEDO USE SPACE BAR\n");
    printf("\n\nTHERE IS AN ENEMY SUBMARINE TRYING TO ATTACK YOU!\n");
    printf("DEFEND YOURSELF AND ELEMINATE THE ENEMY\n");
}
