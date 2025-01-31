#include "primitives.h"

void add_vv(int n, double *a, double *b, double *result) {
  for (int i = 0; i < n; i++)
    result[i] = a[i] + b[i];
}

void sub_vv(int n, double *a, double *b, double *result) {
  for (int i = 0; i < n; i++)
    result[i] = a[i] - b[i];
}

void mul_vf(int n, double *a, double b, double *result) {
  for (int i = 0; i < n; i++)
    result[i] = a[i] * b;
}

void div_vf(int n, double *a, double b, double *result) {
  for (int i = 0; i < n; i++)
    result[i] = a[i] / b;
}

void cross(double a[3], double b[3], double result[3]) {
  result[0] = a[1] * b[2] - a[2] * b[1];
  result[1] = a[2] * b[0] - a[0] * b[2];
  result[2] = a[0] * b[1] - a[1] * b[0];
}

double dot_vv(int n, double *a, double *b) {
  double result = 0;
  for (int i = 0; i < n; i++) 
    result += a[i] * b[i];
  return result;
}

double mag(int n, double *a) {
  double result = 0;
  for (int i = 0; i < n; i++)
    result += a[i] * a[i];
  return sqrt(result);
}

void norm(int n, double *a, double *result) {
  div_vf(n, a, mag(n, a), result);
}

void find_edge_pos(double p1[3], double p2[3], double z, double result[3]) {
  result[0] = p1[0] + (z - p1[2]) * (p2[0] - p1[0]) / (p2[2] - p1[2]);
  result[1] = p1[1] + (z - p1[2]) * (p2[1] - p1[1]) / (p2[2] - p1[2]);
  result[2] = z;
}

void find_surface_normal(double surface[4][3], double result[3]) {
  double diff1[3];
  double diff2[3];
  double pn[3];
  sub_vv(3, surface[1], surface[0], diff1);
  sub_vv(3, surface[2], surface[0], diff2);
  cross(diff1, diff2, pn);
  if (pn[2] < 0)
    mul_vf(3, pn, -1, pn);
  norm(3, pn, result);
}

int same_side(double p1[3], double p2[3], double a[3], double b[3]) {
  double diff1[3];
  double diff2[3];
  double cross1[3];
  double cross2[3];
  sub_vv(3, b, a, diff1);
  sub_vv(3, p1, a, diff2);
  cross(diff1, diff2, cross1);
  sub_vv(3, p2, a, diff2);
  cross(diff1, diff2, cross2);
  return dot_vv(3, cross1, cross2) >= 0;
}

int point_in_triangle(double a[3], double b[3], double c[3], double p[3]) {
  return same_side(p, a, b, c) && same_side(p, b, a, c) && same_side(p, c, a, b);
}

int point_in_surface(double p1[3], double p2[3], double p3[3], double p4[3], double p[3]) {
  return point_in_triangle(p1, p2, p3, p) || point_in_triangle(p3, p4, p1, p);
}

void rotate(double q[4], double v[3], double result[3]) {
  double conj[4] = { -q[0], -q[1], -q[2], q[3] };
  double qv[4] = {
    q[3] * v[0] + q[1] * v[2] - q[2] * v[1],
    q[3] * v[1] + q[2] * v[0] - q[0] * v[2],
    q[3] * v[2] + q[0] * v[1] - q[1] * v[0],
    -q[0] * v[0] - q[1] * v[1] - q[2] * v[2]
  };
  result[0] = qv[3] * conj[0] + qv[0] * conj[3] + qv[1] * conj[2] - qv[2] * conj[1];
  result[1] = qv[3] * conj[1] + qv[1] * conj[3] + qv[2] * conj[0] - qv[0] * conj[2];
  result[2] = qv[3] * conj[2] + qv[2] * conj[3] + qv[0] * conj[1] - qv[1] * conj[0];
}

void dot_mv(int n, double *mat, double *v, double *result) {
  for (int i = 0; i < n; i++) {
    result[i] = 0;
    for (int j = 0; j < n; j++)
      result[i] += mat[i * n + j] * v[j];
  }
}

