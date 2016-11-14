#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctime>
#include <string.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#  include <GLUT/glut.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/freeglut.h>
#endif


float **points;		//The grid for the program, x and z act as indexs to the y values. 
float ***normals;	//The normals for all vetices in the grid. 
int **faces; 		//Stores the faces for the grid. 
int dim, faceNum, minRad, maxRad, gIter;	// dim is dimension of the grid +1 (for drawing purposes), gIter manages iterations. Min and Max rad are used for TerrainCircles
char drawType, faceShape; 			//Stores relavent drawing information
float highest, lowest, fHeight; 	//Stores the highest and lowest y values of the grid. fHeight is used for generation
float camPos[3];					//stores the Camera position in the program. 
float origPos[3]; 					//Stores the origin position in the program. 
bool isLight;						//Indicates whether or not the program is in Lighting Mode 
bool isFlat, isL1 = false, isL2 = false, isMap; //Various booleans used to manage lighting and map drawing. 
GLint WindowID1, WindowID2;			//Helps with Glut's multiple windows
float *lightPos;
float *lightPos2;					//Stores the positions of the lights for lighting mode. 

//Prints the help interface 
void printHelp(){
	printf("-------------------------------------------------------------------\n");
	printf("Welcome to the Terrain Generator!\n");
	printf("-------------------------------------------------------------------\n");
	printf("Here are the Relavent controls:\n");
	printf("	-Use the arrow Keys Navigate the camera in the x and z planes\n");
	printf("	-Increase/decrease the camera with with the 'home' and 'end' keys \n");
	printf("	-Randomize the terrain with the Terrain Circles algorithm by pressing 'r'\n");
	printf("	-Randomize the terrain with the Terrain Fault algorithm by pressing 'f'\n");
	printf("	-Toggle wireframe view only, full coloring view only, and both with 'w'\n");
	printf("	-Display the terrain height-map with 'm' (the view only updates when you click on it)\n");
	printf("	-Toggle between square and triangle grids with 'y' and 't' respectively\n");
	printf("	-Alter the iterations the algorithm performs with the 1 and 2 keys\n");
	printf("	-Alter the grid size with the 3 and 4 keys\n");
	printf("	-Display the Camera location with 'c'\n");
	printf("	-Redisplay this help menu with 'h'\n");
	printf("Pressing 'l' toggles Lighting mode. In Lighting mode you can: \n");
	printf("	-Alter shading with 's'\n");
	printf("	-Toggle the two lights with 'k' and 'p'\n");
	printf("	-Move light 1 with 'g','v','b', and 'n' respectively\n");
	printf("	-Move light 2 with the same controls as light 1, but holding shift while doing so\n");
	printf("Press 'q' to quit\n");
	printf("Enjoy!\n");
	printf("--------------------------------------------------------------------\n");
}

//resets the locations of the Lights used in Lighting Mode. 
 void resetLight(){
	lightPos = (float*) calloc(4, sizeof(float));
	lightPos2 = (float*) calloc(4, sizeof(float));
	lightPos[0] = 0;
	lightPos[2] = 0;
	lightPos[1] = 0;
	lightPos[3] = 1;
	lightPos2[0] = dim+1;
	lightPos2[2] = dim+1;
	lightPos2[0] = dim+1;
	lightPos2[3] = 1;
}

//Activates GLUT's lighting mode. It sets the lights, materials, and their respective colors as well as shading
lightMeUp(bool light){
	if(light){
		//enable lighting
		glEnable(GL_LIGHTING); 
		//turn on light bulb 0
		glEnable(GL_LIGHT0);
		isL1 = true;
		glEnable(GL_LIGHT1);
		isL2 = true;
		//define light properties
		float amb[4] = {1, 0, 0, 1};  
		float dif[4] = {1, 0, 1, 1};
		float spc[4] = {1, 1, 1, 1};
		//upload light data to gpu
		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		glLightfv(GL_LIGHT0, GL_SPECULAR, spc);
		
		float amb2[4] = {0, 0, 1, 1};  
		float dif2[4] = {0, 1, 1, 1};
		float spc2[4] = {1, 0, 1, 1};
		//upload light data to gpu
		glLightfv(GL_LIGHT1, GL_POSITION, lightPos2);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, dif2);
		glLightfv(GL_LIGHT1, GL_AMBIENT, amb2);
		glLightfv(GL_LIGHT1, GL_SPECULAR, spc2);
		
		//define and use a material
		float m_amb[] = {0.33, 0.22, 0.03, 1.0};
		float m_dif[] = {0.78, 0.57, 0.11, 1.0};
		float m_spec[] = {0.99, 0.91, 0.81, 1.0};
		float shiny = 50; //10, 100
		//upload material data to gpu
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, m_amb);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, m_dif);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, m_spec);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shiny);
		isFlat = !isFlat; 
		if (isFlat && isLight)
			glShadeModel(GL_FLAT);
		else if (!isFlat &&  isLight)
			glShadeModel(GL_SMOOTH);
		else
			printf("Error: Cannot alter shading when lighting is off (press L)");
	}
	else{
		glDisable(GL_LIGHTING);
	}
}

