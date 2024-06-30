#pragma once

#include "srl_base.hpp"
#include "srl_mesh.hpp"

namespace SRL
{
    /** @brief Rendering of 3D objects
     */
    class Scene3D
    {
    public:

        /** @brief Draw mesh
         * @param mesh Mesh to draw
         * @param light Light direction
         * @return True on success
         */
        static void DrawSmoothMesh(Types::SmoothMesh& mesh, Types::Vector3D& light)
        {
            slPutPolygonX(mesh.SglPtr(), (FIXED*)&light);
        }

        /** @brief Draw mesh
         * @param mesh Mesh to draw
         * @return True on success
         */
        static bool DrawMesh(Types::Mesh& mesh)
        {
            return slPutPolygonS(mesh.SglPtr());
        }
    };
}

