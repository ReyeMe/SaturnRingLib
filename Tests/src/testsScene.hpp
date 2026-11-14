// Tests/src/testsScene.hpp
// SRL::Types::Attribute, Mesh and SmoothMesh, SRL::Scene2D sprite attributes and SRL::Scene3D projection, visibility
// and polygon drawing.
#pragma once

#include <type_traits>
#include <utility>

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_mesh.hpp>
#include <srl_scene2d.hpp>
#include <srl_scene3d.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Math::Types::Fxp;
using SRL::Math::Types::Vector2D;
using SRL::Math::Types::Vector3D;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

void scene_test_setup(void) {}
void scene_test_teardown(void) {}

void scene_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        LogInfo("****UT_SCENE_ERROR(S)****");
    }
}

/** @brief Used blocks of both work RAM heaps, to check that objects give their memory back. */
static size_t scene_used_blocks()
{
    return Memory::HighWorkRam::GetReport().UsedBlocks + Memory::LowWorkRam::GetReport().UsedBlocks;
}

/** @brief The 8-argument Attribute constructor builds the same bytes as SGL's ATTRIBUTE macro. */
MU_TEST(scene_attribute_matches_sgl_macro)
{
    const Types::Attribute attribute(Types::Attribute::DoubleSided, Types::Attribute::Center, 5, 0x1234, 0xe010,
        CL_Half | CL32KRGB, sprNoflip, UseLight);
    const ATTR expected = ATTRIBUTE(Dual_Plane, SORT_CEN, 5, 0x1234, 0xe010, CL_Half | CL32KRGB, sprNoflip, UseLight);

    mu_assert(sizeof(attribute) == sizeof(expected), "Attribute and ATTR differ in size");
    snprintf(buffer, buffer_size, "Sort 0x%02x/0x%02x, display 0x%04x/0x%04x, direction 0x%04x/0x%04x (SRL/SGL)",
        attribute.Sort, expected.sort, attribute.Display, expected.atrb, attribute.Direction, expected.dir);
    mu_assert(memcmp(&attribute, &expected, sizeof(expected)) == 0, buffer);
}

/** @brief The texture constructor with an RGB texture: no palette flag, RGB display mode, gouraud table address. */
MU_TEST(scene_attribute_rgb_texture)
{
    const Types::Attribute attribute(Types::Attribute::SingleSided, Types::Attribute::Center, 3, 0, 4,
        CRAM::TextureColorMode::RGB555, Types::Attribute::NoMode, Types::Attribute::Sprite, Types::Attribute::NoOption);

    mu_assert((attribute.Sort & UsePalette) == 0, "An RGB texture has the UsePalette flag");
    snprintf(buffer, buffer_size, "Color mode bits are 0x%02x, expected CL32KRGB 0x%02x", attribute.Display & 0x38, CL32KRGB);
    mu_assert((attribute.Display & 0x38) == CL32KRGB, buffer);
    snprintf(buffer, buffer_size, "Gouraud is 0x%04x, expected 0x%04x", attribute.Gouraud, 0xe000 + (4 << 2));
    mu_assert(attribute.Gouraud == 0xe000 + (4 << 2), buffer);
    mu_assert(attribute.Texture == 3, "Texture number not kept");
}

/** @brief The texture constructor with a 16-color texture sets the UsePalette sort flag SGL needs for palettes. */
MU_TEST(scene_attribute_paletted_texture)
{
    const Types::Attribute attribute(Types::Attribute::SingleSided, Types::Attribute::Center, 3, 0x40, 0,
        CRAM::TextureColorMode::Paletted16, Types::Attribute::NoMode, Types::Attribute::Sprite, Types::Attribute::NoOption);

    snprintf(buffer, buffer_size, "Color mode bits are 0x%02x, expected CL16Bnk 0x%02x", attribute.Display & 0x38, CL16Bnk);
    mu_assert((attribute.Display & 0x38) == CL16Bnk, buffer);
    snprintf(buffer, buffer_size, "Sort is 0x%02x: UsePalette (0x%02x) is missing", attribute.Sort, UsePalette);
    mu_assert((attribute.Sort & UsePalette) != 0, buffer);
}

