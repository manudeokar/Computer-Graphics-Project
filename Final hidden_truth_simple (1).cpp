/*==========================================================
  THE HIDDEN TRUTH  -  Simple 2D Detective Game (OpenGL + GLUT)
  Computer Graphics Mini Project

  Story : Gala party, lights go out at 9:45 PM, "Blue Diamond"
          is stolen. Detective Kabir finds 3 clues, questions
          3 suspects and accuses the thief (random every game).

  Controls:
     ENTER ........ start / skip scene
     W A S D or Arrow keys ... move
     E ............ pick clue / talk
     F ............ accuse suspect standing near you
     R ............ restart        ESC ... quit
==========================================================*/
#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
using namespace std;

const int W = 1000;
const int H = 650;

/* game states */
#define TITLE   0
#define PARTY   1
#define BLACKOUT 2
#define INVEST  3
#define WIN     4
#define LOSE    5

int state = TITLE;
int frame = 0;          /* counts frames inside current state */
int totalFrame = 0;     /* counts frames of whole game */
bool keys[256];
bool specialKeys[256];

/* detective */
float dx = 500, dy = 50;
int faceDir = 1;

/* suspects: 0 Arjun, 1 Maya, 2 Victor */
const char *sName[3]  = { "Arjun Rao", "Maya Kapoor", "Victor Shah" };
const char *sFirst[3] = { "Arjun", "Maya", "Victor" };
const char *sInit[3]  = { "A.R.", "M.K.", "V.S." };
const char *sRole[3]  = { "Event Manager", "Pop Singer", "Security Chief" };
const char *sAlibi[3] = {
    "I was on a call in the lobby. The signal died with the lights.",
    "I was on stage fixing my microphone. I saw nothing at all!",
    "I was guarding the east door all night. I never left my post."
};
float sx[3] = { 220, 500, 780 };
float sy[3] = { 200, 130, 200 };
float sCol[3][3] = { {0.2f, 0.3f, 0.7f}, {0.8f, 0.1f, 0.3f}, {0.1f, 0.5f, 0.3f} };

/* guests (just for hints) */
float gx[3] = { 100, 350, 900 };
float gy[3] = { 90, 60, 100 };

/* clues: 0 keycard, 1 phone, 2 watch */
const char *clueName[3] = { "Keycard", "Broken Phone", "Broken Watch" };
float cx[3], cy[3];
bool found[3];
float spots[6][2] = { {80,60}, {200,250}, {420,50}, {620,230}, {880,60}, {930,240} };

int culprit = 0;
int chances = 3;
int cluesFound = 0;

/* message box */
string msgTitle = "";
string msgText = "";
int msgTimer = 0;

/*------------------------------------------------------
   basic drawing functions
------------------------------------------------------*/
void rect(float x1, float y1, float x2, float y2) {
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x2, y1);
    glVertex2f(x2, y2);
    glVertex2f(x1, y2);
    glEnd();
}

/*------------------------------------------------------
   OUR OWN ALGORITHMS  (everything is drawn with points)
   1. DDA line algorithm
   2. Bresenham line algorithm
   3. Mid point circle algorithm   (filled circle)
   4. Bresenham circle algorithm   (circle outline)
   5. Scan line polygon fill algorithm
------------------------------------------------------*/
float pointScale = 1;      /* changes when the window is resized */

/* 1. DDA line */
void ddaLine(float x1, float y1, float x2, float y2, float w) {
    float lenX = x2 - x1;
    float lenY = y2 - y1;
    int steps = (int)(fabs(lenX) > fabs(lenY) ? fabs(lenX) : fabs(lenY));
    if (steps == 0) steps = 1;
    float xInc = lenX / steps;
    float yInc = lenY / steps;
    float x = x1, y = y1;

    glPointSize(w * pointScale);
    glBegin(GL_POINTS);
    for (int i = 0; i <= steps; i++) {
        glVertex2i((int)(x + 0.5f), (int)(y + 0.5f));
        x += xInc;
        y += yInc;
    }
    glEnd();
}

