// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SRC_UI_SCENIC_LIB_GFX_RESOURCES_SHAPES_ROUNDED_RECTANGLE_SHAPE_H_
#define SRC_UI_SCENIC_LIB_GFX_RESOURCES_SHAPES_ROUNDED_RECTANGLE_SHAPE_H_

#include "src/ui/lib/escher/forward_declarations.h"
#include "src/ui/lib/escher/shape/rounded_rect.h"
#include "src/ui/scenic/lib/gfx/resources/shapes/planar_shape.h"

namespace scenic_impl {
namespace gfx {

class RoundedRectangleShape final : public PlanarShape {
 public:
  static const ResourceTypeInfo kTypeInfo;

  RoundedRectangleShape(Session* session, SessionId session_id, ResourceId id,
                        const escher::RoundedRectSpec& spec, escher::MeshPtr mesh);

  float width() const { return spec_.width; }
  float height() const { return spec_.height; }
  float top_left_radius() const { return spec_.top_left_radius; }
  float top_right_radius() const { return spec_.top_right_radius; }
  float bottom_right_radius() const { return spec_.bottom_right_radius; }
  float bottom_left_radius() const { return spec_.bottom_left_radius; }

  // |Resource|.
  void Accept(class ResourceVisitor* visitor) override;

  // |PlanarShape|.
  bool ContainsPoint(const escher::vec2& point) const override;

  // |Shape|.
  escher::Object GenerateRenderObject(const escher::mat4& transform,
                                      const escher::MaterialPtr& material) override;

  const escher::MeshPtr& escher_mesh() const {
    // During tests, this mesh can be null, but otherwise
    // we expect it to be allocated.
    // TODO(SCN-1429) This is going to be removed anyway.
    FXL_CHECK(mesh_);
    return mesh_;
  }

  const escher::RoundedRectSpec& spec() const { return spec_; }

 private:
  escher::RoundedRectSpec spec_;
  escher::MeshPtr mesh_;
};

}  // namespace gfx
}  // namespace scenic_impl

#endif  // SRC_UI_SCENIC_LIB_GFX_RESOURCES_SHAPES_ROUNDED_RECTANGLE_SHAPE_H_
