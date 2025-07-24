#include <iostream>
using namespace std;

void plot(int x, int y) {
    cout << "(" << x << ", " << y << ")\n";  
}

void bresenham(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int p = 2 * dy - dx;  

    int x = x0;
    int y = y0;

    plot(x, y);

    while (x < x1) {
        x++;
        if (p < 0) {
            p = p + 2 * dy;
        } else {
            y++;
            p = p + 2 * dy - 2 * dx;
        }
        plot(x, y);
    }
}

int main() {
    int x0, y0, x1, y1;

    cout << "Enter x0 y0: ";
    cin >> x0 >> y0;

    cout << "Enter x1 y1: ";
    cin >> x1 >> y1;

    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }

    cout << "Points on the line:\n";
    bresenham(x0, y0, x1, y1);

    return 0;
}
