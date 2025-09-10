// main.cpp
// 2D Bubble Shooter using classic raster algorithms (DDA, Bresenham, Midpoint Circle).
// Uses GLFW and fixed-function OpenGL (glBegin/glVertex).
// Build: link with glfw and OpenGL (opengl32.lib on Windows or -lGL on Linux).

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

// ----- Utilities -----
int SCR_W = 900;
int SCR_H = 700;

struct Color { float r, g, b; };
struct Bubble {
    float x, y;      // screen coordinates (center)
    float z;        // pseudo-depth 0..1 (0 near, 1 far)
    float radius;   // base radius in pixels (before depth)
    float vx, vy;   // velocity in screen coords
    Color col;
    bool alive;
};
struct Projectile {
    float x, y;
    float vx, vy;
    float life; // seconds
    bool alive;
};

// clamp helper
float clampf(float a, float b, float v) { return v < a ? a : (v > b ? b : v); }

// ----- Drawing primitives (pixel algorithms) -----
// We will draw into orthographic screen coordinates matching window pixels:
// origin (0,0) bottom-left. We'll map we will use glVertex2i with integer coords.

// Set an integer pixel (draw as GL_POINT)
void drawPixel(int x, int y) {
    glVertex2i(x, y);
}

// Midpoint circle algorithm (draws circle perimeter) - integer version
void drawCircleMidpoint(int xc, int yc, int r) {
    if (r < 0) return;
    int x = 0;
    int y = r;
    int d = 1 - r;

    while (x <= y) {
        // eight octants
        drawPixel(xc + x, yc + y);
        drawPixel(xc - x, yc + y);
        drawPixel(xc + x, yc - y);
        drawPixel(xc - x, yc - y);
        drawPixel(xc + y, yc + x);
        drawPixel(xc - y, yc + x);
        drawPixel(xc + y, yc - x);
        drawPixel(xc - y, yc - x);

        if (d < 0) {
            d += 2 * x + 3;
        }
        else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
    }
}

// Fill a circle by drawing concentric circles (cheap classic approach)
void fillCircleMidpoint(int xc, int yc, int r) {
    for (int rr = r; rr >= 0; --rr) {
        drawCircleMidpoint(xc, yc, rr);
    }
}

// DDA line algorithm
void drawLineDDA(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (steps == 0) {
        drawPixel(x0, y0);
        return;
    }
    float Xinc = dx / (float)steps;
    float Yinc = dy / (float)steps;
    float x = x0;
    float y = y0;
    for (int i = 0; i <= steps; ++i) {
        drawPixel((int)roundf(x), (int)roundf(y));
        x += Xinc;
        y += Yinc;
    }
}

// Bresenham's line algorithm (int) - general
void drawLineBresenham(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        drawPixel(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

// Helper to draw thicker pixel by drawing small cross of points
void drawThickPixel(int x, int y, int thickness = 2) {
    for (int ox = -thickness; ox <= thickness; ++ox)
        for (int oy = -thickness; oy <= thickness; ++oy)
            drawPixel(x + ox, y + oy);
}

// ----- Game state -----
vector<Bubble> bubbles;
vector<Projectile> projectiles;

double lastTime = 0.0;
int score = 0;

// spawn a bubble with pseudo-depth and random color + velocity
void spawnBubble() {
    Bubble b;
    // spawn in upper half at random X
    b.x = rand() % (SCR_W - 120) + 60;
    b.y = rand() % (SCR_H / 2) + SCR_H / 2;
    b.z = ((float)(rand() % 100)) / 200.0f; // near 0 to 0.5
    b.radius = rand() % 18 + 18; // base radius
    // velocity small (bubbles drift)
    b.vx = (rand() % 100 - 50) / 100.0f; // -0.5 .. 0.5
    b.vy = -(rand() % 50 + 10) / 100.0f; // downward drift
    b.col.r = 0.4f + (rand() % 60) / 150.0f;
    b.col.g = 0.4f + (rand() % 60) / 150.0f;
    b.col.b = 0.4f + (rand() % 60) / 150.0f;
    b.alive = true;
    bubbles.push_back(b);
}

// shoot projectile from (gunX,gunY) toward target; speed constant
void shootProjectile(float x, float y, float tx, float ty) {
    float dx = tx - x;
    float dy = ty - y;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-6f) return;
    Projectile p;
    p.x = x; p.y = y;
    float speed = 600.0f; // pixels/sec
    p.vx = dx / len * speed;
    p.vy = dy / len * speed;
    p.life = 3.0f;
    p.alive = true;
    projectiles.push_back(p);
}

// ----- Input state -----
double mouseX = SCR_W / 2.0, mouseY = SCR_H / 2.0;
bool mouseMoved = false;
bool fireRequested = false;

// GLFW callbacks
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    // convert OpenGL window coords: GLFW gives top-left? By default y=0 at top in window? Actually GLFW gives y with origin top-left of window; for our pixel coords we want bottom-left origin:
    mouseY = SCR_H - ypos;
    mouseMoved = true;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) fireRequested = true;
}

