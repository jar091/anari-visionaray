// Copyright 2023-2026 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#include "Quad.h"

namespace visionaray {

QuadLight::QuadLight(VisionarayGlobalState *s) : Light(s)
{
  vlight.type = dco::Light::Quad;
}

QuadLight::~QuadLight()
{
}

void QuadLight::commitParameters()
{
  Light::commitParameters();
  m_position = getParam<vec3>("position", vec3(0.f, 0.f, 0.f));
  m_edge1 = getParam<vec3>("edge1", vec3(1.f, 0.f, 0.f));
  m_edge2 = getParam<vec3>("edge2", vec3(0.f, 1.f, 0.f));
  m_side = getParamString("side", "front");
  // m_intensity is the radiant intensity in W/sr along the normal; the light
  // divides it by the area to get the emitted radiance. 'radiance' is that
  // radiance directly, 'power' the flux of the one-sided Lambertian emitter.
  const float area = length(cross(m_edge1, m_edge2));
  if (hasParam("radiance"))
    m_intensity = getParam<float>("radiance", 1.f) * area;
  else if (hasParam("intensity") || !hasParam("power"))
    m_intensity = getParam<float>("intensity", 1.f);
  else
    m_intensity = getParam<float>("power", 1.f) / float(M_PI);
  m_intensity = std::clamp(m_intensity,
      0.f,
      std::numeric_limits<float>::max());
}

void QuadLight::finalize()
{
  Light::finalize();

  if (length(m_edge1) == 0.f || length(m_edge2) == 0.f) {
    reportMessage(ANARI_SEVERITY_WARNING,
        "quad light has zero-length edges");
    return;
  }

  if (m_side != "front" && m_side != "back" && m_side != "both") {
    reportMessage(ANARI_SEVERITY_WARNING,
        "quad light has invalid side: %s", m_side.c_str());
    return;
  }

  vlight.asQuad.geometry() = dco::Quad{m_position,m_edge1,m_edge2};
  vlight.asQuad.set_cl(m_color);
  vlight.asQuad.set_kl(m_intensity);
  vlight.asQuad.side = m_side == "front" ? dco::Light::Front
      : m_side == "back" ? dco::Light::Back : dco::Light::Both;

  dco::Quad temp{m_position,m_edge1,m_edge2};
  basic_triangle<3,float> t1,t2;
  temp.tessellate(t1,t2);

  dispatch();
}

} // visionaray