/* 2. Bresenham line */
void bresenhamLine(float fx1, float fy1, float fx2, float fy2, float w) {
    int x1 = (int)(fx1 + 0.5f), y1 = (int)(fy1 + 0.5f);
    int x2 = (int)(fx2 + 0.5f), y2 = (int)(fy2 + 0.5f);
    int ax = abs(x2 - x1);
    int ay = abs(y2 - y1);
    int stepX = (x1 < x2) ? 1 : -1;
    int stepY = (y1 < y2) ? 1 : -1;
    int err = ax - ay;

    glPointSize(w * pointScale);
    glBegin(GL_POINTS);
    while (1) {
        glVertex2i(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -ay) { err -= ay; x1 += stepX; }
        if (e2 < ax)  { err += ax; y1 += stepY; }
    }
    glEnd();
}

/* 3. Mid point circle (filled) */
void circle(float fx, float fy, float fr) {
    int xc = (int)(fx + 0.5f);
    int yc = (int)(fy + 0.5f);
    int r = (int)(fr + 0.5f);
    if (r < 1) r = 1;
    if (r > 400) r = 400;

    static int half[402];              /* half width of each row */
    for (int i = 0; i <= r; i++) half[i] = 0;

    int x = 0, y = r;
    int p = 1 - r;                     /* decision parameter */
    while (x <= y) {
        if (y > half[x]) half[x] = y;
        if (x > half[y]) half[y] = x;
        x++;
        if (p < 0) p += 2 * x + 1;
        else { y--; p += 2 * (x - y) + 1; }
    }

    glPointSize(pointScale);
    glBegin(GL_POINTS);
    for (int row = -r; row <= r; row++) {
        int a = (row < 0) ? -row : row;
        for (int k = -half[a]; k <= half[a]; k++)
            glVertex2i(xc + k, yc + row);
    }
    glEnd();
}

/* 4. Bresenham circle (outline only) */
void circleOutline(float fx, float fy, float fr, float w) {
    int xc = (int)(fx + 0.5f);
    int yc = (int)(fy + 0.5f);
    int r = (int)(fr + 0.5f);
    int x = 0, y = r;
    int d = 3 - 2 * r;

    glPointSize(w * pointScale);
    glBegin(GL_POINTS);
    while (y >= x) {
        glVertex2i(xc + x, yc + y);  glVertex2i(xc - x, yc + y);
        glVertex2i(xc + x, yc - y);  glVertex2i(xc - x, yc - y);
        glVertex2i(xc + y, yc + x);  glVertex2i(xc - y, yc + x);
        glVertex2i(xc + y, yc - x);  glVertex2i(xc - y, yc - x);
        x++;
        if (d > 0) { y--; d = d + 4 * (x - y) + 10; }
        else d = d + 4 * x + 6;
    }
    glEnd();
}

/* 5. Scan line polygon fill */
void scanlineFill(float px[], float py[], int n) {
    int yMin = (int)py[0], yMax = (int)py[0];
    for (int i = 1; i < n; i++) {
        if ((int)py[i] < yMin) yMin = (int)py[i];
        if ((int)py[i] > yMax) yMax = (int)py[i];
    }

    glPointSize(pointScale);
    glBegin(GL_POINTS);
    for (int yy = yMin; yy <= yMax; yy++) {
        float inter[10];               /* x where this scan line cuts the edges */
        int cnt = 0;
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            float x1 = px[i], y1 = py[i], x2 = px[j], y2 = py[j];
            if (y1 == y2) continue;    /* skip horizontal edges */
            if (y1 > y2) {
                float t = x1; x1 = x2; x2 = t;
                t = y1; y1 = y2; y2 = t;
            }
            if (yy >= y1 && yy < y2 && cnt < 10)
                inter[cnt++] = x1 + (yy - y1) * (x2 - x1) / (y2 - y1);
        }
        /* sort the intersections */
        for (int a = 0; a < cnt; a++)
            for (int b = a + 1; b < cnt; b++)
                if (inter[b] < inter[a]) { float t = inter[a]; inter[a] = inter[b]; inter[b] = t; }
        /* fill between pairs */
        for (int k = 0; k + 1 < cnt; k += 2)
            for (int xx = (int)(inter[k] + 0.5f); xx <= (int)(inter[k + 1] + 0.5f); xx++)
                glVertex2i(xx, yy);
    }
    glEnd();
}

