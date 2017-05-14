
#include "fractionApprox.h"

#include <cmath>

// Adaptation of algorithm by David Eppstein
void fractionApprox(int &num, int &den, float x, int maxDen) {
    float startX = x;
    int m[2][2] = { 1, 0, 0, 1 };
    int ai = int(x);

    while (m[1][0]*ai+m[1][1] <= maxDen) {
        int t = m[0][0]*ai+m[0][1];
        m[0][1] = m[0][0];
        m[0][0] = t;
        t = m[1][0]*ai+m[1][1];
        m[1][1] = m[1][0];
        m[1][0] = t;
        if (x == float(ai))
            break;
        x = 1.f/(x-float(ai));
        if(x > float(0x7fffffff))
            break;
        ai = int(x);
    } 

    num = m[0][0];
    den = m[1][0];
    ai = (maxDen-m[1][1])/m[1][0];
    m[0][0] = m[0][0]*ai+m[0][1];
    m[1][0] = m[1][0]*ai+m[1][1];
    if (fabsf(float(m[0][0])/float(m[1][0])-startX) < fabsf(float(num)/float(den)-startX)) {
        num = m[0][0];
        den = m[1][0];
    }
}