// ----- Collision helper -----
bool circleCollision(float x1, float y1, float r1, float x2, float y2, float r2) {
    float dx = x1 - x2, dy = y1 - y2;
    float d2 = dx * dx + dy * dy;
    float rsum = r1 + r2;
    return d2 <= rsum * rsum;
}

// ----- Rendering procedures using the algorithms -----
// We will draw with GL_POINTS inside GL_BEGIN/GL_END, and use glVertex2i via our drawPixel wrapper.
// We need to call glBegin(GL_POINTS) then call algorithm functions which call drawPixel() -> glVertex2i.
// To set color we call glColor3f before glBegin.

void drawBubbleClassic(const Bubble& b) {
    // compute screen radius with pseudo depth (farther means smaller)
    float depthScale = 1.0f - clampf(0.0f, 0.8f, b.z);
    int r = (int)roundf(b.radius * depthScale);
    int xc = (int)roundf(b.x);
    int yc = (int)roundf(b.y);
    // draw filled bubble by concentric circles (edge rendered by midpoint)
    glColor3f(b.col.r, b.col.g, b.col.b);
    glBegin(GL_POINTS);
    fillCircleMidpoint(xc, yc, r);
    glEnd();
    // draw shiny highlight (small white circle at top-left)
    int hx = xc - r / 3;
    int hy = yc + r / 3;
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_POINTS);
    fillCircleMidpoint(hx, hy, max(1, r / 6));
    glEnd();
    // draw outline (slightly darker)
    glColor3f(max(0.0f, b.col.r - 0.18f), max(0.0f, b.col.g - 0.18f), max(0.0f, b.col.b - 0.18f));
    glBegin(GL_POINTS);
    drawCircleMidpoint(xc, yc, r);
    glEnd();
}

void drawProjectileClassic(const Projectile& p) {
    // draw projectile as small filled circle
    glColor3f(1.0f, 0.9f, 0.6f);
    glBegin(GL_POINTS);
    fillCircleMidpoint((int)roundf(p.x), (int)roundf(p.y), 3);
    glEnd();
}

void drawLauncherClassic(int baseX, int baseY, int aimX, int aimY) {
    // draw a small base circle
    glColor3f(0.2f, 0.2f, 0.25f);
    glBegin(GL_POINTS);
    fillCircleMidpoint(baseX, baseY, 10);
    glEnd();

    // draw barrel using Bresenham (thicker)
    glColor3f(0.85f, 0.85f, 0.9f);
    glBegin(GL_POINTS);
    // compute barrel end a bit ahead of aim direction
    float dx = aimX - baseX;
    float dy = aimY - baseY;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1e-6f) len = 1;
    float bx = baseX + dx / len * 40.0f;
    float by = baseY + dy / len * 40.0f;
    drawLineBresenham(baseX, baseY, (int)roundf(bx), (int)roundf(by));
    // thicken barrel by drawing nearby parallel lines
    drawLineBresenham(baseX - 1, baseY, (int)roundf(bx) - 1, (int)roundf(by));
    drawLineBresenham(baseX + 1, baseY, (int)roundf(bx) + 1, (int)roundf(by));
    glEnd();
}

void drawAimingDDA(int x0, int y0, int x1, int y1) {
    // draw dashed aim line with DDA
    glColor3f(0.9f, 0.6f, 0.2f);
    // we will draw short segments every few pixels to create dashed style
    int dx = x1 - x0, dy = y1 - y0;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (steps == 0) return;
    float Xinc = dx / (float)steps;
    float Yinc = dy / (float)steps;
    float x = x0, y = y0;
    bool drawSeg = true;
    int dashLen = 8;
    int cnt = 0;
    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++) {
        if (drawSeg) drawPixel((int)roundf(x), (int)roundf(y));
        cnt++;
        if (cnt >= dashLen) { drawSeg = !drawSeg; cnt = 0; }
        x += Xinc; y += Yinc;
    }
    glEnd();
}