void mul_mm(int n1, int m1, int n2, int m2, double *mat1, double *mat2, double *result) {
  for (int i = 0; i < n1; i++) 
    for (int j = 0; j < m2; j++) 
      result[i * m2 + j] = 0;

  for (int i = 0; i < n1; i++) 
    for (int j = 0; j < m2; j++) 
      for (int k = 0; k < m1; k++)
        result[i * m2 + j] += mat1[i * m1 + k] * mat2[k * m2 + j];
}

void eigen(double a[4][4], double eigenvalues[4], double eigenvectors[4][4]) {
  double aa[4][4];
  double temp_vector[4];
  double eigenvector[4];
  double temp_eigenvectors[4][4];

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      aa[i][j] = a[i][j];

  for (int k = 0; k < 4; k++) {
    for (int i = 0; i < 4; i++)
      eigenvector[i] = ((double)i + 1) / 10.0;

    for (int iter = 0; iter < 1000; iter++) {
      dot_mv(4, &aa[0][0], eigenvector, temp_vector);
      norm(4, temp_vector, eigenvector);
    }

    for (int i = 0; i < 4; i++)
      temp_eigenvectors[k][i] = eigenvector[i];

    dot_mv(4, &aa[0][0], eigenvector, temp_vector);
    double eigenvalue = fmax(dot_vv(4, eigenvector, temp_vector), 0.001);
    eigenvalues[k] = eigenvalue;

    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        aa[i][j] -= eigenvector[i] * eigenvector[j] * eigenvalue;
  }

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) 
      eigenvectors[j][i] = temp_eigenvectors[i][j];
}

void lstsq(double a[3][4], double b[3][4], double x[4][4]) {
  double at[4][3];

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 4; j++) 
      at[j][i] = a[i][j];

  double ata[4][4];
  mul_mm(4, 3, 3, 4, &at[0][0], &a[0][0], &ata[0][0]);
  double eigenvalues[4];
  double eigenvectors[4][4];
  eigen(ata, eigenvalues, eigenvectors);
  for (int i = 0; i < 4; i++)
    eigenvalues[i] = sqrt(eigenvalues[i]);

  double U[3][4];
  double temp_U[3][3];
  double Ut[3][3];
  mul_mm(3, 4, 4, 4, &a[0][0], &eigenvectors[0][0], &U[0][0]);

  for (int i = 0; i < 3; i++) 
    for (int j = 0; j < 4; j++) 
      U[i][j] /= eigenvalues[j];

  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) 
      Ut[j][i] = U[i][j];
  
  double Si[4][3] = {
    {1 / eigenvalues[0], 0, 0},
    {0, 1 / eigenvalues[1], 0},
    {0, 0, 1 / eigenvalues[2]},
    {0, 0, 0}
  };

  double temp1[4][3];
  double temp2[4][3];
  mul_mm(4, 4, 4, 3, &eigenvectors[0][0], &Si[0][0], &temp1[0][0]);
  mul_mm(4, 3, 3, 3, &temp1[0][0], &Ut[0][0], &temp2[0][0]);
  mul_mm(4, 3, 3, 4, &temp2[0][0], &b[0][0], &x[0][0]);
}


rgb_t hsv_to_rgb(double h, double s, double v) {
  if (s < 0.01) 
    return (rgb_t){(uint8_t)(v * 255), (uint8_t)(v * 255), (uint8_t)(v * 255)};
  
  int i = (int)(h * 6.0);
  double f = (h * 6.0) - i;
  double p = v * (1.0 - s);
  double q = v * (1.0 - s * f);
  double t = v * (1.0 - s * (1.0 - f));
  rgb_t result = {0, 0, 0};
  switch (i % 6) 
  {
    case 0:
      result.r = (uint8_t)(v * 255);
      result.g = (uint8_t)(t * 255);
      result.b = (uint8_t)(p * 255);
      break;
    case 1:
      result.r = (uint8_t)(q * 255);
      result.g = (uint8_t)(v * 255);
      result.b = (uint8_t)(p * 255);
      break;
    case 2:
      result.r = (uint8_t)(p * 255);
      result.g = (uint8_t)(v * 255);
      result.b = (uint8_t)(t * 255);
      break;
    case 3:
      result.r = (uint8_t)(p * 255);
      result.g = (uint8_t)(q * 255);
      result.b = (uint8_t)(v * 255);
      break;
    case 4:
      result.r = (uint8_t)(t * 255);
      result.g = (uint8_t)(p * 255);
      result.b = (uint8_t)(v * 255);
      break;
    case 5:
      result.r = (uint8_t)(v * 255);
      result.g = (uint8_t)(p * 255);
      result.b = (uint8_t)(q * 255);
      break;
  }

  return result;
}

