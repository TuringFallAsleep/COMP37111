#ifdef MACOSX
#include <GLUT/glut.h>
#include <math.h>
#include <stdlib.h>
#else
#include <GL/glut.h>
#include <math.h>
#include <stdlib.h>
#endif


/* Set to 0 or 1 for normal or reversed mouse Y direction */
#define INVERT_MOUSE 0

#define RUN_SPEED  0.15
#define TURN_ANGLE 4.0
#define DEG_TO_RAD 0.017453292 // 3.1415926/180

typedef enum { false, true } Boolean;


GLdouble lat,     lon;              /* View angles (degrees)    */
GLdouble mlat,    mlon;             /* Mouse look offset angles */   
GLfloat  eyex,    eyey,    eyez;    /* Eye point                */
GLfloat  centerx, centery, centerz; /* Look point               */
GLfloat  upx,     upy,     upz;     /* View up vector           */

GLfloat  ang= 0.0;                  /* Mystical object angle    */
GLint width= 640, height= 480;      /* size of window           */

GLfloat white_light[] =     { 1.0, 1.0, 1.0, 1.0 };
GLfloat light_position0[] = { -5.0, 7.0, -5.0, 0.0 };
GLfloat matSpecular[] =     { 1.0, 1.0, 1.0, 1.0 };
GLfloat matShininess[] =    { 50.0 };
GLfloat matSurface[] =      { 0.8, 0.5, 0.2, 0.1 };
GLfloat matEmissive[] =     { 0.0, 1.0, 0.0, 0.1 };

//////////////////////////////////////////////

static int hit_c = 0;
int is_jumping = 0,    jump_up = 1,   drop_down = 0;
GLfloat jump_hight = 6.0;
int num = 2000000;
int count = 0;
int goDie = 3;
int qualityOfWater = 10; // every water drop has 10*10 particles
Boolean stain = false;

////////////////////////////////////////////////

float teapotPositionX = 0.0;
float teapotPositionY = 2.0;
float teapotPositionZ = 0.0;

float edge = 1.55242; // the distance from rotate center to teapot mouth, sqre(1.5^2 + 0.4^2)
float alpha = 75.06858; // the angle from teapot mouth to y axis, atan(1.5/0.4)/DEG_TO_RAD


////////////////////////////////////////////////

static GLfloat vmax = 1.0; // when theta is 90.0
GLfloat gravity = 0.005;
float teapotTheta = 30.0;
float groundYLocation = -10;
float energyLoss = 0.5;

////////////////////////////////////////////////

float dist (float x, float y, float z) {
// returns distance to origin from point (x,y,z)
return(fabs(sqrt(x*x + y*y + z*z)));
} // dist()

//////////////////////////////////////////////

int within(float x, float y, float z, float inner, float outer) {
// check if (x,y,z) lies between "inner" and "outer"
float d;
d= dist(x,y,z);
if ((d > inner) && (d < outer)) {return(1);}
else {return(0);}
} // within()

//////////////////////////////////////////////

void energy(float ringMin, float ringMax, float r, float g, float b) {
// Draw a ring of energy, comprising a swarm of particles in random positions, but 
// bounded to lie withing ringMin-ringMax, the space between two concentric spheres
int i;
float R= 10.0; // to get rands in range 0-R
float x,y,z;

glDisable(GL_LIGHTING); // don't want lighting on
glEnable(GL_POINT_SMOOTH);
glPointSize(2.0);
glColor3f(r,g,b);
glBegin(GL_POINTS);
for (i= 0; i<10000; i++) {
   // get rand p in range -R/2 < p < +R/2
   x= R* rand() / (((double)RAND_MAX+1)) -R/2.0;
   y= R* rand() / (((double)RAND_MAX+1)) -R/2.0;
   z= 0.0; // to make it a flat ring
   if (within(x,y,z, ringMin, ringMax)) glVertex3f(x,y,z);
}
glEnd();
glEnable(GL_LIGHTING);
} // energy()

////////////////////////////////////////////////

typedef struct {
  float size; 
  float lifetime;

  float x;
  float y;
  float z;

  float velocity;

  float vy;
  float vz;

  float theta;

  float r;
  float g;
  float b;

  float waterDropStartX;
  float waterDropStartY;
  float waterDropStartZ;

  Boolean hitGround;
  int hitTime;

  Boolean startPoint;

  GLfloat matParticle[3];
} WaterDrop;

WaterDrop **waterDrop;

