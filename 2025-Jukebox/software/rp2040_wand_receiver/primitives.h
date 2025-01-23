#ifndef _PRIMITIVES_
#define _PRIMITIVES_

#include <Arduino.h>

typedef struct {
  uint16_t x, y;
  uint8_t r, g, b;
} laser_point_t;

typedef struct {
  uint8_t r, g, b;
} rgb_t;

typedef struct {
  uint16_t x, y;
  uint8_t on;
} xy_t;

void add_vv(int n, double *a, double *b, double *result);
void sub_vv(int n, double *a, double *b, double *result);
void add_vf(int n, double *a, double b, double *result);
void sub_vf(int n, double *a, double b, double *result);
void mul_vf(int n, double *a, double b, double *result);
void div_vf(int n, double *a, double b, double *result);
void cross(double a[3], double b[3], double result[3]);
double dot_vv(int n, double *a, double *b);
double mag(int n, double *a);
void norm(int n, double *a, double *result);
void find_edge_pos(double p1[3], double p2[3], double z, double result[3]);
void find_surface_normal(double surface[4][3], double result[3]);
int same_side(double p1[3], double p2[3], double a[3], double b[3]);
int point_in_triangle(double a[3], double b[3], double c[3], double p[3]);
int point_in_surface(double p1[3], double p2[3], double p3[3], double p4[3], double p[3]);
void rotate(double q[4], double v[3], double result[3]);
void dot_mv(double mat[4][4], double v[4], double result[4]);
void dot_mv33(double mat[3][3], double v[3], double result[3]);
void mul_mm(int n1, int m1, int n2, int m2, double *mat1, double *mat2, double *result);
void eigen(double a[4][4], double eigenvalues[4], double eigenvectors[4][4]);
void lstsq(double a[3][4], double b[3][4], double x[4][4]);

rgb_t hsv_to_rgb(double h, double s, double v);
int get_interpolated_size(xy_t *obj, int obj_len, uint16_t seg_dist);
void interpolate_objects(xy_t *obj, int obj_len, uint16_t seg_dist, xy_t *result);
void get_laser_obj_bounds(xy_t *obj, int obj_len, uint16_t *min_x, uint16_t *max_x, uint16_t *min_y, uint16_t *max_y);
void get_laser_obj_size(xy_t *obj, int obj_len, uint16_t *width, uint16_t *height);
void get_laser_obj_midpoint(xy_t *obj, int obj_len, uint16_t *mid_x, uint16_t *mid_y);
void convert_to_xy(uint16_t *obj, int obj_len, double x_scale, double y_scale, xy_t *result);

#endif