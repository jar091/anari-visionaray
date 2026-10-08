// Copyright 2023-2026 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "ISOSurface.h"

namespace visionaray {

ISOSurface::ISOSurface(VisionarayGlobalState *d)
  : Geometry(d)
  , m_BVH(d)
  , m_field(this)
  , m_isoValue(this)
{
  vgeom.type = dco::Geometry::ISOSurface;
}

ISOSurface::~ISOSurface()
{
}

void ISOSurface::commitParameters()
{
  Geometry::commitParameters();
  m_field = getParamObject<SpatialField>("field");
  m_isoValue = getParamObject<Array1D>("isovalue");
  m_hasUniformIsoValue =
      getParam("isovalue", ANARI_FLOAT32, &m_uniformIsoValue);
}

void ISOSurface::finalize()
{
  if (!m_field) {
    reportMessage(ANARI_SEVERITY_WARNING,
        "no spatial field provided to implicitISOSurface geometry");
    return;
  }

  const bool hasIsoValueArray = m_isoValue && m_isoValue->size() > 0;
  if (!hasIsoValueArray && !m_hasUniformIsoValue) {
    reportMessage(ANARI_SEVERITY_WARNING,
        "no ISO values provided to implicitISOSurface geometry");
    return;
  }

  // device copy of the ISO values (a single FLOAT32 or an array)
  if (hasIsoValueArray) {
    m_isoValues.resize(m_isoValue->size());
    m_isoValues.reset(m_isoValue->beginAs<float>());
  } else {
    m_isoValues.resize(1);
    m_isoValues[0] = m_uniformIsoValue;
  }

  if (!m_field->isValid()) {
    m_field->finalize();
    m_field->markFinalized();
  }

  if (!m_field->isValid()) {
    reportMessage(ANARI_SEVERITY_WARNING,
        "spatial field not valid on implicitISOSurface geometry");
    return;
  }

  m_isoSurface.resize(1);

  m_isoSurface[0].field = m_field->visionaraySpatialField();
  m_isoSurface[0].bounds = m_field->bounds();
  m_isoSurface[0].numValues = m_isoValues.size();
  m_isoSurface[0].values = m_isoValues.devicePtr();

  vgeom.primitives.data = m_isoSurface.devicePtr();
  vgeom.primitives.len = m_isoSurface.size();

  m_BVH.update((const dco::ISOSurface *)vgeom.primitives.data,
               vgeom.primitives.len,
               BVH_FLAG_NO_STREAM_SYNCHRONIZE); // no spatial splits for ISOs 

  vBLS.type = dco::BLS::ISOSurface;
#if defined(WITH_CUDA) || defined(WITH_HIP)
  vBLS.asISOSurface = m_BVH.deviceBVH2();
#else
  vBLS.asISOSurface = m_BVH.deviceBVH4();
#endif

  deviceState()->objectUpdates.lastBLSReconstructSceneRequest = helium::newTimeStamp();

  dispatch();
}

bool ISOSurface::isValid() const
{
  return m_field && m_field->isValid() && (m_isoValue || m_hasUniformIsoValue);
}

} // namespace visionaray