/* helpers that use the algorithms above */
void line(float x1, float y1, float x2, float y2, float w) {
    ddaLine(x1, y1, x2, y2, w);
}

void triangle(float x1, float y1, float x2, float y2, float x3, float y3) {
    float px[3] = { x1, x2, x3 };
    float py[3] = { y1, y2, y3 };
    scanlineFill(px, py, 3);
}

void rectOutline(float x1, float y1, float x2, float y2) {
    bresenhamLine(x1, y1, x2, y1, 2);
    bresenhamLine(x2, y1, x2, y2, 2);
    bresenhamLine(x2, y2, x1, y2, 2);
    bresenhamLine(x1, y2, x1, y1, 2);
}

void drawText(float x, float y, const char *s, void *font) {
    glRasterPos2f(x, y);
    while (*s) {
        glutBitmapCharacter(font, *s);
        s++;
    }
}

void drawTextCenter(float x, float y, const char *s, void *font) {
    int w = 0;
    for (const char *p = s; *p; p++) w += glutBitmapWidth(font, *p);
    drawText(x - w / 2, y, s, font);
}

float distance(float x1, float y1, float x2, float y2) {
    float a = x1 - x2;
    float b = y1 - y2;
    return sqrt(a * a + b * b);
}

/*------------------------------------------------------
   message box
------------------------------------------------------*/
void showMsg(string title, string text, int time) {
    msgTitle = title;
    msgText = text;
    msgTimer = time;
}

void drawMsg() {
    if (msgTimer <= 0) return;

    /* break the text into lines of about 75 characters */
    string lines[6];
    int n = 0;
    string t = msgText;
    while (t.size() > 0 && n < 6) {
        if (t.size() <= 75) {
            lines[n++] = t;
            break;
        }
        int cut = t.rfind(' ', 75);
        if (cut < 0) cut = 75;
        lines[n++] = t.substr(0, cut);
        t = t.substr(cut + 1);
    }

    float h = 50 + n * 22;
    glColor3f(0.1f, 0.1f, 0.2f);
    rect(100, 30, 900, 30 + h);
    glColor3f(0.9f, 0.7f, 0.2f);
    rectOutline(100, 30, 900, 30 + h);

    glColor3f(1, 0.8f, 0.3f);
    drawText(115, 30 + h - 25, msgTitle.c_str(), GLUT_BITMAP_HELVETICA_18);
    glColor3f(1, 1, 1);
    for (int i = 0; i < n; i++)
        drawText(115, 30 + h - 50 - i * 22, lines[i].c_str(), GLUT_BITMAP_HELVETICA_18);
}

/*------------------------------------------------------
   people  (very simple: circle head + rectangle body)
------------------------------------------------------*/
void drawPerson(float x, float y, float r, float g, float b, bool armsUp, bool hat) {
    /* legs */
    glColor3f(0.1f, 0.1f, 0.15f);
    rect(x - 10, y, x - 2, y + 30);
    rect(x + 2, y, x + 10, y + 30);

    /* body */
    glColor3f(r, g, b);
    rect(x - 14, y + 30, x + 14, y + 80);

    /* arms */
    glColor3f(0.95f, 0.75f, 0.6f);
    if (armsUp) {
        line(x - 14, y + 75, x - 28, y + 105, 4);
        line(x + 14, y + 75, x + 28, y + 105, 4);
    } else {
        line(x - 14, y + 75, x - 22, y + 45, 4);
        line(x + 14, y + 75, x + 22, y + 45, 4);
    }

    /* head */
    glColor3f(0.95f, 0.75f, 0.6f);
    circle(x, y + 95, 14);
    glColor3f(0, 0, 0);
    circleOutline(x, y + 95, 14, 1);      /* head outline */

    /* eyes */
    glColor3f(0, 0, 0);
    circle(x - 5, y + 97, 2);
    circle(x + 5, y + 97, 2);

    /* hat (only detective) */
    if (hat) {
        glColor3f(0.4f, 0.25f, 0.1f);
        rect(x - 18, y + 106, x + 18, y + 111);
        rect(x - 11, y + 111, x + 11, y + 125);
    }
}

