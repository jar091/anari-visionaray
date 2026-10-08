// Copyright 2023-2026 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "Spot.h"

namespace visionaray {

Spot::Spot(VisionarayGlobalState *s) : Light(s)
{
  vlight.type = dco::Light::Spot;
}

Spot::~Spot()
{
}

void Spot::commitParameters()
{
  Light::commitParameters();
  m_position = getParam<vec3>("position", vec3(0.f, 0.f, 0.f));
  m_direction = getParam<vec3>("direction", vec3(0.f, 0.f, -1.f));
  m_openingAngle = getParam<float>("openingAngle", M_PI);
  m_falloffAngle = getParam<float>("falloffAngle", 0.1f);
  // 'intensity' is the radiant intensity in W/sr, 'power' the flux emitted
  // into the cone; 'intensity' takes precedence
  if (hasParam("intensity") || !hasParam("power")) {
    m_intensity = getParam<float>("intensity", 1.f);
  } else {
    const float solidAngle =
        2.f * float(M_PI) * (1.f - cosf(0.5f * m_openingAngle));
    m_intensity = getParam<float>("power", 1.f) / std::max(solidAngle, 1e-6f);
  }
  m_intensity = std::clamp(m_intensity,
      0.f,
      std::numeric_limits<float>::max());
}

void Spot::finalize()
{
  Light::finalize();

  float innerAngle = m_openingAngle - 2.f * m_falloffAngle;
  if (innerAngle < 0.f) {
    reportMessage(ANARI_SEVERITY_WARNING,
        "falloffAngle should be smaller than half of openingAngle");
  }

  vlight.asSpot.position = m_position;
  vlight.asSpot.direction = m_direction;
  vlight.asSpot.color = m_color;
  vlight.asSpot.lightIntensity = m_intensity;

  // 'openingAngle' is the full apex angle of the cone, the light compares
  // against the cosines of the half angles
  vlight.asSpot.cosOuterAngle = cosf(0.5f * m_openingAngle);
  vlight.asSpot.cosInnerAngle = cosf(0.5f * std::max(innerAngle, 0.f));

  dispatch();
}

} // visionaray