//Draws the wire frame for the grid. 
void drawWire(int a){
	int l;
	if (faceShape == 't')
		l = 3;
	else if (faceShape == 'q')
		l = 4;
	else{
		printf("drawWire Error");
		return; 
	}
	glBegin(GL_LINES);
		for (int i = 3; i < l*2; i= i+2){
			glVertex3f((float)faces[a][i-3], points[faces[a][i-3]][faces[a][i-2]],(float)faces[a][i-2]);
			glVertex3f((float)faces[a][i-1], points[faces[a][i-1]][faces[a][i]],(float)faces[a][i]);
		}
		glVertex3f((float)faces[a][0], points[faces[a][0]][faces[a][1]],(float)faces[a][1]);
		glVertex3f((float)faces[a][l*2-2], points[faces[a][l*2-2]][faces[a][l*2-1]], (float)faces[a][l*2-1]);
	glEnd();
}

//Draws the faces for the grid. 
void drawFace(int a){
	int l;
	float height;
	if (faceShape == 't'){
		l = 3;
		glBegin(GL_TRIANGLES);
			for (int i = 1; i < l*2; i= i+2){
				height = points[faces[a][i-1]][faces[a][i]];
				glColor3f(fabs(height/(highest-lowest)), (highest - height)/(highest - lowest),0);
				glNormal3f(normals[faces[a][i-1]][faces[a][i]][0], normals[faces[a][i-1]][faces[a][i]][1],normals[faces[a][i-1]][faces[a][i]][2]);
				glVertex3f((float)faces[a][i-1], height, (float)faces[a][i]);
			}
		glEnd();
	}
	else if (faceShape == 'q'){
		l = 4;
		glBegin(GL_QUADS);
			for (int i = 1; i < l*2; i= i+2){
				height = points[faces[a][i-1]][faces[a][i]];
				glNormal3f(normals[faces[a][i-1]][faces[a][i]][0], normals[faces[a][i-1]][faces[a][i]][1],normals[faces[a][i-1]][faces[a][i]][2]);
				glColor3f(fabs(height/(highest-lowest)), (highest - height)/(highest - lowest),0);
				glVertex3f((float)faces[a][i-1], height, (float)faces[a][i]);
			}
		glEnd();
	}
	else{
		printf("drawShape Error");
		return; 
	}
}

//Helper function for set Normals
void addToNormals(int x, int z, float *norm){
	float n = sqrt(norm[0]*norm[0] + norm[1]*norm[1] + norm[2]*norm[2]);
	normals[x][z][0] += norm[0]/n;
	normals[x][z][1] += norm[1]/n;
	normals[x][z][2] += norm[2]/n;
}