/** @brief Mesh and SmoothMesh give their arrays back when destroyed. */
MU_TEST(scene_mesh_frees_its_arrays)
{
    const size_t before = scene_used_blocks();
    {
        Types::Mesh mesh(8, 6);
        mu_assert(mesh.Vertices != nullptr && mesh.Faces != nullptr && mesh.Attributes != nullptr, "Mesh arrays not allocated");
    }
    {
        Types::SmoothMesh mesh(8, 6);
        mu_assert(mesh.Normals != nullptr, "SmoothMesh normals not allocated");
    }
    snprintf(buffer, buffer_size, "Used blocks %d -> %d after destroying a Mesh and a SmoothMesh", before, scene_used_blocks());
    mu_assert(scene_used_blocks() == before, buffer);
}

/** @brief Move assignment hands the arrays over, empties the source and leaks nothing. */
MU_TEST(scene_mesh_move_assignment)
{
    const size_t before = scene_used_blocks();
    {
        Types::Mesh source(4, 2);
        Types::Mesh target(8, 6);
        Vector3D *vertices = source.Vertices;
        target = std::move(source);

        mu_assert(target.Vertices == vertices && target.VertexCount == 4 && target.FaceCount == 2, "Target did not take the source arrays");
        mu_assert(source.Vertices == nullptr && source.Faces == nullptr && source.Attributes == nullptr, "Source still owns its arrays");
    }
    snprintf(buffer, buffer_size, "Used blocks %d -> %d after the move and both destructors", before, scene_used_blocks());
    mu_assert(scene_used_blocks() == before, buffer);
}

/** @brief Copying a Mesh would free its arrays twice, so it must not compile. */
MU_TEST(scene_mesh_not_copyable)
{
    mu_assert(!std::is_copy_constructible<Types::Mesh>::value, "Mesh is copyable: both copies free the same arrays");
    mu_assert(!std::is_copy_constructible<Types::SmoothMesh>::value, "SmoothMesh is copyable: both copies free the same arrays");
}

/** @brief Sprite attributes take the color mode and color bank from the palette. */
MU_TEST(scene2d_sprite_attribute_palettes)
{
    CRAM::Palette palette16(CRAM::TextureColorMode::Paletted16, 3);
    const SPR_ATTR attr16 = Scene2D::GetSpriteAttribute(0, &palette16);
    snprintf(buffer, buffer_size, "16 colors, palette 3: color bank 0x%04x (expected 0x30), mode 0x%02x (expected 0x%02x)",
        attr16.colno, attr16.atrb & 0x38, CL16Bnk);
    mu_assert(attr16.colno == 0x30 && (attr16.atrb & 0x38) == CL16Bnk, buffer);

    CRAM::Palette palette256(CRAM::TextureColorMode::Paletted256, 2);
    const SPR_ATTR attr256 = Scene2D::GetSpriteAttribute(0, &palette256);
    snprintf(buffer, buffer_size, "256 colors, palette 2: color bank 0x%04x (expected 0x200), mode 0x%02x (expected 0x%02x)",
        attr256.colno, attr256.atrb & 0x38, CL256Bnk);
    mu_assert(attr256.colno == 0x200 && (attr256.atrb & 0x38) == CL256Bnk, buffer);

    CRAM::Palette rgb;
    const SPR_ATTR attrRgb = Scene2D::GetSpriteAttribute(0, &rgb);
    snprintf(buffer, buffer_size, "RGB: color bank 0x%04x (expected 0), mode 0x%02x (expected 0x%02x)",
        attrRgb.colno, attrRgb.atrb & 0x38, CL32KRGB);
    mu_assert(attrRgb.colno == No_Palet && (attrRgb.atrb & 0x38) == CL32KRGB, buffer);
}