int get_interpolated_size(xy_t *obj, int obj_len, uint16_t seg_dist) {
  int result = obj_len;
  for (int i = 1; i < obj_len; i++) {
    int x_seg = abs(obj[i-1].x - obj[i].x) / seg_dist;
    int y_seg = abs(obj[i-1].y - obj[i].y) / seg_dist;
    if (x_seg > y_seg) result += x_seg;
    else result += y_seg;
  }
  return result;
}

void interpolate_objects(xy_t *obj, int obj_len, uint16_t seg_dist, xy_t *result) {
  int k = 0;
  result[k++] = obj[0];
  for (int i = 1; i < obj_len; i++) {
    int num_segs = 2;
    int x_seg = abs(obj[i-1].x - obj[i].x) / seg_dist;
    int y_seg = abs(obj[i-1].y - obj[i].y) / seg_dist;
    if (x_seg > y_seg) num_segs += x_seg;
    else num_segs += y_seg;

    double x_inc = (double)(obj[i].x - obj[i-1].x) / num_segs;
    double y_inc = (double)(obj[i].y - obj[i-1].y) / num_segs;
    for (int j = 1; j < num_segs; j++)
      result[k++] = (xy_t){(uint16_t)(obj[i-1].x + j * x_inc), (uint16_t)(obj[i-1].y + j * y_inc), obj[i].on};
  }
}

void get_laser_obj_bounds(xy_t *obj, int obj_len, uint16_t *min_x, uint16_t *max_x, uint16_t *min_y, uint16_t *max_y) {
  *min_x = 65535;
  *min_y = 65535;
  *max_x = 0;
  *max_y = 0;
  for (int i = 0; i < obj_len; i++) {
    if (obj[i].x < *min_x) *min_x = obj[i].x;
    if (obj[i].x > *max_x) *max_x = obj[i].x;
    if (obj[i].y < *min_y) *min_y = obj[i].y;
    if (obj[i].y > *max_y) *max_y = obj[i].y;
  }
}

void get_laser_obj_size(xy_t *obj, int obj_len, uint16_t *width, uint16_t *height) {
  uint16_t min_x, max_x, min_y, max_y;
  get_laser_obj_bounds(obj, obj_len, &min_x, &max_x, &min_y, &max_y);
  *width = max_x - min_x;
  *height = max_y - min_y;
}

void get_laser_obj_midpoint(xy_t *obj, int obj_len, uint16_t *mid_x, uint16_t *mid_y) {
  uint16_t min_x, max_x, min_y, max_y;
  get_laser_obj_bounds(obj, obj_len, &min_x, &max_x, &min_y, &max_y);
  *mid_x = (min_x + max_x) / 2;
  *mid_y = (min_y + max_y) / 2;
}

int convert_to_xy(uint16_t *obj, int obj_len, double x_scale, double y_scale, xy_t *result) {
  int j = 0;
  for (int i = 0; i < obj_len; i+=2)
    result[j++] = (xy_t){(uint16_t)((obj[i] & 0x7fff) * x_scale), (uint16_t)((obj[i+1]) * y_scale), (obj[i] & 0x8000) > 0 ? 1 : 0};
  result[j++] = result[0];
  uint16_t mid_x, mid_y;
  get_laser_obj_midpoint(result, j, &mid_x, &mid_y);
  for (int i = 0; i < j; i++) {
    result[i].x -= mid_x;
    result[i].y -= mid_y;
  }
  return j;
}