void deadParticle(int i){
  // printf("deadParticle\n");
  // printf("waterDrop[%d]->hitTime = %d \n",i,waterDrop[i]->hitTime );
  if (waterDrop[i]!=NULL){
    if (waterDrop[i]->hitTime >= goDie){
      free(waterDrop[i]);
      waterDrop[i] = NULL;
      printf("waterDrop[%d] freed \n", i);
    }
  }
}

void particleMove(int index){
  // printf("particleMove\n");
  // if (index == 10)
  // {
  //   waterDrop[index]->size = 5.0;
  // }
  if (waterDrop[index]->hitTime < 4){
    if (waterDrop[index]->hitGround == false){
      waterDrop[index]->vy += gravity; 

      waterDrop[index]->y += waterDrop[index]->velocity * cos(waterDrop[index]->theta * DEG_TO_RAD) - waterDrop[index]->vy;
      waterDrop[index]->x += waterDrop[index]->velocity * sin(waterDrop[index]->theta * DEG_TO_RAD);
      // printf("hit_ground: %d \n", hit_ground);
      // printf("particleMove: %i, particleMove: (x,y,z) = (%f, %f, %f) \n", index, waterDrop[index]->x, waterDrop[index]->y, waterDrop[index]->z);
    }else if(waterDrop[index]->hitGround == true){
      waterDrop[index]->vy = -energyLoss * waterDrop[index]->vy;
      waterDrop[index]->velocity = energyLoss * waterDrop[index]->velocity;
      waterDrop[index]->hitGround = false;
      waterDrop[index]->hitTime++;
      // printf("waterDrop[%d]->vy = %f \n", index, waterDrop[index]->vy);
      // printf("hitGround, particleMove: %i, particleMove: (x,y,z) = (%f, %f, %f) \n", index, waterDrop[index]->x, waterDrop[index]->y, waterDrop[index]->z);
    }

    // printf("waterDrop[0]->x,y,z = (%f, %f, %f)\n", waterDrop[0]->x, waterDrop[0]->y, waterDrop[0]->z);
  }

  }


void initialiseParticle(int i){
  waterDrop[i]->size = 1.0/16.0;
  waterDrop[i]->lifetime = 100000;

  waterDrop[i]->x = 0.1;
  waterDrop[i]->y = 0.0;
  waterDrop[i]->z = 0.0;

  waterDrop[i]->velocity = 0.08;
  waterDrop[i]->vy = 0.0;
  waterDrop[i]->vz = 0.0;

  waterDrop[i]->theta = teapotTheta;

  waterDrop[i]->r = 0.6;
  waterDrop[i]->g = 0.8;
  waterDrop[i]->b = 1.0;

  waterDrop[i]->matParticle[0] = waterDrop[i]->r;
  waterDrop[i]->matParticle[1] = waterDrop[i]->g;
  waterDrop[i]->matParticle[2] = waterDrop[i]->b;

  waterDrop[i]->waterDropStartX = 0.0;
  waterDrop[i]->waterDropStartY = 0.0;
  waterDrop[i]->waterDropStartZ = 0.0;

  waterDrop[i]->hitGround = false;
  waterDrop[i]->hitTime = 0;

  waterDrop[i]->startPoint = true;
}

//////////////////////////////////////////////

void calculate_start_position(int i){
  if (waterDrop[i]->startPoint == true){

   float beta = (180 - waterDrop[i]->theta - alpha) * DEG_TO_RAD; 
   
   waterDrop[i]->waterDropStartX = teapotPositionX + edge * sin(beta);
   waterDrop[i]->waterDropStartY = teapotPositionY - edge * cos(beta);
   waterDrop[i]->waterDropStartZ = teapotPositionZ;

   waterDrop[i]->x = waterDrop[i]->waterDropStartX;
   waterDrop[i]->y = waterDrop[i]->waterDropStartY;
   waterDrop[i]->z = waterDrop[i]->waterDropStartZ;

   waterDrop[i]->startPoint = false;
  }
  
}

//////////////////////////////////////////////


