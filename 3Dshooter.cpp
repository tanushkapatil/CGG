#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <string>

// ================== CAMERA ==================
float camX = 0.0f, camY = 1.5f, camZ = 5.0f;
float camYaw = 0.0f, camPitch = 0.0f;

// ================== BULLET ==================
struct Bullet {
    float x, y, z;
    float dx, dy, dz;
    bool active;
};
std::vector<Bullet> bullets;

// ================== TARGET ==================
struct Target {
    float x, y, z;
    bool alive;
    float spinAngle;
    float r, g, b; // fixed color
};
std::vector<Target> targets;

// ================== SCORE ==================
int score = 0;

// ================== SHOOT BULLET ==================
void shootBullet() {
    Bullet b;
    b.x = camX;
    b.y = 1.5f;
    b.z = camZ;

    float speed = 0.5f;
    b.dx = sinf(camYaw) * cosf(camPitch) * speed;
    b.dy = sinf(camPitch) * speed;
    b.dz = -cosf(camYaw) * cosf(camPitch) * speed;
    b.active = true;

    bullets.push_back(b);
}

// ================== DRAW GROUND ==================
void drawGround() {
    glColor3f(0.1f, 0.6f, 0.1f); // green grass
    glBegin(GL_QUADS);
    glVertex3f(-50, 0, -50);
    glVertex3f(50, 0, -50);
    glVertex3f(50, 0, 50);
    glVertex3f(-50, 0, 50);
    glEnd();
}

// ================== DRAW SKY ==================
void drawSky() {
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glColor3f(0.5f, 0.7f, 1.0f); // light blue top
    glVertex3f(-50, 50, -50);
    glVertex3f(50, 50, -50);
    glColor3f(0.2f, 0.4f, 0.8f); // darker near horizon
    glVertex3f(50, 0, -50);
    glVertex3f(-50, 0, -50);
    glEnd();
    glEnable(GL_LIGHTING);
}

// ================== DRAW TARGET ==================
void drawTarget(Target& t) {
    glPushMatrix();
    glTranslatef(t.x, t.y, t.z);
    glRotatef(t.spinAngle, 0, 1, 0);

    glColor3f(t.r, t.g, t.b); // fixed color
    glutSolidCube(1.0f);

    glPopMatrix();
}

// ================== DRAW BULLETS ==================
void drawBullets() {
    for (auto& b : bullets) {
        if (b.active) {
            glPushMatrix();
            glTranslatef(b.x, b.y, b.z);
            glColor3f(1.0f, 0.8f, 0.0f); // glowing yellow
            glutSolidSphere(0.1, 12, 12);
            glPopMatrix();
        }
    }
}

// ================== UPDATE BULLETS ==================
void updateBullets() {
    for (auto& b : bullets) {
        if (b.active) {
            b.x += b.dx;
            b.y += b.dy;
            b.z += b.dz;

            // Check collision with targets
            for (auto& t : targets) {
                if (t.alive &&
                    fabs(b.x - t.x) < 0.6 &&
                    fabs(b.y - t.y) < 0.6 &&
                    fabs(b.z - t.z) < 0.6) {
                    t.alive = false;
                    b.active = false;
                    score += 10;
                }
            }
        }
    }
}

// ================== DRAW GUN ==================
void drawGun() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(60, 800.0 / 600.0, 0.1, 100);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTranslatef(0.5f, -0.5f, -1.2f);

    // Body
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glScalef(0.3f, 0.2f, 0.6f);
    glutSolidCube(0.5f);
    glPopMatrix();

    // Barrel
    glColor3f(0.6f, 0.6f, 0.6f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -0.5f);
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, 0.05, 0.05, 0.4, 16, 16);
    gluDeleteQuadric(quad);
    glPopMatrix();

    // Handle
    glColor3f(0.4f, 0.2f, 0.05f);
    glPushMatrix();
    glTranslatef(0.0f, -0.25f, 0.1f);
    glRotatef(70, 1, 0, 0);
    glScalef(0.15f, 0.5f, 0.2f);
    glutSolidCube(0.5f);
    glPopMatrix();

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ================== DISPLAY FUNCTION ==================
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float lx = cosf(camPitch) * sinf(camYaw);
    float ly = sinf(camPitch);
    float lz = -cosf(camYaw) * cosf(camPitch);

    gluLookAt(camX, camY, camZ,
        camX + lx, camY + ly, camZ + lz,
        0, 1, 0);

    drawSky();
    drawGround();

    for (auto& t : targets)
        if (t.alive) drawTarget(t);

    drawBullets();
    drawGun();

    // Score display
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.0f);
    std::string scoreText = "Score: " + std::to_string(score);
    glRasterPos2f(-3.5f, 2.0f);
    for (char c : scoreText) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    glEnable(GL_LIGHTING);

    glutSwapBuffers();
}

// ================== IDLE FUNCTION ==================
void idle() {
    updateBullets();

    // Spin targets
    for (auto& t : targets) {
        if (t.alive) t.spinAngle += 0.5f;
    }

    glutPostRedisplay();
}

// ================== KEYBOARD ==================
void keyboard(unsigned char key, int, int) {
    float speed = 0.3f;
    float lx = sinf(camYaw);
    float lz = -cosf(camYaw);

    if (key == 'w') { camX += lx * speed; camZ += lz * speed; }
    if (key == 's') { camX -= lx * speed; camZ -= lz * speed; }
    if (key == 'a') { camX += lz * speed; camZ -= lx * speed; }
    if (key == 'd') { camX -= lz * speed; camZ += lx * speed; }
    if (key == 27) exit(0);
}

// ================== MOUSE LOOK ==================
void mouseMotion(int x, int y) {
    static bool warp = false;
    if (warp) { warp = false; return; }

    int cx = glutGet(GLUT_WINDOW_WIDTH) / 2;
    int cy = glutGet(GLUT_WINDOW_HEIGHT) / 2;

    float dx = (x - cx) * 0.002f;
    float dy = (y - cy) * 0.002f;

    camYaw += dx;
    camPitch -= dy;

    if (camPitch > 1.5f) camPitch = 1.5f;
    if (camPitch < -1.5f) camPitch = -1.5f;

    glutWarpPointer(cx, cy);
    warp = true;
}

// ================== MOUSE CLICK ==================
void mouseClick(int button, int state, int, int) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) shootBullet();
}

// ================== RESHAPE ==================
void reshape(int w, int h) {
    if (h == 0) h = 1;
    float ratio = 1.0f * w / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, ratio, 0.1, 100);
    glViewport(0, 0, w, h);
    glMatrixMode(GL_MODELVIEW);
}

// ================== MAIN ==================
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("FPS Shooting Game");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);

    GLfloat lightPos[] = { 0.0f, 10.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // Create targets with fixed random colors
    for (int i = 0; i < 10; i++) {
        targets.push_back({
            (float)(rand() % 20 - 10),
            0.5f,
            (float)(-(rand() % 20)),
            true,
            0.0f,
            (rand() % 100) / 100.0f,   // R
            (rand() % 100) / 100.0f,   // G
            (rand() % 100) / 100.0f    // B
            });
    }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutPassiveMotionFunc(mouseMotion);
    glutMouseFunc(mouseClick);
    glutIdleFunc(idle);

    glutSetCursor(GLUT_CURSOR_NONE);
    glutMainLoop();
    return 0;
}
