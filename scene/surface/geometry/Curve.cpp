// Copyright 2023-2026 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "Curve.h"

namespace visionaray {

Curve::Curve(VisionarayGlobalState *s)
  : Geometry(s)
  , m_BVH(s)
  , m_index(this)
  , m_vertexPosition(this)
  , m_vertexRadius(this)
{
  vgeom.type = dco::Geometry::Cone;
}

void Curve::commitParameters()
{
  Geometry::commitParameters();
  m_index = getParamObject<Array1D>("primitive.index");
  m_vertexPosition = getParamObject<Array1D>("vertex.position");
  m_vertexRadius = getParamObject<Array1D>("vertex.radius");
  m_vertexAttributes[0] = getParamObject<Array1D>("vertex.attribute0");
  m_vertexAttributes[1] = getParamObject<Array1D>("vertex.attribute1");
  m_vertexAttributes[2] = getParamObject<Array1D>("vertex.attribute2");
  m_vertexAttributes[3] = getParamObject<Array1D>("vertex.attribute3");
  m_vertexAttributes[4] = getParamObject<Array1D>("vertex.color");
  m_globalRadius = getParam<float>("radius", 1.f);
}

void Curve::finalize()
{
  Geometry::finalize();

  if (!m_vertexPosition) {
    reportMessage(ANARI_SEVERITY_WARNING,
        "missing required parameter 'vertex.position' on curve geometry");
    return;
  }

  const size_t numVertices = m_vertexPosition->size();
  const auto *vertices = m_vertexPosition->beginAs<float3>();
  const float *radii = m_vertexRadius ? m_vertexRadius->beginAs<float>() : nullptr;

  // Each primitive is the segment between the vertices index and index+1;
  // without an index, consecutive vertex pairs form the segments.
  const size_t numSegments = m_index ? m_index->size() : numVertices / 2;

  m_cones.resize(numSegments);
  vindex.resize(numSegments);

  for (size_t i=0; i<numSegments; ++i) {
    const unsigned first = m_index ? m_index->beginAs<unsigned>()[i] : unsigned(i*2);
    dco::Cone cone;
    cone.prim_id = i;
    cone.geom_id = -1;
    if (size_t(first)+1 < numVertices) {
      cone.v1 = vertices[first];
      cone.v2 = vertices[first+1];
      cone.r1 = radii ? radii[first] : m_globalRadius;
      cone.r2 = radii ? radii[first+1] : m_globalRadius;
      vindex[i] = uint2(first, first+1);
    } else {
      // invalid index: degenerate segment that cannot be hit
      cone.v1 = cone.v2 = float3(0.f);
      cone.r1 = cone.r2 = 0.f;
      vindex[i] = uint2(0, 0);
    }
    m_cones[i] = cone;
  }

  vgeom.primitives.data = m_cones.devicePtr();
  vgeom.primitives.len = m_cones.size();

  vgeom.index.data = vindex.devicePtr();
  vgeom.index.len = vindex.size();
  vgeom.index.typeInfo = getInfo(ANARI_UINT32_VEC2);

  for (int i = 0; i < 5; ++i ) {
    if (m_vertexAttributes[i]) {
      size_t sizeInBytes
          = m_vertexAttributes[i]->size()
          * anari::sizeOf(m_vertexAttributes[i]->elementType());

      vattributes[i].resize(sizeInBytes);
      vattributes[i].reset(m_vertexAttributes[i]->begin());

      vgeom.vertex.attributes[i].data = vattributes[i].devicePtr();
      vgeom.vertex.attributes[i].len = m_vertexAttributes[i]->size();
      vgeom.vertex.attributes[i].typeInfo
          = getInfo(m_vertexAttributes[i]->elementType());
    }
  }

  m_BVH.update((const dco::Cone *)vgeom.primitives.data,
               vgeom.primitives.len,
               BVH_FLAG_NO_STREAM_SYNCHRONIZE); // no spatial splits for cones yet!

  vBLS.type = dco::BLS::Cone;
#if defined(WITH_CUDA) || defined(WITH_HIP)
  vBLS.asCone = m_BVH.deviceBVH2();
#else
  vBLS.asCone = m_BVH.deviceBVH4();
#endif

  dispatch();
}

} // namespace visionaray