void draw_scene(void) {
  // printf("draw_scene\n");
  // Draws all the elements in the scene
  int x, z;
  int L= 40;
  int j;
  float R = 1.0;
  
  GLfloat matTeapot[] = { 0.8, 0.6, 0.6, 0.0 };
  GLfloat matWater[] = { 0.6, 0.8, 1.0, 0.5};


  // newParticle(waterDrop->size, waterDrop->velocity, waterDrop->lifetime, waterDrop->x, waterDrop->y, waterDrop->z, waterDrop->vy, waterDrop->theta, waterDrop->r, waterDrop->g, waterDrop->b);
  
  /* Draw ground */
  glDisable(GL_LIGHTING);
  // the ground quad and the grid lines are co-planar, which would lead to horrible Z-fighting, 
  // so we resort to 2 hacks. First, fiddle with the Z-buffer depth range, using glDepthRange(), 
  // and second, draw the lines 0.01 higher in Y than the ground plane
  glDepthRange (0.1, 1.0);
  glColor3f(0.2, 0.2, 0.4);
  glBegin(GL_QUADS);
  glVertex3f(-L, -10, -L);
  glVertex3f(L, -10, -L);
  glVertex3f(L, -10, L);
  glVertex3f(-L, -10, L);
  glEnd();

  glEnable(GL_LIGHTING);
  
  /* Draw the teapot */
  glPushMatrix();
    // glScalef(0.5, 0.5, 0.5); 
    glTranslatef(teapotPositionX, teapotPositionY, teapotPositionZ); // (0.0, 2.0, 0.0)
    // glRotatef(ang, 0.0, 1.0, 0.0);  // rotate the teapot
    
    
    glRotatef(teapotTheta, 0.0, 0.0, -1.0); // teapot 30 deg left
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matTeapot);

    glutSolidTeapot(1.0); // the fairy

      glPushMatrix();
      energy(1.0, 1.1,  1.0,1.0,0.0);
      glPopMatrix();

  glPopMatrix();

//////////// draw particle starts
    if (count < num){
      initialiseParticle(count);
      for (int i = 0; i < count; i++){
        if (waterDrop[i]!=NULL){
          // glRotatef(teapotTheta, 0.0, 0.0, 1.0);
          if (teapotTheta > 0.0){
            glPushMatrix();
            calculate_start_position(i); // has new water drop comes out
            glTranslatef(waterDrop[i]->x, waterDrop[i]->y, waterDrop[i]->z);
            glEnable(GL_POINT_SMOOTH);  
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, waterDrop[i]->matParticle);
            glutSolidSphere(waterDrop[i]->size,qualityOfWater,qualityOfWater); 
            // glutSolidTeapot(waterDrop[count]->size);  
            glEnd(); 
            glPopMatrix();
          }else{
            glPushMatrix();
            glTranslatef(waterDrop[i]->x, waterDrop[i]->y, waterDrop[i]->z);
            glEnable(GL_POINT_SMOOTH);  
            glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, waterDrop[i]->matParticle);
            glutSolidSphere(waterDrop[i]->size,qualityOfWater,qualityOfWater); 
            // glutSolidTeapot(waterDrop[count]->size);  
            glEnd();
            glPopMatrix();
          }
          // glRotatef(teapotTheta, 0.0, 0.0, 1.0); // water drop to vertical direction 
        }
      }
    }
    
      
    for (int i = 0; i < count; ++i){
      if (waterDrop[i]!=NULL){
        if (waterDrop[i]->hitTime<=goDie){
        glPushMatrix();
        glTranslatef(waterDrop[i]->x, waterDrop[i]->y, waterDrop[i]->z);
        glEnable(GL_POINT_SMOOTH);  
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, waterDrop[i]->matParticle);
        glutSolidSphere(waterDrop[i]->size,qualityOfWater,qualityOfWater); 
        // glutSolidTeapot(waterDrop[count]->size);  
        glEnd();
        glPopMatrix();
      }
      // printf("particle %d hitTime: %d \n",i, waterDrop[i]->hitTime );
      if (waterDrop[i]->hitTime == goDie+1){ // life time
        if (stain==true)
        {
          deadParticle(i);
        }
        
        // printf("waterDrop[%d] is freed\n", i);
        // glClear(GL_ACCUM_BUFFER_BIT);
      }
      }
    }
////////////////////// draw particle ends



} // draw_scene()

//////////////////////////////////////////////

void calculate_lookpoint(void) { /* Given an eyepoint and latitude and longitude angles, will
     compute a look point one unit away */

  centerx = eyex + cos(lat*DEG_TO_RAD)*sin(lon*DEG_TO_RAD);
  centery = eyey + sin(lat*DEG_TO_RAD);
  centerz = eyez + cos(lat*DEG_TO_RAD)*cos(lon*DEG_TO_RAD);
  // gluLookAt(eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);


} // calculate_lookpoint()

