#pragma once

#include "modelObject.hpp"

/** @brief Model object
 */
class AnimatedObject
{
private:

    /** @brief Current mesh state
     */
    void* currentMesh;

    /** @brief Current state of vertices
     */
    SRL::Types::Vector3D* vertexArray;

    /** @brief Current state of polygons
     */
    SRL::Types::Polygon* polygonArray;

    /** @brief Current state of normals
     */
    SRL::Types::Vector3D* normalsArray;

    /** @brief Interpolate a single point
     * @param startPoint Start point
     * @param endPoint End point
     * @param time Interpolation time (value between 0.0 and 1.0)
     * @param result Result of interpolation
     */
    inline void InterpolatePoint(const SRL::Types::Vector3D& startPoint, const SRL::Types::Vector3D& endPoint, const SRL::Types::Fxp time, SRL::Types::Vector3D* result)
    {
        *result = startPoint + ((endPoint - startPoint) * time);
    }

public:

    /** @brief Currently loaded keyframes
     */
    ModelObject* KeyFrames;

    /** @brief Construct a new Animated object
     * @param modelFile Model file containing key frames
     * @param gouraudTableStart Gouraud table start
     */
    AnimatedObject(const char* modelFile, size_t gouraudTableStart = 0)
    {
        this->KeyFrames = new ModelObject(modelFile, gouraudTableStart);

        if (this->IsSmooth())
        {
            SRL::Types::SmoothMesh* first = this->KeyFrames->GetMesh<SRL::Types::SmoothMesh>(0);

            SRL::Types::SmoothMesh* smooth = new SRL::Types::SmoothMesh(first->VertexCount, first->FaceCount);
            this->vertexArray = smooth->Vertices;
            this->polygonArray = smooth->Faces;
            this->normalsArray = smooth->Normals;

            // Copy polygons over
            for (size_t face = 0; face < first->FaceCount; face++)
            {
                smooth->Faces[face].Vertices[0] = first->Faces[face].Vertices[0];
                smooth->Faces[face].Vertices[1] = first->Faces[face].Vertices[1];
                smooth->Faces[face].Vertices[2] = first->Faces[face].Vertices[2];
                smooth->Faces[face].Vertices[3] = first->Faces[face].Vertices[3];
                smooth->Attributes[face] = first->Attributes[face];
            }
            
            this->currentMesh = smooth;
        }
        else
        {
            SRL::Types::Mesh* first = this->KeyFrames->GetMesh<SRL::Types::Mesh>(0);

            SRL::Types::Mesh* flat = new SRL::Types::Mesh(first->VertexCount, first->FaceCount);
            this->vertexArray = flat->Vertices;
            this->polygonArray = flat->Faces;
            this->normalsArray = nullptr;
            
            // Copy polygons over
            for (size_t face = 0; face < first->FaceCount; face++)
            {
                flat->Faces[face].Vertices[0] = first->Faces[face].Vertices[0];
                flat->Faces[face].Vertices[1] = first->Faces[face].Vertices[1];
                flat->Faces[face].Vertices[2] = first->Faces[face].Vertices[2];
                flat->Faces[face].Vertices[3] = first->Faces[face].Vertices[3];
                flat->Attributes[face] = first->Attributes[face];
            }
            
            this->currentMesh = flat;
        }

        // Copy points and normals over
        this->ProcessFrame(0.0);
    }

    /** @brief Destroy the Animated object
     */
    ~AnimatedObject()
    {
        if (!this->IsSmooth())
        {
            delete (SRL::Types::Mesh*)this->currentMesh;
        }
        else
        {
            delete (SRL::Types::SmoothMesh*)this->currentMesh;
        }

        delete this->KeyFrames;
    }
    
    /** @brief Draw current animation frame
     * @note Used only with flat type mesh data
     */
    void Draw()
    {
        if (!this->IsSmooth())
        {
            SRL::Scene3D::DrawMesh(*(SRL::Types::Mesh*)this->currentMesh);
        }
    }

