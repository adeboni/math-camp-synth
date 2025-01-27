#ifndef _SPIROGRAPH_
#define _SPIROGRAPH_

#include <Arduino.h>

#define DELTA_TYPE_R1 0
#define DELTA_TYPE_R2 1
#define DELTA_TYPE_A  2
#define MAX_DELTAS    10

typedef struct {
  uint8_t type;
  double d;
  double ll;
  double ul;
} spiro_delta_t;

class Spirograph {
  public:
    void init(double r1, double r2, double a, double t_d);
    void add_delta(spiro_delta_t d);
    void update(double xs, double ys, double xo, double yo);
    double x = 0;
    double y = 0;

  private:
    double _r1 = 1;
    double _r2 = 1;
    double _a = 1;
    double _t_d = 1;
    double _t = 0;

    spiro_delta_t deltas[MAX_DELTAS];
    int numDeltas = 0;
};

#endif