//Sets the normal values for each vertex in the grid. This is done by taking the cross product of relavent vectors around each vertex 
void setNormals(){
	int avgNum;
	float v1[3] = { 0, 0,-1};
	float v2[3] = {-1, 0, 0};
	float v3[3] = { 0, 0, 1};
	float v4[3] = { 1, 0, 0};
	float v5[3] = {-1, 0, 1};
	float v6[3] = { 1, 0,-1};
	float n1[3] = {};
	float n2a[3] = {};
	float n2b[3] = {};
	float n3[3] = {};
	float n4a[3] = {};
	float n4b[3] = {};
	for(int x = 0; x < dim; x++){
		for(int z = 0;  z < dim; z++){
			avgNum = 0;
			if (x-1 >= 0){
				v2[1] = points[x-1][z] - points[x][z];
				if(z-1 >= 0){
					v1[1] = points[x][z-1] - points[x][z];
					n1[0] = v2[1]*v1[2] - v2[2]*v1[1];
					n1[1] = v2[2]*v1[0] - v2[0]*v1[2];
					n1[2] = v2[0]*v1[1] - v2[1]*v1[0];
					addToNormals(x, z, n1);
					avgNum++;
				}
				if(z+1 < dim){
					v3[1] = points[x][z+1] - points[x][z];
					if(faceShape == 't'){
						v5[1] = points[x-1][z+1] - points[x][z];
						n2a[0] = v3[1]*v5[2] - v3[2]*v5[1];
						n2a[1] = v3[2]*v5[0] - v3[0]*v5[2];
						n2a[2] = v3[0]*v5[1] - v3[1]*v5[0];
						n2b[0] = v5[1]*v2[2] - v5[2]*v2[1];
						n2b[1] = v5[2]*v2[0] - v5[0]*v2[2];
						n2b[2] = v5[0]*v2[1] - v5[1]*v2[0];
						addToNormals(x,z, n2a);
						addToNormals(x,z, n2b);
						avgNum += 2;
					}
					else{
						n2a[0] = v3[1]*v2[2] - v3[2]*v2[1];
						n2a[1] = v3[2]*v2[0] - v3[0]*v2[2];
						n2a[2] = v3[0]*v2[1] - v3[1]*v2[0];
						addToNormals(x, z, n2a);
						avgNum++;
					}
				}
			}
			if (x+1 < dim){
				v4[1] = points[x+1][z] - points[x][z];
				if(z+1 < dim){
					v3[1] = points[x][z+1] - points[x][z];
					n3[0] = v4[1]*v3[2] - v4[2]*v3[1];
					n3[1] = v4[2]*v3[0] - v4[0]*v3[2];
					n3[2] = v4[0]*v3[1] - v4[1]*v3[0];
					addToNormals(x, z, n3);
					avgNum++;
				}
				if(z-1 >= 0){
					v1[1] = points[x][z-1] - points[x][z];
					if(faceShape == 't'){
						v6[1] = points[x+1][z-1] - points[x][z];
						n4a[0] = v6[1]*v4[2] - v6[2]*v4[1];
						n4a[1] = v6[2]*v4[0] - v6[0]*v4[2];
						n4a[2] = v6[0]*v4[1] - v6[1]*v4[0];
						n4b[0] = v1[1]*v6[2] - v1[2]*v6[1];
						n4b[1] = v1[2]*v6[0] - v1[0]*v6[2];
						n4b[2] = v1[0]*v6[1] - v1[1]*v6[0];
						addToNormals(x,z, n4a);
						addToNormals(x,z, n4b);
						avgNum += 2;
					}
					else{
						n4a[0] = v1[1]*v4[2] - v1[2]*v4[1];
						n4a[1] = v1[2]*v4[0] - v1[0]*v4[2];
						n4a[2] = v1[0]*v4[1] - v1[1]*v4[0];
						addToNormals(x, z, n4a);
						avgNum++;
					}
				}
			}
			normals[x][z][0] = normals[x][z][0]/avgNum;
			normals[x][z][1] = normals[x][z][1]/avgNum;
			normals[x][z][2] = normals[x][z][2]/avgNum;
		}
	}
}

//Generates the faces for the grid. 
void makeFaces(char type){
	faceShape = type; 
	int c = 0;
	if (faceShape == 't'){
		faceNum = (dim-1)*(dim-1)*2;
		faces = (int **) calloc ((dim-1)*(dim-1)*2, sizeof(int *));
		for(int i = 0; i < (dim-1)*(dim-1)*2; i++){
			faces[i] = (int *) calloc (6, sizeof(int));
		}
		for(int x = 0; x < dim-1; x++){
			for(int z = 0;  z < dim-1; z++){
				faces[c][0] = x;
				faces[c][1] = z;
				faces[c][2] = x+1;
				faces[c][3] = z;
				faces[c][4] = x;
				faces[c][5] = z+1;
				faces[c+1][0] = x+1;
				faces[c+1][1] = z;
				faces[c+1][2] = x+1;
				faces[c+1][3] = z+1;
				faces[c+1][4] = x;
				faces[c+1][5] = z+1;
				c = c+2;
			}
		}
	}
	else if (faceShape == 'q'){
		faces = (int **) calloc ((dim-1)*(dim-1), sizeof(int *));
		faceNum = (dim-1)*(dim-1);
		for(int i = 0; i < (dim-1)*(dim-1); i++){
			faces[i] = (int *) calloc (8, sizeof(int));
		}
		for(int x = 0; x < dim-1; x++){
			for(int z = 0;  z < dim-1; z++){
				faces[c][0] = x;
				faces[c][1] = z;
				faces[c][2] = x+1;
				faces[c][3] = z;
				faces[c][4] = x+1;
				faces[c][5] = z+1;
				faces[c][6] = x;
				faces[c][7] = z+1;
				c++;
			}
		}
	}
	else{
		printf("MakeFaces Error");
	}
}