// ----- Main -----
int main() {
    srand((unsigned)time(nullptr));
    if (!glfwInit()) {
        cerr << "Failed to init GLFW\n";
        return -1;
    }

    // Request compatibility profile with old fixed-function pipeline (so glBegin/glVertex works)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    // create window
    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "Classic Algorithms Bubble Shooter", nullptr, nullptr);
    if (!window) {
        cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    // callbacks
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetKeyCallback(window, key_callback);
    // show cursor (we'll use it)
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    // viewport and orthographic projection set up in resize loop
    glViewport(0, 0, SCR_W, SCR_H);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // use pixel coordinates: left=0, right=SCR_W, bottom=0, top=SCR_H
    glOrtho(0, SCR_W, 0, SCR_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // set point size to 1
    glPointSize(1.0f);

    // spawn initial bubbles
    for (int i = 0; i < 8; i++) spawnBubble();

    lastTime = glfwGetTime();
    cout << "Controls: move mouse to aim, SPACE to shoot, ESC to quit\n";

    // Gun base position (bottom center)
    int gunX = SCR_W / 2;
    int gunY = 60;

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        // input: fire
        if (fireRequested) {
            shootProjectile((float)gunX, (float)gunY, (float)mouseX, (float)mouseY);
            fireRequested = false;
        }

        // occasionally spawn new bubbles
        if (bubbles.size() < 14 && (rand() % 100) < 5) spawnBubble();

        // update bubbles
        for (auto& b : bubbles) {
            if (!b.alive) continue;
            b.x += b.vx * dt * 60.0f; // scale velocities visually
            b.y += b.vy * dt * 60.0f;
            // bounce off sides
            if (b.x < 30) { b.x = 30; b.vx = fabs(b.vx); }
            if (b.x > SCR_W - 30) { b.x = SCR_W - 30; b.vx = -fabs(b.vx); }
            // remove if below screen
            if (b.y < -50) b.alive = false;
        }
        // remove dead bubbles
        bubbles.erase(remove_if(bubbles.begin(), bubbles.end(), [](const Bubble& bb) { return !bb.alive; }), bubbles.end());

        // update projectiles
        for (auto& p : projectiles) {
            if (!p.alive) continue;
            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.life -= dt;
            if (p.life <= 0.0f || p.x < -50 || p.x > SCR_W + 50 || p.y < -50 || p.y > SCR_H + 50) p.alive = false;
        }
        projectiles.erase(remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& pp) { return !pp.alive; }), projectiles.end());

        // collisions projectile <-> bubble
        for (auto& p : projectiles) if (p.alive) {
            for (auto& b : bubbles) if (b.alive) {
                // compute effective radius with depth
                float depthScale = 1.0f - clampf(0.0f, 0.8f, b.z);
                float r = b.radius * depthScale;
                if (circleCollision(p.x, p.y, 4.0f, b.x, b.y, r)) {
                    p.alive = false;
                    b.alive = false;
                    score += 10;
                    // sometimes spawn two smaller bubbles near the popped one
                    if (b.radius > 14 && (rand() % 100) < 50) {
                        for (int k = 0; k < 2; k++) {
                            Bubble nb;
                            nb.x = b.x + (rand() % 40 - 20);
                            nb.y = b.y + (rand() % 40 - 20);
                            nb.z = b.z + 0.05f * (rand() % 3);
                            nb.radius = b.radius * 0.6f;
                            nb.vx = (rand() % 100 - 50) / 120.0f;
                            nb.vy = (rand() % 50) / 120.0f;
                            nb.col = b.col;
                            nb.alive = true;
                            bubbles.push_back(nb);
                        }
                    }
                    break;
                }
            }
        }

        // --- render ---
        glClearColor(0.06f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw background grid very faint (using DDA lines)
        glColor3f(0.08f, 0.1f, 0.15f);
        // vertical grid lines every 60 px
        for (int gx = 0; gx <= SCR_W; gx += 60) {
            glBegin(GL_POINTS); drawLineDDA(gx, 0, gx, SCR_H); glEnd();
        }
        // horizontal grid lines
        for (int gy = 0; gy <= SCR_H; gy += 60) {
            glBegin(GL_POINTS); drawLineDDA(0, gy, SCR_W, gy); glEnd();
        }

        // draw bubbles (farthest first for nicer overlap)
        // sort by z descending (farther z larger) -> draw far first
        vector<int> order(bubbles.size());
        for (int i = 0; i < (int)bubbles.size(); ++i) order[i] = i;
        sort(order.begin(), order.end(), [&](int a, int b) { return bubbles[a].z > bubbles[b].z; });

        for (int idx : order) {
            drawBubbleClassic(bubbles[idx]);
        }

        // draw projectiles
        for (auto& p : projectiles) drawProjectileClassic(p);

        // draw launcher (gun) using Bresenham; barrel aimed at mouse
        drawLauncherClassic(gunX, gunY, (int)roundf(mouseX), (int)roundf(mouseY));

        // draw aiming dashed DDA line (from gun to mouse)
        drawAimingDDA(gunX, gunY, (int)roundf(mouseX), (int)roundf(mouseY));

        // draw HUD text using very simple blocky numbers (we'll draw score as circles/lines)
        // Instead: draw small score indicator as colored squares on top-left
        {
            int sx = 12, sy = SCR_H - 20;
            // draw score color bar
            glColor3f(0.9f, 0.9f, 0.2f);
            glBegin(GL_POINTS);
            for (int i = 0; i < 6; i++)
                for (int j = 0; j < 12; j++)
                    drawPixel(sx + i, sy - j);
            glEnd();
            // simple numeric print to console periodically
            static double tprint = 0;
            if (now - tprint > 0.4) {
                cout << "\rScore: " << score << "  Bubbles: " << bubbles.size() << "  Projectiles: " << projectiles.size() << "     " << flush;
                tprint = now;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    cout << "\nGame closed. Final score: " << score << endl;
    return 0;
}
