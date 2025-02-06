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
  find_edge_pos(edges[4][0], edges[4][1], projection_top,    surfaces[0][1]);
  find_edge_pos(edges[5][0], edges[5][1], projection_top,    surfaces[0][2]);
  find_edge_pos(edges[5][0], edges[5][1], projection_bottom, surfaces[0][3]);

  find_edge_pos(edges[3][0], edges[3][1], projection_bottom, surfaces[1][0]);
  find_edge_pos(edges[3][0], edges[3][1], projection_top,    surfaces[1][1]);
  find_edge_pos(edges[5][0], edges[5][1], projection_top,    surfaces[1][2]);
  find_edge_pos(edges[5][0], edges[5][1], projection_bottom, surfaces[1][3]);

  find_edge_pos(edges[3][0], edges[3][1], projection_bottom, surfaces[2][0]);
  find_edge_pos(edges[3][0], edges[3][1], projection_top,    surfaces[2][1]);
  find_edge_pos(edges[4][0], edges[4][1], projection_top,    surfaces[2][2]);
  find_edge_pos(edges[4][0], edges[4][1], projection_bottom, surfaces[2][3]);

  for (int i = 0; i < 3; i++)
    find_surface_normal(surfaces[i], plane_normals[i]);

  double lasers[3][3];
  find_edge_pos(edges[3][0], edges[3][1], projection_bottom, lasers[0]);
  find_edge_pos(edges[4][0], edges[4][1], projection_bottom, lasers[1]);
  find_edge_pos(edges[5][0], edges[5][1], projection_bottom, lasers[2]);

  for (int i = 0; i < 3; i++) {
    double dot = 0.0;
    for (int j = 0; j < 3; j++)
      dot += (lasers[i][j] - surfaces[i][0][j]) * plane_normals[i][j];
    
    double laser_center[3] = {
      lasers[i][0] - plane_normals[i][0] * dot,
      lasers[i][1] - plane_normals[i][1] * dot,
      lasers[i][2] - plane_normals[i][2] * dot
    };

    double half_width = 0.0;
    for (int j = 0; j < 3; j++) 
      half_width += (laser_center[j] - lasers[i][j]) * (laser_center[j] - lasers[i][j]);
    half_width = sqrt(half_width) * tan(LASER_PROJECTION_RANGE_DEG * PI / 180.0);

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

/*
void Sierpinski::calibrate_wand_position(double q[4]) {
  double center_line_p1[3] = {
    (vertices[0][0] + vertices[2][0]) / 2.0,
    (vertices[0][1] + vertices[2][1]) / 2.0,
    (vertices[0][2] + vertices[2][2]) / 2.0
  };
  double center_line_p2[3] = {0, 0, tetra_height};

  double center_point[3];
  find_edge_pos(center_line_p1, center_line_p2, (projection_top + projection_bottom) / 2, center_point);

  double target_vector[3] = {center_point[0], center_point[1], center_point[2] - WAND_HEIGHT};
  norm(3, target_vector, target_vector);

  double wand_pos[3];
  rotate(q, wand_vector, wand_pos);

  double cross_result[3];
  cross(wand_pos, target_vector, cross_result);

  double dot = 0.0;
  for (int i = 0; i < 3; i++)
    dot += wand_pos[i] * target_vector[i];

  double target_yaw = atan2(cross_result[2], dot);
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
*/

void Sierpinski::calibrate_wand_position(double q[4]) {
  double center_line_p1[3] = {
    (vertices[0][0] + vertices[2][0]) / 2.0,
    (vertices[0][1] + vertices[2][1]) / 2.0,
    (vertices[0][2] + vertices[2][2]) / 2.0
  };
  double center_line_p2[3] = {0, 0, tetra_height};

  double center_point[3];
  find_edge_pos(center_line_p1, center_line_p2, (projection_top + projection_bottom) / 2, center_point);

  double target_vector[3] = {center_point[0], center_point[1], center_point[2] - WAND_HEIGHT};
  norm(3, target_vector, target_vector);

  double wand_pos[3];
  rotate(q, wand_vector, wand_pos);

  pitch_diff = asin(target_vector[2]) - asin(wand_pos[2]);
  yaw_diff = atan2(target_vector[1], target_vector[0]) - atan2(wand_pos[1], wand_pos[0]);
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
  return (xy_t){(int)dot_result[0], (int)dot_result[1], true};
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
  result[0] = (uint16_t)min(top1.x, top2.x);
  result[1] = (uint16_t)max(top1.x, top2.x);
  result[2] = (uint16_t)bottom1.y;
  result[3] = (uint16_t)top1.y;
}
/*
void Sierpinski::apply_quaternion(double q[4], double result[3]) {
  double qv[3];
  rotate(q, wand_vector, qv);
  double _v1[3] = {qv[0], qv[1], 0};
  double v1[3];
  dot_mv(3, &yaw_matrix[0][0], _v1, v1);
  double _v2[3] = {sqrt(1.0 - qv[2] * qv[2]), 0, qv[2]};
  double v2[3];
  dot_mv(3, &pitch_matrix[0][0], _v2, v2);
  double v3[3] = {v1[0], v1[1], v2[2]};
  norm(3, v3, result);
}
*/

void Sierpinski::apply_quaternion(double q[4], double result[3]) {
  double qv[3];
  rotate(q, wand_vector, qv);
  double pitch = asin(qv[2]) + pitch_diff;
  double yaw = atan2(qv[1], qv[0]) + yaw_diff;
  result[0] = cos(pitch) * cos(yaw);
  result[1] = cos(pitch) * sin(yaw);
  result[2] = sin(pitch);
}

void Sierpinski::get_wand_projection(double q[4], int *laser_index, double result[3]) {
  *laser_index = -1;

  double start[3] = {0, 0, WAND_HEIGHT};
  double end[3];
  apply_quaternion(q, end);
  end[2] += WAND_HEIGHT;
  double v[3] = {end[0] - start[0], end[1] - start[1], end[2] - start[2]};
  if (v[2] < 0) return;
  
  for (int i = 0; i < 3; i++) {
    double denom = 0.0;
    for (int j = 0; j < 3; j++)
      denom += v[j] * plane_normals[i][j];
    if (denom < 0.01) continue;

    double dot = 0.0;
    for (int j = 0; j < 3; j++)
      dot += (surfaces[i][0][j] - end[j]) * (plane_normals[i][j] / denom);
    for (int j = 0; j < 3; j++)
      result[j] = end[j] + dot * v[j];
    
    if (point_in_surface(surfaces[i][0], surfaces[i][1], surfaces[i][2], surfaces[i][3], result)) {
      *laser_index = i;
      return;
    }
  }
}

rgb_t Sierpinski::get_wand_rotation_color(double q[4], int degree_offset) {
  int angle = wand_rotation(q);
  return get_color_from_angle(angle + degree_offset);
}

rgb_t Sierpinski::get_color_from_angle(int angle) {
  return hsv_to_rgb((double)(angle % 360) / 360.0, 1.0, 1.0);
}