/*------------------------------------------------------
   clue pictures  (small shapes)
------------------------------------------------------*/
void drawClueIcon(int type, float x, float y, float s) {
    if (type == 0) {                       /* keycard */
        glColor3f(0.8f, 0.1f, 0.15f);
        rect(x - 25 * s, y - 15 * s, x + 25 * s, y + 15 * s);
        glColor3f(0.95f, 0.8f, 0.3f);
        rect(x - 18 * s, y - 8 * s, x - 6 * s, y + 4 * s);
    } else if (type == 1) {                /* phone */
        glColor3f(0.05f, 0.05f, 0.05f);
        rect(x - 14 * s, y - 24 * s, x + 14 * s, y + 24 * s);
        glColor3f(0.2f, 0.4f, 0.8f);
        rect(x - 11 * s, y - 20 * s, x + 11 * s, y + 18 * s);
        glColor3f(1, 1, 1);
        bresenhamLine(x - 10 * s, y + 15 * s, x + 2 * s, y - 5 * s, 2);   /* crack */
    } else {                               /* watch */
        glColor3f(0.4f, 0.25f, 0.1f);
        rect(x - 6 * s, y - 30 * s, x + 6 * s, y + 30 * s);
        glColor3f(0.9f, 0.7f, 0.2f);
        circle(x, y, 16 * s);
        glColor3f(1, 1, 1);
        circle(x, y, 12 * s);
        glColor3f(0, 0, 0);
        bresenhamLine(x, y, x, y + 8 * s, 2);
        bresenhamLine(x, y, x + 6 * s, y, 2);
    }
}

/*------------------------------------------------------
   background
------------------------------------------------------*/
void drawPedestal() {
    glColor3f(0.6f, 0.6f, 0.65f);
    rect(480, 200, 520, 255);
    glColor3f(0.8f, 0.8f, 0.85f);
    rect(470, 255, 530, 265);

    /* glass box */
    glColor4f(0.7f, 0.9f, 1, 0.3f);
    rect(478, 265, 522, 305);

    /* the diamond (only before the theft) */
    bool gem = (state < BLACKOUT);
    if (gem) {
        glColor3f(0.2f, 0.6f, 1);
        float gemX[5] = { 500, 514, 506, 494, 486 };
        float gemY[5] = { 266, 282, 296, 296, 282 };
        scanlineFill(gemX, gemY, 5);
    } else {
        /* broken glass lines */
        glColor3f(1, 1, 1);
        bresenhamLine(500, 285, 480, 305, 2);
        bresenhamLine(500, 285, 522, 300, 2);
        bresenhamLine(500, 285, 485, 267, 2);
    }
}

void drawBackground() {
    /* wall */
    glColor3f(0.25f, 0.18f, 0.3f);
    rect(0, 300, W, H);

    /* ground */
    glColor3f(0.4f, 0.25f, 0.15f);
    rect(0, 0, W, 300);

    drawPedestal();
}