/** @brief With an identity matrix, a point in front of the camera is on screen; points behind or far aside are not. */
MU_TEST(scene3d_is_on_screen)
{
    Scene3D::PushIdentityMatrix();
    const bool front = Scene3D::IsOnScreen(Vector3D(0, 0, 100), 1);
    const bool behind = Scene3D::IsOnScreen(Vector3D(0, 0, -100), 1);
    const bool aside = Scene3D::IsOnScreen(Vector3D(5000, 0, 100), 1);
    Scene3D::PopMatrix();

    snprintf(buffer, buffer_size, "On screen: in front %d (expected 1), behind %d (expected 0), far aside %d (expected 0)", front, behind, aside);
    mu_assert(front && !behind && !aside, buffer);
}

/** @brief A point straight ahead projects to the screen center, one to the right projects right of it. */
MU_TEST(scene3d_project_to_screen)
{
    Scene3D::PushIdentityMatrix();
    Vector2D center;
    Vector2D right;
    Scene3D::ProjectToScreen(Vector3D(0, 0, 100), &center);
    Scene3D::ProjectToScreen(Vector3D(10, 0, 100), &right);
    Scene3D::PopMatrix();

    snprintf(buffer, buffer_size, "Center projects to (%d, %d), expected (0, 0); (10,0,100) to x %d, expected > 0",
        center.X.As<int32_t>(), center.Y.As<int32_t>(), right.X.As<int32_t>());
    mu_assert(center.X == 0 && center.Y == 0 && right.X > 0 && right.Y == 0, buffer);
}

/** @brief DrawMesh accepts a quad in front of the camera and refuses polygons once the SGL polygon buffer is full. */
MU_TEST(scene3d_draw_mesh_until_full)
{
    static Vector3D vertices[4] = { Vector3D(-10, -10, 0), Vector3D(10, -10, 0), Vector3D(10, 10, 0), Vector3D(-10, 10, 0) };
    static const uint16_t indices[4] = { 0, 1, 2, 3 };
    static Types::Polygon faces[1] = { Types::Polygon(Vector3D(0, 0, -1), indices) };
    static Types::Attribute attributes[1] = { Types::Attribute(Types::Attribute::DoubleSided, Types::Attribute::Center,
        No_Texture, 0x7fff, No_Gouraud, CL32KRGB | MESHoff, sprPolygon, No_Option) };
    Types::MeshData mesh;
    mesh.Vertices = vertices;
    mesh.VertexCount = 4;
    mesh.Faces = faces;
    mesh.FaceCount = 1;
    mesh.Attributes = attributes;

    Scene3D::PushIdentityMatrix();
    Scene3D::Translate(0, 0, 100);
    int accepted = 0;
    while (accepted < SGL_MAX_POLYGONS + 10 && Scene3D::DrawMesh(mesh)) accepted++;
    Scene3D::PopMatrix();
    Core::Synchronize();    // draws and empties the polygon buffer

    snprintf(buffer, buffer_size, "DrawMesh accepted %d quads, expected between 1 and SGL_MAX_POLYGONS (%d)", accepted, SGL_MAX_POLYGONS);
    mu_assert(accepted >= 1 && accepted <= SGL_MAX_POLYGONS, buffer);
}

MU_TEST_SUITE(scene_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&scene_test_setup, &scene_test_teardown, &scene_test_output_header);

    MU_RUN_TEST(scene_attribute_matches_sgl_macro);
    MU_RUN_TEST(scene_attribute_rgb_texture);
    MU_RUN_TEST(scene_attribute_paletted_texture);
    MU_RUN_TEST(scene_mesh_frees_its_arrays);
    MU_RUN_TEST(scene_mesh_move_assignment);
    MU_RUN_TEST(scene_mesh_not_copyable);
    MU_RUN_TEST(scene2d_sprite_attribute_palettes);
    MU_RUN_TEST(scene3d_is_on_screen);
    MU_RUN_TEST(scene3d_project_to_screen);
    MU_RUN_TEST(scene3d_draw_mesh_until_full);
}
