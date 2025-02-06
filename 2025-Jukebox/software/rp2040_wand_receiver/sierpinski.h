#ifndef _SIERPINSKI_
#define _SIERPINSKI_

#include <Arduino.h>
#include "primitives.h"

#define WAND_HEIGHT 5.0
#define SIDE_LENGTH 39.0
#define LASER_PROJECTION_RANGE_DEG 55.0

class Sierpinski {
  public:
    void init();
    void calibrate_wand_position(double q[4]);
    void laser_to_sierpinski_coords(int laser_index, int x, int y, double result[3]);
    xy_t sierpinski_to_laser_coords(int laser_index, double v[3]);
    void get_laser_coordinate_bounds(xy_t result[4]);
    void get_laser_rect_interior(uint16_t result[4]);
    void apply_quaternion(double q[4], double result[3]);
    void get_wand_projection(double q[4], int *laser_index, double result[3]);
    rgb_t get_wand_rotation_color(double q[4], int degree_offset);
    rgb_t get_color_from_angle(int angle);

  private:
    double triangle_height;
    double tetra_height;
    double projection_bottom;
    double projection_top;

    double vertices[4][3];
    double surfaces[3][4][3];
    double plane_normals[3][3];
    double wand_vector[3] = {0, -1, 0};
    double trans_matrix[3][4][4];
    double inv_trans_matrix[3][4][4];

    double yaw_diff = 0.0;
    double pitch_diff = 0.0;
};

#endif