    /** @brief Draw current animation frame
     * @note Used only with smooth type mesh data
     * @param light Light direction
     */
    void Draw(SRL::Types::Vector3D& light)
    {
        if (this->IsSmooth())
        {
            SRL::Scene3D::DrawSmoothMesh(*(SRL::Types::SmoothMesh*)this->currentMesh, light);
        }
    }

    /** @brief Gets number of loaded mesh faces
     * @return Number of loaded mesh faces
     */
    size_t GetFaceCount()
    {
        if (this->IsSmooth())
        {
            return ((SRL::Types::SmoothMesh*)this->currentMesh)->FaceCount;
        }
        else
        {
            return ((SRL::Types::Mesh*)this->currentMesh)->FaceCount;
        }
    }
    
    /** @brief Gets number of loaded mesh vertices
     * @return Number of loaded mesh vertices
     */
    size_t GetVertexCount()
    {
        if (this->IsSmooth())
        {
            return ((SRL::Types::SmoothMesh*)this->currentMesh)->VertexCount;
        }
        else
        {
            return ((SRL::Types::Mesh*)this->currentMesh)->VertexCount;
        }
    }

    /** @brief Get a value indicating whether we are dealing with smooth mesh
     * @return true if its a smooth mesh
     */
    bool IsSmooth()
    {
        return this->KeyFrames->IsSmooth();
    }

    /** @brief Process animation frame
     * @param time frame time, where number before the decimal place indicates current frame, and number after decimal place indicate interpolation to next frame
     */
    void ProcessFrame(SRL::Types::Fxp time)
    {
        // Totals
        size_t frames = this->KeyFrames->GetMeshCount();
        size_t faces = this->GetFaceCount();
        size_t vertices = this->GetVertexCount();

        // Retrieve only positive whole number
        size_t frame = SRL::Math::Min((size_t)SRL::Math::Max((int)time.ToInt(), 0), frames - 1);
        size_t nextFrame = SRL::Math::Min(frame + 1, frames - 1);

        // Retrieve only decimal portion
        SRL::Types::Fxp interpolation = SRL::Types::Fxp::FromRaw(time.Value() & 0x0000ffff);

        if (this->IsSmooth())
        {
            SRL::Types::SmoothMesh* current = this->KeyFrames->GetMesh<SRL::Types::SmoothMesh>(frame);
            SRL::Types::SmoothMesh* next = this->KeyFrames->GetMesh<SRL::Types::SmoothMesh>(nextFrame);

            // Interpolate vertices of a mesh
            for (size_t vertex = 0; vertex < vertices; vertex++)
            {
                InterpolatePoint(
                    &current->Vertices[vertex],
                    &next->Vertices[vertex],
                    interpolation,
                    &this->vertexArray[vertex]);
            }

            // Interpolate normals
            for (size_t face = 0; face < faces; face++)
            {
                InterpolatePoint(
                    &current->Faces[face].Normal,
                    &current->Faces[face].Normal,
                    interpolation,
                    &this->polygonArray[face].Normal);
            }
            
            // Interpolate vertex normals
            for (size_t norm = 0; norm < vertices; norm++)
            {
                InterpolatePoint(
                    &current->Normals[norm],
                    &current->Normals[norm],
                    interpolation,
                    &this->normalsArray[norm]);
            }
        }
        else
        {
            SRL::Types::Mesh* current = this->KeyFrames->GetMesh<SRL::Types::Mesh>(frame);
            SRL::Types::Mesh* next = this->KeyFrames->GetMesh<SRL::Types::Mesh>(nextFrame);

            // Interpolate vertices of a mesh
            for (size_t vertex = 0; vertex < vertices; vertex++)
            {
                InterpolatePoint(
                    &current->Vertices[vertex],
                    &next->Vertices[vertex],
                    interpolation,
                    &this->vertexArray[vertex]);
            }

            // Interpolate normals
            for (size_t face = 0; face < faces; face++)
            {
                InterpolatePoint(
                    &current->Faces[face].Normal,
                    &current->Faces[face].Normal,
                    interpolation,
                    &this->polygonArray[face].Normal);
            }
        }
    }
};
