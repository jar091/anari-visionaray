// Copyright 2023-2026 Stefan Zellmann
// SPDX-License-Identifier: Apache-2.0

#pragma once

// visionaray
#include "visionaray/matrix_camera.h"
#include "visionaray/thin_lens_camera.h"
// ours
#include "dco/common.h"

namespace visionaray::dco {

// Camera //

struct Camera
{
  enum Type { Matrix, Pinhole, Omni, Ortho, Unknown, };
  Type type;
  unsigned camID;
  box1 shutter;
  // Clip planes of the camera rays (pinhole and ortho cameras only). The
  // planes are perpendicular to clipDir, the distances are measured along
  // clipDir from clipPos (not along the ray); 0 and FLT_MAX: no clipping
  float3 clipPos;
  float3 clipDir;
  float clipNear;
  float clipFar;
  thin_lens_camera asPinholeCam;
  union {
    matrix_camera asMatrixCam;

    struct {
      void init(float3 pos, float3 dir, float3 up)
      {
        this->pos = pos;
        this->dir = dir;
        this->up  = up;

        float2 imgPlaneSize(1.f, 1.f);

        U = normalize(cross(dir, up)) * imgPlaneSize.x;
        V = normalize(cross(U, dir)) * imgPlaneSize.y;
        W = pos - 0.5f * U - 0.5f * V;
      }

      VSNRAY_FUNC
      inline Ray primary_ray(Ray/**/, float x, float y, float width, float height) const
      {
        float2 screen((x + 0.5f) / width, (y + 0.5f) / height);

        Ray ray;

        float theta = float(M_PI) * screen.y;
        float phi = float(2*M_PI) * screen.x;

        float3 localDir(sinf(theta) * cosf(phi),
                        cosf(theta),
                        sinf(theta) * sinf(phi));

        ray.ori = U * screen.x + V * screen.y + W;
        ray.dir = localDir * float3(1,-1,1);

        ray.tmin = 0.f;
        ray.tmax = FLT_MAX;
        return ray;
      }

      float3 dir,pos,up;
      float3 U, V, W;
    } asOmniCam;

    struct {
      void init(float3 pos, float3 dir, float3 up, float aspect, float height,
                box2f image_region)
      {
        this->pos = pos;
        this->dir = dir;
        this->up  = up;
        this->image_region = image_region;

        float2 imgPlaneSize(height * aspect, height);

        U = normalize(cross(dir, up)) * imgPlaneSize.x;
        V = normalize(cross(U, dir)) * imgPlaneSize.y;
        W = pos - 0.5f * U - 0.5f * V;
      }

      VSNRAY_FUNC
      inline Ray primary_ray(Ray/**/, float x, float y, float width, float height) const
      {
        float2 screen((x + 0.5f) / width, (y + 0.5f) / height);
        screen = (float2(1.0) - screen) * float2(image_region.min)
                               + screen * float2(image_region.max);

        Ray ray;
        ray.ori = U * screen.x + V * screen.y + W;
        ray.dir = dir;
        ray.tmin = 0.f;
        ray.tmax = FLT_MAX;
        return ray;
      }
      float3 dir,pos,up;
      float3 U, V, W;
      box2f image_region;
    } asOrthoCam;
  };

  template <typename RNG>
  VSNRAY_FUNC
  inline Ray primary_ray(RNG &rng, float x, float y, float width, float height) const
  {
    Ray ray;
    if (type == Pinhole)
      ray = asPinholeCam.primary_ray(Ray{}, rng, x, y, width, height);
    else if (type == Omni)
      ray = asOmniCam.primary_ray(Ray{}, x, y, width, height);
    else if (type == Ortho)
      ray = asOrthoCam.primary_ray(Ray{}, x, y, width, height);
    else if (type == Matrix)
      ray = asMatrixCam.primary_ray(Ray{}, x, y, width, height);

    if (type == Pinhole || type == Ortho)
      clipPrimaryRay(ray);

    ray.time = lerp_r(shutter.min, shutter.max, rng());

    return ray;
  }

  // Restrict a camera ray to the slab between the near and far clip planes.
  // Only the ray interval changes, so hit distances remain relative to the
  // ray origin (camera position resp. lens sample); rays spawned later on
  // (shadow, bounce) are not clipped
  VSNRAY_FUNC
  inline void clipPrimaryRay(Ray &ray) const
  {
    const bool hasNear = clipNear > 0.f;
    const bool hasFar = clipFar < FLT_MAX;
    if (!hasNear && !hasFar)
      return;

    const float cosAxis = dot(ray.dir, clipDir);
    if (cosAxis > 1e-6f) {
      const float axisDist = dot(ray.ori - clipPos, clipDir);
      if (hasNear)
        ray.tmin = max(ray.tmin, (clipNear - axisDist) / cosAxis);
      if (hasFar)
        ray.tmax = min(ray.tmax, (clipFar - axisDist) / cosAxis);
    } else {
      ray.tmax = -1.f; // ray never enters the slab
    }
  }
};

VSNRAY_FUNC
inline Camera createCamera()
{
  Camera cam;
  memset(&cam,0,sizeof(cam));
  cam.type = Camera::Unknown;
  cam.camID = UINT_MAX;
  cam.shutter = {0.5f, 0.5f};
  cam.clipNear = 0.f;
  cam.clipFar = FLT_MAX;
  return cam;
}

} // namespace visionaray::dco