// A basic function that returns the distance between two points as a floating points value. 
float dis (float x1, float y1, float x2, float y2){
	return (float)sqrt(double((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)));	
}

void flattenPoints(){
	for(int x = 0; x < dim; x++){
		for(int z = 0; z < dim; z++){
			points[x][z] = 0.0f; 
		}
	}
}

//A Helper function for the TerrainCircles method
float bumpY (int xSpec, int zSpec, int rad, float maxH){
	float pd;
	for (int x = 0; x < dim; x++){
		for (int z = 0; z < dim; z++){	
			pd = dis(xSpec,zSpec,x,z) * 2 / rad;
			if (fabs(pd) <= 1.0){
				points[x][z]+=  maxH/2 + cos(pd*3.14)*maxH/2;
				if (highest < points[x][z])
					highest = points[x][z];
			}
		}
	}
}

//Generates the terrain as specified by the "Circles" algorithm. Takes a random point and bumps its Y value. 
void terrainCircles (float maxH){
	int rad, x ,z;
	highest = 1;
	lowest = 0;
	float pd;
	srand(time(0));
	for (int i = 0; i < gIter; i++){
		x = rand()%dim; 
		z = rand()%dim;
		rad = maxRad - rand()%(maxRad-minRad);
		//printf("Radius %i\n", rad);
		//rad = 5;
		bumpY(x,z,rad,maxH);
	}
	camPos[1] = 2* highest;
	setNormals();
}

//A helper function for the TerrainFault method 
void faultY(int vx, int vz, int x1, int z1, float maxH){
	int tx, tz;
	for(int x = 0; x < dim; x++){
		for(int z = 0; z < dim; z++){
			tx = x - x1;
			tz = z - z1;			
			if (vx * tz - vz*tx >0){
				points[x][z] += maxH;
				if (points[x][z] > highest)
					highest = points[x][z];
			}
			else{
				points[x][z] -= maxH;
				if (points[x][z] < lowest)
					lowest = points[x][z];
			}
		}
	}
}

//Generates terrain based on the Fault Algorithm 
void terrainFault(float maxH){
	int x1, z1, x2, z2;
	int vx,vz;
	highest = 1;
	lowest = 0;
	srand(time(0));
	for (int i = 0; i < gIter; i++){
		x1 = rand()%dim; 
		z1 = rand()%dim;
		x2 = rand()%dim; 
		z2 = rand()%dim;
		if (x2 == x1)
			x2++;
		if (z2 == z1)
			z2++;
		vz = (z2-z1);
		vx = (x2-x1);
		//printf("p1: %i, %i p2: %i, %i abc: %i. %i, %i\n",x1,z1,x2,z2,a,b,c);
		faultY(vx,vz,x1,z1, maxH/10);
	}
	//printf("highets and lowest: %f, %f\n",highest, lowest);
	camPos[1] = 2* highest;
	setNormals();
}

//A Display function for the second window generated by pressing 'm' 
void display2(void){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(-2, 300, -2, 300, -2, 300);
	glBegin(GL_POINTS);
		glVertex3f(0,0,0);
		glVertex3f(dim,dim,0);
		for (int x = 0; x < dim; x++){
			for (int z = 0; z < dim; z++){
				//glColor3f(1.0,0,0);
				glColor3f(points[x][z]/highest, points[x][z]/highest, points[x][z]/highest);
				glVertex3f(x,z,0);
			}
		}
	glEnd();
	glutSwapBuffers();
}

//A Helper function for the second glut window made by pressing 'm' 
void mouse(int btn, int state, int x, int y){
	if(btn == GLUT_LEFT_BUTTON || btn == GLUT_RIGHT_BUTTON){
		glutPostRedisplay();
	}
}

//Generates a window that shows heightMaps
void makeNewWindow(){
	if (isMap)
		return;
	glutInitWindowSize(300, 300);
	glutInitWindowPosition(150, 150);
	WindowID2 = glutCreateWindow("Map");	//creates the window
	glutDisplayFunc(display2);
	glutMouseFunc(mouse);
}

