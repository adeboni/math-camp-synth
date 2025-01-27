#include "sierpinski.h"

void Sierpinski::init() {
  triangle_height = sqrt(SIDE_LENGTH * SIDE_LENGTH - (SIDE_LENGTH / 2) * (SIDE_LENGTH / 2));
  tetra_height = SIDE_LENGTH * sqrt(2.0 / 3.0);
  projection_bottom = tetra_height / 4;
  projection_top = tetra_height / 2;

  vertices[0][0] = -SIDE_LENGTH / 2;
  vertices[0][1] = -triangle_height / 3;
  vertices[0][2] = 0;

  vertices[1][0] = SIDE_LENGTH / 2;
  vertices[1][1] = -triangle_height / 3;
  vertices[1][2] = 0;

  vertices[2][0] = 0;
  vertices[2][1] = triangle_height * 2 / 3;
  vertices[2][2] = 0;

  vertices[3][0] = 0;
  vertices[3][1] = 0;
  vertices[3][2] = tetra_height;

  double edges[6][2][3] = {
    {{vertices[0][0], vertices[0][1], vertices[0][2]}, {vertices[1][0], vertices[1][1], vertices[1][2]}},
    {{vertices[0][0], vertices[0][1], vertices[0][2]}, {vertices[2][0], vertices[2][1], vertices[2][2]}},
    {{vertices[1][0], vertices[1][1], vertices[1][2]}, {vertices[2][0], vertices[2][1], vertices[2][2]}},
    {{vertices[1][0], vertices[1][1], vertices[1][2]}, {vertices[3][0], vertices[3][1], vertices[3][2]}},
    {{vertices[0][0], vertices[0][1], vertices[0][2]}, {vertices[3][0], vertices[3][1], vertices[3][2]}},
    {{vertices[2][0], vertices[2][1], vertices[2][2]}, {vertices[3][0], vertices[3][1], vertices[3][2]}}
  };

  find_edge_pos(edges[4][0], edges[4][1], projection_bottom, surfaces[0][0]);
  find_edge_pos(edges[4][0], edges[4][1], projection_top, surfaces[0][1]);
  find_edge_pos(edges[5][0], edges[5][1], projection_top, surfaces[0][2]);
  find_edge_pos(edges[5][0], edges[5][1], projection_bottom, surfaces[0][3]);

  find_edge_pos(edges[3][0], edges[3][1], projection_bottom, surfaces[1][0]);
  find_edge_pos(edges[3][0], edges[3][1], projection_top, surfaces[1][1]);
  find_edge_pos(edges[5][0], edges[5][1], projection_top, surfaces[1][2]);
  find_edge_pos(edges[5][0], edges[5][1], projection_bottom, surfaces[1][3]);

  find_edge_pos(edges[3][0], edges[3][1], projection_bottom, surfaces[2][0]);
  find_edge_pos(edges[3][0], edges[3][1], projection_top, surfaces[2][1]);
  find_edge_pos(edges[4][0], edges[4][1], projection_top, surfaces[2][2]);
  find_edge_pos(edges[4][0], edges[4][1], projection_bottom, surfaces[2][3]);

  for (int i = 0; i < 3; i++)
    find_surface_normal(surfaces[i], plane_normals[i]);

  double lasers[3][3];
  find_edge_pos(edges[3][0], edges[3][1], projection_bottom, lasers[0]);
  find_edge_pos(edges[4][0], edges[4][1], projection_bottom, lasers[1]);
  find_edge_pos(edges[5][0], edges[5][1], projection_bottom, lasers[2]);

  for (int i = 0; i < 3; i++) {
    double laser_center[3];
    sub_vv(3, lasers[i], surfaces[i][0], laser_center);
    mul_vf(3, plane_normals[i], dot_vv(3, laser_center, plane_normals[i]), laser_center);
    sub_vv(3, lasers[i], laser_center, laser_center);
    double temp[3];
    sub_vv(3, laser_center, lasers[i], temp);
    double laser_distance = mag(3, temp);
    double half_width = laser_distance * tan(LASER_PROJECTION_RANGE_DEG * 3.14159265358979323846 / 180.0);
    double v1[3] = {-plane_normals[i][1], plane_normals[i][0], 0};
    norm(3, v1, v1);
    double v2[3];
    cross(plane_normals[i], v1, v2);
    double a[3][4] = {
      {0, 2048, 0, 1},
      {2048, 4095, 0, 1},
      {2048, 2048, 0, 1}
    };
    double b[3][4] = {
      {laser_center[0] + half_width * v1[0], laser_center[1] + half_width * v1[1], laser_center[2] + half_width * v1[2], 1},
      {laser_center[0] + half_width * v2[0], laser_center[1] + half_width * v2[1], laser_center[2] + half_width * v2[2], 1},
      {laser_center[0], laser_center[1], laser_center[2], 1}
    };
     
    double x[4][4];
    lstsq(a, b, x);
    for (int ii = 0; ii < 4; ii++)
      for (int jj = 0; jj < 4; jj++)
      trans_matrix[i][jj][ii] = x[ii][jj];
    
    lstsq(b, a, x);
    for (int ii = 0; ii < 4; ii++)
      for (int jj = 0; jj < 4; jj++)
      inv_trans_matrix[i][jj][ii] = x[ii][jj];
  }

  double q[4] = {0, 0, 0, 1};
  calibrate_wand_position(q);
}

