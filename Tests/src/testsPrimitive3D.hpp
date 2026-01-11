#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_primitive3d.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using namespace SRL::Types;
using namespace SRL::Math::Types;

extern "C"
{
    extern const uint8_t buffer_size;
    extern char buffer[];

    void primitive3d_test_setup(void)
    {
    }

    void primitive3d_test_teardown(void)
    {
    }

    void primitive3d_test_output_header(void)
    {
        if (!suite_error_counter++)
        {
            if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
            {
                LogDebug("****UT_PRIMITIVE3D****");
            }
            else
            {
                LogInfo("****UT_PRIMITIVE3D_ERROR(S)****");
            }
        }
    }

    MU_TEST(primitive3d_test_cube_vertex_count)
    {
        Mesh cube = Primitive3D::CreateCube(Fxp(10), HighColor(255, 0, 0));

        snprintf(buffer, buffer_size, "Cube vertex count failed: %d != 8", (int)cube.VertexCount);
        mu_assert(cube.VertexCount == 8, buffer);

        snprintf(buffer, buffer_size, "Cube face count failed: %d != 6", (int)cube.FaceCount);
        mu_assert(cube.FaceCount == 6, buffer);
    }

    MU_TEST(primitive3d_test_cube_vertices_not_null)
    {
        Mesh cube = Primitive3D::CreateCube(Fxp(5), HighColor(0, 255, 0));

        snprintf(buffer, buffer_size, "Cube vertices pointer is null");
        mu_assert(cube.Vertices != nullptr, buffer);

        snprintf(buffer, buffer_size, "Cube faces pointer is null");
        mu_assert(cube.Faces != nullptr, buffer);

        snprintf(buffer, buffer_size, "Cube attributes pointer is null");
        mu_assert(cube.Attributes != nullptr, buffer);
    }

    MU_TEST(primitive3d_test_cube_vertex_positions)
    {
        Fxp size = Fxp(10);
        Mesh cube = Primitive3D::CreateCube(size, HighColor(255, 255, 255));

        snprintf(buffer, buffer_size, "Cube vertex 0 X failed: %f != %f", cube.Vertices[0].X.As<float>(), (-size).As<float>());
        mu_assert(cube.Vertices[0].X == -size, buffer);

        snprintf(buffer, buffer_size, "Cube vertex 6 X failed: %f != %f", cube.Vertices[6].X.As<float>(), size.As<float>());
        mu_assert(cube.Vertices[6].X == size, buffer);

        snprintf(buffer, buffer_size, "Cube vertex 6 Y failed: %f != %f", cube.Vertices[6].Y.As<float>(), size.As<float>());
        mu_assert(cube.Vertices[6].Y == size, buffer);

        snprintf(buffer, buffer_size, "Cube vertex 6 Z failed: %f != %f", cube.Vertices[6].Z.As<float>(), size.As<float>());
        mu_assert(cube.Vertices[6].Z == size, buffer);
    }

    MU_TEST(primitive3d_test_plane_vertex_count)
    {
        Mesh plane = Primitive3D::CreatePlane(Fxp(10), Fxp(10), HighColor(0, 0, 255));

        snprintf(buffer, buffer_size, "Plane vertex count failed: %d != 4", (int)plane.VertexCount);
        mu_assert(plane.VertexCount == 4, buffer);

        snprintf(buffer, buffer_size, "Plane face count failed: %d != 1", (int)plane.FaceCount);
        mu_assert(plane.FaceCount == 1, buffer);
    }

    MU_TEST(primitive3d_test_plane_vertices_on_xz)
    {
        Mesh plane = Primitive3D::CreatePlane(Fxp(5), Fxp(5), HighColor(128, 128, 128));

        for (size_t i = 0; i < plane.VertexCount; i++)
        {
            snprintf(buffer, buffer_size, "Plane vertex %d Y not zero: %f", (int)i, plane.Vertices[i].Y.As<float>());
            mu_assert(plane.Vertices[i].Y == Fxp(0), buffer);
        }
    }

    MU_TEST(primitive3d_test_plane_double_sided)
    {
        Mesh planeSingle = Primitive3D::CreatePlane(Fxp(5), Fxp(5), HighColor(255, 0, 0), false);
        Mesh planeDouble = Primitive3D::CreatePlane(Fxp(5), Fxp(5), HighColor(255, 0, 0), true);

        snprintf(buffer, buffer_size, "Single-sided plane visibility wrong");
        mu_assert(planeSingle.Attributes[0].Visibility == Attribute::FaceVisibility::SingleSided, buffer);

        snprintf(buffer, buffer_size, "Double-sided plane visibility wrong");
        mu_assert(planeDouble.Attributes[0].Visibility == Attribute::FaceVisibility::DoubleSided, buffer);
    }

    MU_TEST(primitive3d_test_sphere_vertex_count)
    {
        uint16_t segments = 8;
        uint16_t rings = 4;
        size_t expectedVertices = (rings + 1) * segments;
        size_t expectedFaces = rings * segments;

        Mesh sphere = Primitive3D::CreateSphere(Fxp(10), segments, rings, HighColor(255, 255, 0));

        snprintf(buffer, buffer_size, "Sphere vertex count failed: %d != %d", (int)sphere.VertexCount, (int)expectedVertices);
        mu_assert(sphere.VertexCount == expectedVertices, buffer);

        snprintf(buffer, buffer_size, "Sphere face count failed: %d != %d", (int)sphere.FaceCount, (int)expectedFaces);
        mu_assert(sphere.FaceCount == expectedFaces, buffer);
    }

    MU_TEST(primitive3d_test_sphere_minimum_segments)
    {
        Mesh sphere = Primitive3D::CreateSphere(Fxp(10), 2, 1, HighColor(0, 255, 255));

        snprintf(buffer, buffer_size, "Sphere minimum segments not enforced: %d", (int)sphere.VertexCount);
        mu_assert(sphere.VertexCount >= 12, buffer);

        snprintf(buffer, buffer_size, "Sphere minimum rings not enforced: %d", (int)sphere.FaceCount);
        mu_assert(sphere.FaceCount >= 8, buffer);
    }

    MU_TEST(primitive3d_test_sphere_vertices_not_null)
    {
        Mesh sphere = Primitive3D::CreateSphere(Fxp(5), 6, 4, HighColor(255, 128, 0));

        snprintf(buffer, buffer_size, "Sphere vertices pointer is null");
        mu_assert(sphere.Vertices != nullptr, buffer);

        snprintf(buffer, buffer_size, "Sphere faces pointer is null");
        mu_assert(sphere.Faces != nullptr, buffer);

        snprintf(buffer, buffer_size, "Sphere attributes pointer is null");
        mu_assert(sphere.Attributes != nullptr, buffer);
    }

    MU_TEST_SUITE(primitive3d_test_suite)
    {
        MU_SUITE_CONFIGURE_WITH_HEADER(&primitive3d_test_setup,
                                       &primitive3d_test_teardown,
                                       &primitive3d_test_output_header);

        MU_RUN_TEST(primitive3d_test_cube_vertex_count);
        MU_RUN_TEST(primitive3d_test_cube_vertices_not_null);
        MU_RUN_TEST(primitive3d_test_cube_vertex_positions);
        MU_RUN_TEST(primitive3d_test_plane_vertex_count);
        MU_RUN_TEST(primitive3d_test_plane_vertices_on_xz);
        MU_RUN_TEST(primitive3d_test_plane_double_sided);
        MU_RUN_TEST(primitive3d_test_sphere_vertex_count);
        MU_RUN_TEST(primitive3d_test_sphere_minimum_segments);
        MU_RUN_TEST(primitive3d_test_sphere_vertices_not_null);
    }
}