/*------------------------------------------------------
   HUD
------------------------------------------------------*/
void drawHUD() {
    char buf[100];

    /* evidence box */
    glColor3f(0.1f, 0.1f, 0.2f);
    rect(10, 520, 340, 640);
    glColor3f(0.9f, 0.7f, 0.2f);
    rectOutline(10, 520, 340, 640);
    drawText(20, 618, "EVIDENCE LOG", GLUT_BITMAP_HELVETICA_18);

    for (int i = 0; i < 3; i++) {
        float y = 585 - i * 32;
        if (found[i]) {
            drawClueIcon(i, 40, y + 4, 0.5f);
            if (i == 0) sprintf(buf, "Keycard: initials '%s'", sInit[culprit]);
            else if (i == 1) sprintf(buf, "Phone: message to '%s'", sFirst[culprit]);
            else sprintf(buf, "Watch: stopped 9:47, '%s'", sFirst[culprit]);
            glColor3f(0.5f, 1, 0.5f);
        } else {
            sprintf(buf, "%s: not found", clueName[i]);
            glColor3f(0.6f, 0.6f, 0.7f);
        }
        drawText(75, y, buf, GLUT_BITMAP_HELVETICA_12);
    }

    /* chances (hearts as red squares) */
    glColor3f(0.9f, 0.7f, 0.2f);
    drawText(700, 620, "CHANCES:", GLUT_BITMAP_HELVETICA_18);
    for (int i = 0; i < 3; i++) {
        if (i < chances) glColor3f(0.9f, 0.1f, 0.2f);
        else glColor3f(0.3f, 0.3f, 0.3f);
        circle(830 + i * 30, 625, 10);
    }

    /* clue radar: how close is the nearest clue */
    float best = 99999;
    for (int i = 0; i < 3; i++)
        if (!found[i]) {
            float d = distance(dx, dy, cx[i], cy[i]);
            if (d < best) best = d;
        }
    glColor3f(0.9f, 0.7f, 0.2f);
    drawText(700, 585, "CLUE RADAR:", GLUT_BITMAP_HELVETICA_18);
    if (cluesFound == 3) {
        glColor3f(0.5f, 1, 0.5f);
        drawText(830, 585, "ALL FOUND!", GLUT_BITMAP_HELVETICA_18);
    } else {
        if (best > 400) { glColor3f(0.4f, 0.6f, 1); drawText(830, 585, "COLD", GLUT_BITMAP_HELVETICA_18); }
        else if (best > 180) { glColor3f(1, 0.8f, 0.2f); drawText(830, 585, "WARM", GLUT_BITMAP_HELVETICA_18); }
        else { glColor3f(1, 0.2f, 0.2f); drawText(830, 585, "HOT!", GLUT_BITMAP_HELVETICA_18); }
    }

    glColor3f(0.8f, 0.8f, 0.9f);
    drawTextCenter(500, 8, "WASD/Arrows: move    E: search / talk    F: accuse    R: restart",
                   GLUT_BITMAP_HELVETICA_12);
}