void Sierpinski::calibrate_wand_position(double q[4]) {
  double center_line_p1[3];
  add_vv(3, vertices[0], vertices[2], center_line_p1);
  div_vf(3, center_line_p1, 2, center_line_p1);
  double center_line_p2[3] = {0, 0, tetra_height};

  double center_point[3];
  find_edge_pos(center_line_p1, center_line_p2, projection_bottom  + (projection_top - projection_bottom) / 2, center_point);

  double target_vector[3] = {center_point[0], center_point[1], center_point[2] - WAND_HEIGHT};
  norm(3, target_vector, target_vector);

  double wand_pos[3];
  rotate(q, wand_vector, wand_pos);

  double cross_result[3];
  cross(wand_pos, target_vector, cross_result);

  double target_yaw = atan2(cross_result[2], dot_vv(3, wand_pos, target_vector));
  double target_pitch = asin(wand_pos[2]) - asin(target_vector[2]);
  yaw_matrix[0][0] = cos(target_yaw);
  yaw_matrix[0][1] = -sin(target_yaw);
  yaw_matrix[1][0] = sin(target_yaw);
  yaw_matrix[1][1] = cos(target_yaw);
  pitch_matrix[0][0] = cos(target_pitch);
  pitch_matrix[0][2] = sin(target_pitch);
  pitch_matrix[2][0] = -sin(target_pitch);
  pitch_matrix[2][2] = cos(target_pitch);
}

void Sierpinski::laser_to_sierpinski_coords(int laser_index, int x, int y, double result[3]) {
  double v[4] = {(double)x, (double)y, 0, 1};
  double dot_result[4];
  dot_mv(4, &trans_matrix[laser_index][0][0], v, dot_result);
  result[0] = dot_result[0];
  result[1] = dot_result[1];
  result[2] = dot_result[2];
}

xy_t Sierpinski::sierpinski_to_laser_coords(int laser_index, double v[3]) {
  double v2[4] = {v[0], v[1], v[2], 1};
  double dot_result[4];
  dot_mv(4, &inv_trans_matrix[laser_index][0][0], v2, dot_result);
  return (xy_t){(uint16_t)dot_result[0], (uint16_t)dot_result[1], 1};
}

void Sierpinski::get_laser_coordinate_bounds(xy_t result[4]) {
  for (int i = 0; i < 4; i++)
    result[i] = sierpinski_to_laser_coords(0, surfaces[0][i]);
}

void Sierpinski::get_laser_rect_interior(uint16_t result[4]) {
  xy_t bottom1 = sierpinski_to_laser_coords(0, surfaces[0][0]);
  xy_t top1 = sierpinski_to_laser_coords(0, surfaces[0][1]);
  xy_t top2 = sierpinski_to_laser_coords(0, surfaces[0][2]);
  xy_t bottom2 = sierpinski_to_laser_coords(0, surfaces[0][3]);
  result[0] = min(top1.x, top2.x);
  result[1] = max(top1.x, top2.x);
  result[2] = bottom1.y;
  result[3] = top1.y;
}

void Sierpinski::apply_quaternion(double q[4], double result[3]) {
  double qv[3];
  rotate(q, wand_vector, qv);
  double v1[3] = {qv[0], qv[1], 0};
  double dot_result1[3];
  dot_mv(3, &yaw_matrix[0][0], v1, dot_result1);
  double v2[3] = {sqrt(1 - qv[2] * qv[2]), 0, qv[2]};
  double dot_result2[3];
  dot_mv(3, &pitch_matrix[0][0], v2, dot_result2);
  double v3[3] = {v1[0], v1[1], v2[2]};
  norm(3, v3, result);
}

void Sierpinski::get_wand_projection(double q[4], int *laser_index, double result[3]) {
  double temp1[3];
  double temp2[3];
  double start[3] = {0, 0, WAND_HEIGHT};
  double end[3];
  apply_quaternion(q, end);
  end[2] += WAND_HEIGHT;
  double v[3];
  sub_vv(3, end, start, v);
  if (v[2] < 0) {
    *laser_index = -1;
    return;
  }

  for (int i = 0; i < 3; i++) {
    double denom = dot_vv(3, v, plane_normals[i]);
    if (denom < 0.01) continue;
    
    div_vf(3, plane_normals[i], denom, temp1);
    sub_vv(3, surfaces[i][0], end, temp2);
    mul_vf(3, v, dot_vv(3, temp1, temp2), v);
    add_vv(3, v, end, result);
    if (point_in_surface(surfaces[i][0], surfaces[i][1], surfaces[i][2], surfaces[i][3], result)) {
      *laser_index = i;
      return;
    }
  }
}
