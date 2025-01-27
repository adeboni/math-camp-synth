#include "spirograph.h"

void Spirograph::init(double r1, double r2, double a, double t_d) {
  _r1 = r1;
  _r2 = r2;
  _a = a;
  _t_d = t_d;
  _t = 0;
  x = 0;
  y = 0;
}

void Spirograph::add_delta(spiro_delta_t d) {
  if (numDeltas < MAX_DELTAS)
    deltas[numDeltas++] = d;
}

void Spirograph::update(double xs, double ys, double xo, double yo) {
  _t += _t_d;
  for (int i = 0; i < numDeltas; i++) {
    switch (deltas[i].type) {
      case DELTA_TYPE_R1:
        _r1 += deltas[i].d;
        if (_r1 < deltas[i].ll || _r1 > deltas[i].ul)
          deltas[i].d *= -1;
        break;
      case DELTA_TYPE_R2:
        _r2 += deltas[i].d;
        if (_r2 < deltas[i].ll || _r2 > deltas[i].ul)
          deltas[i].d *= -1;
        break;
      case DELTA_TYPE_A:
        _a += deltas[i].d;
        if (_a < deltas[i].ll || _a > deltas[i].ul)
          deltas[i].d *= -1;
        break;
    }
  }

  double q1 = _t;
  double s1 = sin(q1);
  double c1 = cos(q1);
  double q2 = q1 * _r1 / _r2;
  double s2 = sin(q2);
  double c2 = cos(q2);
  x = _r1 * s1 + _a * _r2 * (-s1 + c2 * s1 - c1 * s2);
  y = -_r1 * c1 + _a * _r2 * (c1 - c1 * c2 - s1 * s2);
  x = x * xs + xo;
  y = y * ys + yo;
}