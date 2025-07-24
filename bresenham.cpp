#include <iostream>
using namespace std;

void plot(int x, int y) {
    cout << "(" << x << ", " << y << ")\n";
}

void bresenham(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0;
    int dy = y1 - y0;

    int sx, sy;
    if (dx >= 0) {
        sx = 1;
    } else {
        sx = -1;
    }

    if (dy >= 0) {
        sy = 1;
    } else {
        sy = -1;
    }

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    int x = x0;
    int y = y0;

    if (dx > dy) {
        int p = 2 * dy - dx;
        for (int i = 0; i <= dx; i++) {
            plot(x, y);
            x = x + sx;
            if (p >= 0) {
                y = y + sy;
                p = p - 2 * dx;
            }
            p = p + 2 * dy;
        }
    } else {
        int p = 2 * dx - dy;
        for (int i = 0; i <= dy; i++) {
            plot(x, y);
            y = y + sy;
            if (p >= 0) {
                x = x + sx;
                p = p - 2 * dy;
            }
            p = p + 2 * dx;
        }
    }
}

int main() {
    int x0, y0, x1, y1;

    cout << "Enter x0 y0: ";
    cin >> x0 >> y0;

    cout << "Enter x1 y1: ";
    cin >> x1 >> y1;

    cout << "Points on the line:\n";
    bresenham(x0, y0, x1, y1);

    return 0;
}
