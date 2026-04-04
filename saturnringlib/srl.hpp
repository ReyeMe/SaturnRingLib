#pragma once

// Validates the environment 
static_assert(SRL_MAX_TEXTURES > 0,
    "SRL_MAX_TEXTURES must be greater than 0");

static_assert(SGL_MAX_VERTICES > 15,
    "SGL_MAX_VERTICES must be greater or equal to 16");

static_assert(SGL_MAX_POLYGONS > 4,
    "SGL_MAX_POLYGONS must be greater than 4");

#include "srl_core.hpp"
#include "srl_datetime.hpp"
#include "srl_tga.hpp"
#include "srl_scene2d.hpp"
#include "srl_scene3d.hpp"


#if SRL_USE_SGL_SOUND_DRIVER == 1
    #include "srl_cinepak.hpp"
#endif