//////////////////////////////////////////////

void display(void) {	
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   // glMaterialfv(GL_FRONT, GL_SPECULAR,  matSpecular);
   // glMaterialfv(GL_FRONT, GL_SHININESS, matShininess);
   // glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matSurface);
   glLoadIdentity();
   calculate_lookpoint(); /* Compute the centre of interest   */
   gluLookAt(eyex, eyey, eyez, centerx, centery, centerz, upx, upy, upz);
   glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
   draw_scene();
   glutSwapBuffers();
} // display()

//////////////////////////////////////////////

void animation(void) {
  // printf("animation\n");
  // Spin the mystical object
  ang= ang + 1.0; if (ang > 360.0) ang= ang - 360.0;
  // the track of water drop
  if (count < num-1){
    for (int i = 0; i < count; i++){
      if (waterDrop[i]!=NULL){
      particleMove(i); // use count as index
      if (waterDrop[i]->y < groundYLocation && waterDrop[i]->vy > 0.0){
        waterDrop[i]->hitGround = true;
        // printf("waterDrop[%d]->hitGround: true \n", i);
      }
      // printf("waterDrop[%d]->hitTime = %d \n", i, waterDrop[i]->hitTime);
    }
    }
    if (teapotTheta > 0.0){
      count++;  
      // printf("number of particles: %d\n", count);
    }   
  }

  for (int i = 0; i < count; i++){
    if (waterDrop[i]!=NULL){
      if (waterDrop[i]->hitTime<=goDie){ // 4
      particleMove(i); 
      if (waterDrop[i]->y < groundYLocation && waterDrop[i]->vy > 0.0){
        waterDrop[i]->hitGround = true;
        // printf("waterDrop[%d]->hitGround: true \n", i);
      }
    }
    }
  }
  
  // printf("count = %d \n", count);

  if (is_jumping){
        if (jump_up){
          eyey = eyey + 0.1;
          centery = centery + 0.1;
          if (eyey >= jump_hight){
            jump_up = 0;
            drop_down = 1;
          }
        }
        if (drop_down){
          eyey = eyey - 0.1;
          centery = centery - 0.1;
          if (eyey <= 1.7){
            drop_down = 0;
            jump_up = 1;
            is_jumping = 0;
          }
        }
  }


  glutPostRedisplay();
} //animation()

//////////////////////////////////////////////

void allocateParticle(){
  for (int i = 0; i < num; ++i){
    waterDrop[i] = malloc(sizeof(WaterDrop));
  }
}

//////////////////////////////////////////////

void reshape(int w, int h) {
  glViewport (0, 0, (GLsizei) w, (GLsizei) h); 
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  gluPerspective (50, (GLfloat) w / (GLfloat) h, 0.1, 80.0);
  glMatrixMode (GL_MODELVIEW);
  width= w;   /* Record the new width and height */
  height= h;
} //reshape()
  
//////////////////////////////////////////////

void mouse_motion(int x, int y) {
  /* To be completed */
  mlon = 0.01*(x-width/2)*(-100)/width;
  mlat = 0.01*(y-height/2)*(-100)/height;
  if (lon + mlon <= 90 && lon + mlon >= -90)
    lon += mlon;
  if (lat + mlat <= 90 && lat + mlat>= -90)
    lat += mlat;
} // mouse_motion()

//////////////////////////////////////////////
  
void keyboard(unsigned char key, int x, int y) {
  switch (key) {
    case 27:  /* Escape key */
      printf("exit.\n");
      // printf("%d", GLUT_KEY_PAGE_UP);
      // printf("%d", GLUT_KEY_PAGE_DOWN);
      exit(0);
      break;

    case 44:
      eyex += sin(DEG_TO_RAD*(lon+90))*RUN_SPEED;
      eyez += cos(DEG_TO_RAD*(lon+90))*RUN_SPEED;
      break;
    case 46:
      eyex += sin(DEG_TO_RAD*(lon-90))*RUN_SPEED;
      eyez += cos(DEG_TO_RAD*(lon-90))*RUN_SPEED;
      break;
    case 99:
      if (!hit_c){
        hit_c = 1;
        eyey -= 1;
      }else{
        hit_c = 0;
        eyey += 1;
      }
      break;
    case 32:     
      is_jumping = 1;
      break;
    case 'e':
      if (teapotTheta >= -89)
        teapotTheta -= 5;
      break;
    case 'r':
      if (teapotTheta <= 89)
        teapotTheta += 5;
      break;
    case 'q':
      teapotTheta = 0.0;
      break;
    case 'n':
      if (num>50)
      {
        num -= 100;
      }
    case 'N':
      if (num<10000)
      {
        num += 100;
      }
    case 'w': // w - up
      teapotPositionY += 0.1;
      break;
    case 's': // s - down
      teapotPositionY -= 0.1;
      break;
    case 'a': // a - left
      teapotPositionX += 0.1;
      break;
    case 'd': // d - right
      teapotPositionX -= 0.1;
      break;
    case 'k':
      teapotPositionZ += 0.1;
      break;
    case 'l':
      teapotPositionZ -= 0.1;
      break;
    case 'p':
      stain = !stain;
      break;

    default: break;
      /* To be completed */
   }

   
} // keyboard()