//Sets The global values for the program. 
void setGlobals(int size, int min, int iter, char type){
	dim = size + 1;
	faceShape = type;
	glDisable(GL_LIGHTING);
	fHeight = ((float)size)/15;
	drawType = 'w';
	origPos[0] = (float)(size)/2;
	origPos[1] = 0;
	origPos[2] = (float)(size)/2;
	camPos[0] = (float)size/2;
	camPos[1] = (float)size;
	camPos[2] = -size;
	gIter = iter;
	minRad = min;
	maxRad = dim/2; 
	isFlat = false;
	isLight = false; 
	points = (float **) calloc (dim, sizeof(float *));
	normals = (float ***) calloc (dim, sizeof(float**));
	resetLight();
	for(int x = 0; x < dim; x++){
		points[x] = (float*) calloc(dim, sizeof(float));
		normals[x] = (float**) calloc(dim,sizeof(float*));
		for(int z = 0; z < dim; z++){
			points[x][z] = 0.0f;
			normals[x][z] = (float*) calloc(3,sizeof(float));
		}
	}
	makeFaces(faceShape);
	terrainCircles(fHeight);
	//printArray(points, dim, dim);
	//printArray(faces, faceNum, faceShape);
}

//Reads the keyboard input for the program 
void keyboard(unsigned char key, int x, int y){
	/* key presses move the cube, if it isn't at the extents (hard-coded here) */
	switch (key)
	{
		case 'q':
		case 27:
			exit (0);
			break;
		case 'f':
		case 'F':
			flattenPoints();
			resetLight();
			terrainFault(fHeight);
			break;
		case 'w':
		case 'W':
			if (drawType == 'b')
				drawType = 'f';
			else if (drawType == 'w')
				drawType = 'b';
			else
				drawType = 'w';
			break;
			
		case 'l':
		case 'L':
			isLight = !isLight;
			lightMeUp(isLight); 
			break;
		case 's':
		case 'S':
			isFlat = !isFlat; 
			if (isFlat && isLight)
				glShadeModel(GL_FLAT);
			else if (!isFlat &&  isLight)
				glShadeModel(GL_SMOOTH);
			else
				printf("Error: Cannot alter shading when lighting is off (press L)\n");
			break;
		case 'r':
		case 'R':
			flattenPoints();
			resetLight();
			terrainCircles(fHeight);
			break;
		case 'h':
		case 'H':
			printHelp();
			break;
		case 'm':
		case 'M':
			makeNewWindow();
			isMap = true;
			break;
		case 'T':
		case 't':
			makeFaces('t');
			//printf("highets and lowest: %f, %f\n",highest, lowest);
			break;
		case 'Y':
		case 'y':
			makeFaces('q');
			//printf("highets and lowest: %f, %f\n",highest, lowest);
			break;
		case 'g':
			lightPos2[2] += 1;
			glLightfv(GL_LIGHT1, GL_POSITION, lightPos2);
			printf("Light 2 at: %f, %f, %f\n", lightPos2[0],lightPos2[1],lightPos2[2]);
			break;
		case 'G':
			lightPos[2] += 1;
			glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
			printf("Light 1 at: %f, %f, %f\n", lightPos[0],lightPos[1],lightPos[2]);
			break;
		case 'b':
			lightPos2[2] -= 1;
			glLightfv(GL_LIGHT1, GL_POSITION, lightPos2);
			printf("Light 2 at: %f, %f, %f\n", lightPos2[0],lightPos2[1],lightPos2[2]);
			break;
		case 'B':
			lightPos[2] -= 1;
			glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
			printf("Light 1 at: %f, %f, %f\n", lightPos[0],lightPos[1],lightPos[2]);
			break;
		case 'n':
			lightPos2[0] -= 1;
			glLightfv(GL_LIGHT1, GL_POSITION, lightPos2);
			printf("Light 2 at: %f, %f, %f\n", lightPos2[0],lightPos2[1],lightPos2[2]);
			break;
		case 'N':
			lightPos[0] -= 1;
			glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
			printf("Light 1 at: %f, %f, %f\n", lightPos[0],lightPos[1],lightPos[2]);
			break;
		case 'v':
			lightPos2[0] -= 1;
			glLightfv(GL_LIGHT1, GL_POSITION, lightPos2);
			printf("Light 2 at: %f, %f, %f\n", lightPos2[0],lightPos2[1],lightPos2[2]);
			break;
		case 'V':
			lightPos[0] += 1;
			glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
			printf("Light 1 at: %f, %f, %f\n", lightPos[0],lightPos[1],lightPos[2]);
			break;
		case 'c':
			printf("Camera at: %f, %f, %f\n", camPos[0],camPos[1],camPos[2]);
			break;
		case '1':
			gIter -= 100;
			if (gIter < 100 || gIter == 0)
				gIter = 100;
			printf("Iterations for the next generation are %i\n", gIter);
			break;
		case '2':
			gIter += 100;
			if (gIter > 1000)
				gIter = 1000;
			printf("Iterations for the next generation are %i\n", gIter);
			break;
		case 'k':
		case 'K':
			if(isLight && !isL1)
				glEnable(GL_LIGHT0);
			else
				glDisable(GL_LIGHT0);
			isL1 = !isL1;
			break;
		case 'p':
		case 'P':
			if(isLight && !isL2)
				glEnable(GL_LIGHT1);
			else
				glDisable(GL_LIGHT1);
			isL2 = !isL2;
			break;
		case '4':
			dim += 49;
			if (dim > 300){
				dim = 300;
				printf("Error: Cannot expand grid\n");
			}
			else{
				printf("increasing grid size\n");
				setGlobals (dim, minRad, gIter, faceShape);
			}
			break;
		case '3':
			dim -= 51;
			if (dim < 50 ){
				dim = 50;
				printf("Error: Cannot shrink grid\n");
			}
			else{
				printf("decreasing grid size\n");
				setGlobals(dim,minRad, gIter, faceShape);
			}
			break;
	}
	lightPos[1] = highest;
	lightPos2[1] = highest;
	glutPostRedisplay();
}