/*------------------------------------------------------
   display
------------------------------------------------------*/
void drawCharacters() {
    bool scared = (state == BLACKOUT || state == INVEST);

    /* guests */
    for (int i = 0; i < 3; i++) {
        float y = gy[i];
        if (state == PARTY) y += fabs(sin((totalFrame + i * 20) * 0.1f)) * 8;   /* dancing */
        drawPerson(gx[i], y, 0.9f - i * 0.2f, 0.6f, 0.2f + i * 0.3f, scared || state == PARTY, false);
    }
    /* suspects */
    for (int i = 0; i < 3; i++)
        drawPerson(sx[i], sy[i], sCol[i][0], sCol[i][1], sCol[i][2], state == WIN && i == culprit, false);

    /* detective */
    drawPerson(dx, dy, 0.55f, 0.42f, 0.28f, false, true);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    drawBackground();
    drawCharacters();

    /* ---------- TITLE ---------- */
    if (state == TITLE) {
        glColor4f(0, 0, 0.1f, 0.7f);
        rect(0, 0, W, H);
        glColor3f(0.9f, 0.7f, 0.2f);
        drawTextCenter(500, 460, "THE HIDDEN TRUTH", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(1, 1, 1);
        drawTextCenter(500, 400, "A celebrity gala. A sudden blackout. A stolen diamond.", GLUT_BITMAP_HELVETICA_18);
        drawTextCenter(500, 370, "Find 3 clues, question 3 suspects and accuse the thief.", GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.9f, 0.7f, 0.2f);
        drawTextCenter(500, 320, "Move: WASD / Arrows     E: search / talk     F: accuse", GLUT_BITMAP_HELVETICA_12);
        drawTextCenter(500, 300, "Wrong accusation = lose 1 chance (only 3 chances)", GLUT_BITMAP_HELVETICA_12);
        if ((totalFrame / 30) % 2 == 0) {
            glColor3f(1, 1, 1);
            drawTextCenter(500, 220, "PRESS ENTER TO START", GLUT_BITMAP_HELVETICA_18);
        }
    }

    /* ---------- PARTY intro ---------- */
    if (state == PARTY) {
        glColor3f(0, 0, 0);
        rect(0, 0, W, 50);
        glColor3f(1, 0.9f, 0.6f);
        if (frame < 250) drawTextCenter(500, 20, "9:30 PM - Grand Hotel. Detective Kabir arrives for a security check.", GLUT_BITMAP_HELVETICA_18);
        else drawTextCenter(500, 20, "The famous Blue Diamond is on display. The clock nears 9:45...", GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.7f, 0.7f, 0.8f);
        drawText(850, 630, "ENTER = skip", GLUT_BITMAP_HELVETICA_12);
    }

    /* ---------- BLACKOUT ---------- */
    if (state == BLACKOUT) {
        glColor4f(0, 0, 0, 0.92f);
        rect(0, 0, W, H);
        glColor3f(0.9f, 0.1f, 0.15f);
        drawTextCenter(500, 360, "BLACKOUT!", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(1, 0.9f, 0.6f);
        drawTextCenter(500, 300, "A scream... and when the lights return, the Blue Diamond is GONE!", GLUT_BITMAP_HELVETICA_18);
    }

    /* ---------- INVESTIGATION ---------- */
    if (state == INVEST) {
        /* clues are only visible near the detective's flashlight */
        for (int i = 0; i < 3; i++)
            if (!found[i] && distance(dx, dy, cx[i], cy[i]) < 150)
                drawClueIcon(i, cx[i], cy[i] + 8, 1.0f);

        /* darkness */
        glColor4f(0, 0, 0.03f, 0.82f);
        rect(0, 0, W, H);
        /* flashlight circle */
        glColor4f(1, 0.95f, 0.6f, 0.18f);
        circle(dx, dy + 55, 150);
        glColor4f(1, 0.95f, 0.6f, 0.12f);
        circle(dx, dy + 55, 100);
        glColor4f(1, 0.95f, 0.6f, 0.6f);
        circleOutline(dx, dy + 55, 150, 2);   /* edge of the flashlight */

        /* draw clues and people again inside the light so they look bright */
        for (int i = 0; i < 3; i++)
            if (!found[i] && distance(dx, dy, cx[i], cy[i]) < 150)
                drawClueIcon(i, cx[i], cy[i] + 8, 1.0f);

        /* names of nearby suspects */
        for (int i = 0; i < 3; i++)
            if (distance(dx, dy, sx[i], sy[i]) < 200) {
                glColor3f(1, 0.85f, 0.4f);
                drawTextCenter(sx[i], sy[i] + 135, sName[i], GLUT_BITMAP_HELVETICA_12);
            }

        drawHUD();

        /* press-E hint above detective */
        if (msgTimer <= 0) {
            glColor3f(1, 1, 1);
            for (int i = 0; i < 3; i++)
                if (!found[i] && distance(dx, dy, cx[i], cy[i]) < 50)
                    drawTextCenter(dx, dy + 150, "[E] Pick up clue", GLUT_BITMAP_HELVETICA_12);
            for (int i = 0; i < 3; i++)
                if (distance(dx, dy, sx[i], sy[i]) < 80)
                    drawTextCenter(dx, dy + 150, "[E] Question    [F] Accuse", GLUT_BITMAP_HELVETICA_12);
        }
        drawMsg();
    }

    /* ---------- WIN ---------- */
    if (state == WIN) {
        glColor4f(0, 0, 0.1f, 0.6f);
        rect(0, 450, W, H);
        glColor3f(0.95f, 0.8f, 0.3f);
        drawTextCenter(500, 580, "CASE CLOSED!", GLUT_BITMAP_TIMES_ROMAN_24);
        char b[150];
        sprintf(b, "%s (%s) stole the Blue Diamond!", sName[culprit], sRole[culprit]);
        glColor3f(1, 1, 1);
        drawTextCenter(500, 540, b, GLUT_BITMAP_HELVETICA_18);
        glColor3f(0.9f, 0.6f, 0.6f);
        drawTextCenter(500, 500, "\"The blackout was perfect... how did you find the clues?\"", GLUT_BITMAP_HELVETICA_18);
        glColor3f(1, 1, 1);
        drawTextCenter(500, 20, "Press R to play again   |   ESC to quit", GLUT_BITMAP_HELVETICA_18);
    }

    /* ---------- LOSE ---------- */
    if (state == LOSE) {
        glColor4f(0, 0, 0.05f, 0.75f);
        rect(0, 0, W, H);
        glColor3f(0.9f, 0.15f, 0.2f);
        drawTextCenter(500, 380, "CASE UNSOLVED", GLUT_BITMAP_TIMES_ROMAN_24);
        char b[150];
        sprintf(b, "You ran out of chances. The real thief was %s!", sName[culprit]);
        glColor3f(1, 1, 1);
        drawTextCenter(500, 330, b, GLUT_BITMAP_HELVETICA_18);
        drawTextCenter(500, 280, "Press R to try again   |   ESC to quit", GLUT_BITMAP_HELVETICA_18);
    }

    glutSwapBuffers();
}

/*------------------------------------------------------
   game logic
------------------------------------------------------*/
void initGame() {
    culprit = rand() % 3;
    chances = 3;
    cluesFound = 0;
    msgTimer = 0;

    /* choose 3 different random hiding spots */
    int a = rand() % 6, b = rand() % 6, c = rand() % 6;
    while (b == a) b = rand() % 6;
    while (c == a || c == b) c = rand() % 6;
    int pick[3] = { a, b, c };
    for (int i = 0; i < 3; i++) {
        cx[i] = spots[pick[i]][0];
        cy[i] = spots[pick[i]][1];
        found[i] = false;
    }

    dx = -60;  dy = 60;  faceDir = 1;
}

void startParty()    { initGame(); state = PARTY; frame = 0; }
void startBlackout() { state = BLACKOUT; frame = 0; }
void startInvest() {
    state = INVEST;
    frame = 0;
    dx = 500;  dy = 45;
    showMsg("THE INVESTIGATION BEGINS",
            "The diamond is gone and the lights are dead. Use your flashlight to search the "
            "room, talk to guests for hints and question the suspects.", 600);
}

void pickClueOrTalk() {
    if (state != INVEST) return;

    /* 1) clue nearby? */
    for (int i = 0; i < 3; i++) {
        if (!found[i] && distance(dx, dy, cx[i], cy[i]) < 50) {
            found[i] = true;
            cluesFound++;
            string t;
            if (i == 0) t = string("A hotel keycard engraved with the initials '") + sInit[culprit] + "'.";
            if (i == 1) t = string("Cracked screen. Last message: '") + sFirst[culprit] + ", take it at 9:45. Lights out.'";
            if (i == 2) t = string("Stopped at 9:47. Engraved on the back: '") + sName[culprit] + "'.";
            if (cluesFound == 3) t += "  ALL CLUES FOUND! Go next to the suspect you think did it and press F.";
            showMsg(string(clueName[i]) + " FOUND!", t, 500);
            return;
        }
    }

    /* 2) suspect nearby? */
    for (int i = 0; i < 3; i++) {
        if (distance(dx, dy, sx[i], sy[i]) < 80) {
            string t = sAlibi[i];
            if (i == culprit && cluesFound > 0) t += "  (They avoid your eyes and look at the exit.)";
            showMsg(string(sName[i]) + " - " + sRole[i], t, 480);
            return;
        }
    }

    /* 3) guest nearby? */
    for (int i = 0; i < 3; i++) {
        if (distance(dx, dy, gx[i], gy[i]) < 70) {
            if (cluesFound == 3) {
                showMsg("SCARED GUEST", "You have all the evidence, detective. Go catch the thief!", 400);
                return;
            }
            for (int k = 0; k < 3; k++) {
                if (!found[k]) {
                    string side = "middle";
                    if (cx[k] < 333) side = "left";
                    else if (cx[k] > 666) side = "right";
                    showMsg("SCARED GUEST",
                            "I saw something shiny fall on the floor on the " + side + " side of the room!", 400);
                    return;
                }
            }
        }
    }

    showMsg("Nothing here", "There is nothing to search here. Keep moving.", 120);
}

void accuse() {
    if (state != INVEST) return;

    int who = -1;
    for (int i = 0; i < 3; i++)
        if (distance(dx, dy, sx[i], sy[i]) < 80) who = i;

    if (who == -1) {
        showMsg("ACCUSE", "Stand right next to a suspect and press F.", 200);
        return;
    }
    if (cluesFound < 3) {
        showMsg("NOT ENOUGH EVIDENCE", "You need all 3 clues before you can accuse anyone!", 300);
        return;
    }

    if (who == culprit) {
        state = WIN;
        frame = 0;
        dx = sx[who] - 80;          /* detective stands next to the thief */
        dy = sy[who];
        faceDir = 1;
    } else {
        chances--;
        if (chances <= 0) {
            state = LOSE;
            frame = 0;
        } else {
            char b[100];
            sprintf(b, "%s has a solid alibi! Check the evidence log again. Chances left: %d",
                    sName[who], chances);
            showMsg("WRONG SUSPECT!", b, 400);
        }
    }
}

/*------------------------------------------------------
   timer / input
------------------------------------------------------*/
void update(int value) {
    frame++;
    totalFrame++;
    if (msgTimer > 0) msgTimer--;

    if (state == PARTY) {
        if (dx < 170) dx += 2;          /* detective walks in */
        if (frame > 480) startBlackout();
    }
    if (state == BLACKOUT) {
        if (frame > 300) startInvest();
    }
    if (state == INVEST) {
        if (keys['a'] || specialKeys[GLUT_KEY_LEFT])  { dx -= 4; faceDir = -1; }
        if (keys['d'] || specialKeys[GLUT_KEY_RIGHT]) { dx += 4; faceDir = 1; }
        if (keys['w'] || specialKeys[GLUT_KEY_UP])    dy += 3;
        if (keys['s'] || specialKeys[GLUT_KEY_DOWN])  dy -= 3;

        if (dx < 30) dx = 30;
        if (dx > 970) dx = 970;
        if (dy < 15) dy = 15;
        if (dy > 290) dy = 290;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyDown(unsigned char k, int x, int y) {
    if (k >= 'A' && k <= 'Z') k = k + 32;       /* to lowercase */
    keys[k] = true;

    if (k == 27) exit(0);
    if (k == 13) {
        if (state == TITLE) startParty();
        else if (state == PARTY) startBlackout();
        else if (state == BLACKOUT) startInvest();
    }
    if (k == 'r' && state != TITLE) startParty();
    if (k == 'e') pickClueOrTalk();
    if (k == 'f') accuse();
}

void keyUp(unsigned char k, int x, int y) {
    if (k >= 'A' && k <= 'Z') k = k + 32;
    keys[k] = false;
}

void specialDown(int k, int x, int y) { if (k < 256) specialKeys[k] = true; }
void specialUp(int k, int x, int y)   { if (k < 256) specialKeys[k] = false; }

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    pointScale = (float)w / W;            /* keep points big enough when window grows */
    if (pointScale < 1) pointScale = 1;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, W, 0, H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int main(int argc, char **argv) {
    srand((unsigned)time(0));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(W, H);
    glutInitWindowPosition(100, 40);
    glutCreateWindow("The Hidden Truth - Detective Mystery Game");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0, 0, 0, 1);

    for (int i = 0; i < 256; i++) { keys[i] = false; specialKeys[i] = false; }
    initGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialDown);
    glutSpecialUpFunc(specialUp);
    glutIgnoreKeyRepeat(1);
    glutTimerFunc(16, update, 0);

    glutMainLoop();
    return 0;
}