//////////////////////////////////////////////

void cursor_keys(unsigned char key, int x, int y) {
  switch (key) {
    case GLUT_KEY_LEFT:
      lon += TURN_ANGLE;
      if (lon == 360)
        lon = 0;
      break;
    case GLUT_KEY_RIGHT:
      lon -= TURN_ANGLE;
      if (lon == -360)
        lon = 0;
      break;
    case GLUT_KEY_PAGE_UP: //GLUT_KEY_PAGE_UP  DEG_TO_RAD
      if (lat + TURN_ANGLE <= 90){
        lat += TURN_ANGLE;
      }
      break;
    case GLUT_KEY_PAGE_DOWN: //GLUT_KEY_PAGE_DOWN
      if (lat - TURN_ANGLE >= -90){
        lat -= TURN_ANGLE;
      }
      break;
    case GLUT_KEY_UP:
      eyex += sin(DEG_TO_RAD*lon)*RUN_SPEED;
      eyez += cos(DEG_TO_RAD*lon)*RUN_SPEED;
      break;
    case GLUT_KEY_DOWN:
      eyex -= sin(DEG_TO_RAD*lon)*RUN_SPEED;
      eyez -= cos(DEG_TO_RAD*lon)*RUN_SPEED;
      break;

    case GLUT_KEY_HOME:
      eyex =  0.0; /* Set eyepoint at eye height within the scene */
      eyey =  1.7;
      eyez = -8.0;

      upx = 0.0;   /* Set up direction to the +Y axis  */
      upy = 1.0;
      upz = 0.0;

      lon = 0.0;
      lat = 0.0;

      mlat= 0.0;  /* Zero mouse look angles */
      mlon= 0.0;
      break;
    default: break;
      /* To be completed */
  }
} // cursor_keys()
  
//////////////////////////////////////////////

void init(void) {
  glClearColor(0.0, 0.0, 0.0, 0.0);   /* Define background colour */
   
  /* Set initial view parameters */
  eyex=  0.0; /* Set eyepoint at eye height within the scene */
  eyey=  3.0;
  eyez= -20.0;

  upx= 0.0;   /* Set up direction to the +Y axis  */
  upy= 1.0;
  upz= 0.0;

  lat= 0.0;   /* Look horizontally ...  */
  lon= 0.0;   /* ... along the +Z axis  */

  mlat= 0.0;  /* Zero mouse look angles */
  mlon= 0.0;

  /* set up lighting */
  glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
  glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glEnable(GL_NORMALIZE);
} // init()

int main(int argc, char** argv) {

  waterDrop = malloc(num * sizeof(WaterDrop*));

  printf("Object:\nw - up; s - down; a - left; d - right;\nl - front; k - back; \nr: rotate down; e: rotate up;\nViewer: \nupkey,downkey,leftkey,rightkey\nEye: mouse;\nStain: p\n");
  // printf("Finish, allow users to play as long as possible. If they feel the particle system is not fluent, they can use 'p' to free the particles\n");
  // printf("%f\n", cos(135*DEG_TO_RAD));

  allocateParticle(); 

  glutInit (&argc, argv);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
  glutInitWindowSize (width, height); 
  glutCreateWindow ("COMP37111 Coursework");
  init();
  glutDisplayFunc (display); 
  glutIdleFunc (animation); 
  glutReshapeFunc (reshape);
  glutKeyboardFunc (keyboard);
  glutSpecialFunc (cursor_keys);
  glutPassiveMotionFunc (mouse_motion);
  glutMainLoop ();
  return 0;
}

/* end of fairy.c */

//gcc -DMACOSX -framework OpenGL -framework GLUT -o fairy fairy.c
