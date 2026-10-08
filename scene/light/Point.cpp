// Copyright 2023-2026 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "Point.h"

namespace visionaray {

Point::Point(VisionarayGlobalState *s) : Light(s)
{
  vlight.type = dco::Light::Point;
}

Point::~Point()
{
}

void Point::commitParameters()
{
  Light::commitParameters();
  m_position = getParam<vec3>("position", vec3(0.f, 0.f, -1.f));
  m_radius = std::clamp(getParam<float>("radius", 0.f),
      0.f,
      std::numeric_limits<float>::max());
  // m_intensity is the radiant intensity in W/sr; 'power' is the total flux
  // and 'radiance' the radiance emitted by the surface of a sphere light
  if (hasParam("intensity"))
    m_intensity = getParam<float>("intensity", 1.f);
  else if (hasParam("power"))
    m_intensity = getParam<float>("power", 1.f) / (4.f * float(M_PI));
  else if (hasParam("radiance") && m_radius > 0.f)
    m_intensity =
        getParam<float>("radiance", 1.f) * float(M_PI) * m_radius * m_radius;
  else
    m_intensity = 1.f;
  m_intensity = std::clamp(m_intensity,
      0.f,
      std::numeric_limits<float>::max());
}

void Point::finalize()
{
  Light::finalize();
  vlight.asPoint.position = m_position;
  vlight.asPoint.color = m_color;
  vlight.asPoint.lightIntensity = m_intensity;
  vlight.asPoint.radius = m_radius;

  dispatch();
}

} // visionaray
