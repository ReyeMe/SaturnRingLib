#pragma once

// Validates the environment 
static_assert(SRL_MAX_TEXTURES > 0,
    "SRL_MAX_TEXTURES must be greater than 0");

#include "srl_core.hpp"
#include "srl_datetime.hpp"
#include "srl_tga.hpp"
#include "srl_scene2d.hpp"
#include "srl_scene3d.hpp"
