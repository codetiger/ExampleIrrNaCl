// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "cube.h"
#include "shader_util.h"
#include "transforms.h"
#include "testirrlicht.h"

namespace tumbler {

static const size_t kVertexCount = 24;
static const int kIndexCount = 36;

Cube::Cube(SharedOpenGLContext opengl_context)
    : opengl_context_(opengl_context),
      width_(1),
      height_(1) {
  eye_[0] = eye_[1] = 0.0f;
  eye_[2] = 2.0f;
  orientation_[0] = 0.0f;
  orientation_[1] = 0.0f;
  orientation_[2] = 0.0f;
  orientation_[3] = 1.0f;
}

Cube::~Cube() {
}

void Cube::PrepareOpenGL() {
	setupGraphics(width_, height_);
}

void Cube::Resize(int width, int height) {
  width_ = std::max(width, 1);
  height_ = std::max(height, 1);
  // Set the viewport
  glViewport(0, 0, width_, height_);
  // Compute the perspective projection matrix with a 60 degree FOV.
  GLfloat aspect = static_cast<GLfloat>(width_) / static_cast<GLfloat>(height_);
  transform_4x4::LoadIdentity(perspective_proj_);
  transform_4x4::Perspective(perspective_proj_, 60.0f, aspect, 1.0f, 20.0f);
}

void Cube::Draw() {
	renderFrame();
}

void Cube::ComputeModelViewTransform(GLfloat* model_view) {
  // This method takes into account the possiblity that |orientation_|
  // might not be normalized.
  double sqrx = orientation_[0] * orientation_[0];
  double sqry = orientation_[1] * orientation_[1];
  double sqrz = orientation_[2] * orientation_[2];
  double sqrw = orientation_[3] * orientation_[3];
  double sqrLength = 1.0 / (sqrx + sqry + sqrz + sqrw);

  transform_4x4::LoadIdentity(model_view);
  model_view[0] = (sqrx - sqry - sqrz + sqrw) * sqrLength;
  model_view[5] = (-sqrx + sqry - sqrz + sqrw) * sqrLength;
  model_view[10] = (-sqrx - sqry + sqrz + sqrw) * sqrLength;

  double temp1 = orientation_[0] * orientation_[1];
  double temp2 = orientation_[2] * orientation_[3];
  model_view[1] = 2.0 * (temp1 + temp2) * sqrLength;
  model_view[4] = 2.0 * (temp1 - temp2) * sqrLength;

  temp1 = orientation_[0] * orientation_[2];
  temp2 = orientation_[1] * orientation_[3];
  model_view[2] = 2.0 * (temp1 - temp2) * sqrLength;
  model_view[8] = 2.0 * (temp1 + temp2) * sqrLength;
  temp1 = orientation_[1] * orientation_[2];
  temp2 = orientation_[0] * orientation_[3];
  model_view[6] = 2.0 * (temp1 + temp2) * sqrLength;
  model_view[9] = 2.0 * (temp1 - temp2) * sqrLength;
  model_view[3] = 0.0;
  model_view[7] = 0.0;
  model_view[11] = 0.0;

  // Concatenate the translation to the eye point.
  model_view[12] = -eye_[0];
  model_view[13] = -eye_[1];
  model_view[14] = -eye_[2];
  model_view[15] = 1.0;
}

}  // namespace tumbler