//The Display function for the program. Draws the grid. 
void display(void){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(camPos[0], camPos[1], camPos[2], origPos[0],origPos[1],origPos[2], 0,1,0);
	
	for (int i = 0; i < faceNum; i++){
		if (drawType == 'f' || drawType == 'b')
			drawFace(i);
		if (drawType == 'w' || drawType == 'b'){
			glColor3f(0,0,1);
			glLineWidth(2.0);
			drawWire(i);
			glLineWidth(1.0);
		}	
	}
	//glColor3f(0,1,1);
	//drawFace(0);
	//glColor3f(1,0,1);
	//drawFace(1);
	glutSwapBuffers();
}

//Initializes the window for the program. 
void init(void){
	glClearColor(0, 0, 0, 0);
	glColor3f(1, 1, 1);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glOrtho(-2, 2, -2, 2, -2, 2);
	gluPerspective(45, 1, 1, (dim-1)*4);
}

//Manages the arrow key presses used for camera movement 
void special(int key, int x, int y)
{
	/* arrow key presses move the camera */
	switch(key)
	{
		case GLUT_KEY_LEFT:
			camPos[0]-=(dim-1)/10;
			if (camPos[0] == 0)
				camPos[0] -= (dim-1)/10;
			break;

		case GLUT_KEY_RIGHT:
			camPos[0]+=(dim-1)/10;
			break;

		case GLUT_KEY_UP:
			camPos[2] -= (dim-1)/10;
			if (camPos[2] == 0)
				camPos[2] -= (dim-1)/10;
			break;

		case GLUT_KEY_DOWN:
			camPos[2] += (dim-1)/10;
			break;
		
		case GLUT_KEY_HOME:
			camPos[1] += (dim-1)/10;
			break;

		case GLUT_KEY_END:
			camPos[1] -= (dim-1)/10;
			break;
	}
	glutPostRedisplay();
}

/* main function - program entry point */
int main(int argc, char** argv){
	glutInit(&argc, argv);		//starts up GLUT
	
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	
	
	glutInitWindowSize(600, 600);
	glutInitWindowPosition(100, 100);
	WindowID1 = glutCreateWindow("Terrain Generation");	//creates the window
	glutDisplayFunc(display);	//registers "display" as the display callback function
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glEnable(GL_DEPTH_TEST);
	setGlobals(150,2,100,'t');
	init();
	printHelp();
	glFrontFace(GL_CW); 
	glCullFace(GL_BACK); 
	glEnable(GL_CULL_FACE);
	glutMainLoop();				//starts the event loop
	return(0);					//return may not be necessary on all compilers
}