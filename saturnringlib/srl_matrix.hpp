#pragma once

#include "srl_vector.hpp"

namespace SRL::Types
{
    /** @brief Transformation matrix
     */
    struct Matrix : public SRL::SGL::SglType<Matrix, MATRIX>
    {
        /** @brief Matrix table
         */
        Fxp Table[4][3];

        /** @brief Construct a new identity Matrix
         * @param identity Indicates whether to initialize matrix if set to true
         */
        constexpr Matrix(const bool identity = true)
        {
            if (__builtin_expect(identity, true))
            {
                this->Table[X][X] = 1.0;
                this->Table[Y][X] = 0.0;
                this->Table[Z][X] = 0.0;
                this->Table[XYZ][X] = 0.0;
                this->Table[X][Y] = 0.0;
                this->Table[Y][Y] = 1.0;
                this->Table[Z][Y] = 0.0;
                this->Table[XYZ][Y] = 0.0;
                this->Table[X][Z] = 0.0;
                this->Table[Y][Z] = 0.0;
                this->Table[Z][Z] = 1.0;
                this->Table[XYZ][Z] = 0.0;
            }
        }

        /** @brief Construct a new identity Matrix with specified origin point
         * @param origin Origin point
         */
        constexpr Matrix(const Vector3D& origin)
        {
            this->Table[X][X] = 1.0;
            this->Table[Y][X] = 0.0;
            this->Table[Z][X] = 0.0;
            this->Table[XYZ][X] = origin.X;
            this->Table[X][Y] = 0.0;
            this->Table[Y][Y] = 1.0;
            this->Table[Z][Y] = 0.0;
            this->Table[XYZ][Y] = origin.Y;
            this->Table[X][Z] = 0.0;
            this->Table[Y][Z] = 0.0;
            this->Table[Z][Z] = 1.0;
            this->Table[XYZ][Z] = origin.Z;
        }

        /** @brief Construct a new Matrix from origin and two vectors
         * @param origin Origin point
         * @param axisX X axis
         * @param axisY Y axis
         */
        constexpr Matrix(const Vector3D& origin, const Vector3D& axisX, const Vector3D& axisY)
        {
            // Get cross of direction and up vector
            Vector3D normal = axisX.Cross(axisY);

            this->Table[X][X] = axisX.X;
            this->Table[Y][X] = axisX.Y;
            this->Table[Z][X] = axisX.Z;
            this->Table[XYZ][X] = origin.X;
            this->Table[X][Y] = axisY.X;
            this->Table[Y][Y] = axisY.Y;
            this->Table[Z][Y] = axisY.Z;
            this->Table[XYZ][Y] = origin.Y;
            this->Table[X][Z] = normal.X;
            this->Table[Y][Z] = normal.Y;
            this->Table[Z][Z] = normal.Z;
            this->Table[XYZ][Z] = origin.Z;
        }
        
        /** @brief Construct a new Matrix from origin and three vectors
         * @param origin Origin point
         * @param axisX X axis
         * @param axisY Y axis
         * @param axisZ Z axis
         */
        constexpr Matrix(const Vector3D& origin, const Vector3D& axisX, const Vector3D& axisY, const Vector3D& axisZ)
        {
            this->Table[X][X] = axisX.X;
            this->Table[Y][X] = axisX.Y;
            this->Table[Z][X] = axisX.Z;
            this->Table[XYZ][X] = origin.X;
            this->Table[X][Y] = axisY.X;
            this->Table[Y][Y] = axisY.Y;
            this->Table[Z][Y] = axisY.Z;
            this->Table[XYZ][Y] = origin.Y;
            this->Table[X][Z] = axisZ.X;
            this->Table[Y][Z] = axisZ.Y;
            this->Table[Z][Z] = axisZ.Z;
            this->Table[XYZ][Z] = origin.Z;
        }
        
        /** @brief Get X axis vector
         * @return X axis vector
         */
        constexpr Vector3D GetAxisX()
        {
            return Vector3D(this->Table[0][X], this->Table[1][X], this->Table[2][X]);
        }

        /** @brief Get Y axis vector
         * @return Y axis vector
         */
        constexpr Vector3D GetAxisY()
        {
            return Vector3D(this->Table[0][Y], this->Table[1][Y], this->Table[2][Y]);
        }

        /** @brief Get Z axis vector
         * @return Z axis vector
         */
        constexpr Vector3D GetAxisZ()
        {
            return Vector3D(this->Table[0][Z], this->Table[1][Z], this->Table[2][Z]);
        }

        /** @brief Get origin point
         * @return Origin point
         */
        constexpr Vector3D GetOrigin()
        {
            return Vector3D(this->Table[XYZ][X], this->Table[XYZ][Y], this->Table[XYZ][Z]);
        }

        /** @brief Set X axis vector
         * @param axis Axis vector
         */
        void SetAxisX(const Vector3D& axis)
        {
            this->Table[X][X] = axis.X;
            this->Table[Y][X] = axis.Y;
            this->Table[Z][X] = axis.Z;
        }

        /** @brief Set Y axis vector
         * @param axis Axis vector
         */
        void SetAxisY(const Vector3D& axis)
        {
            this->Table[X][Y] = axis.X;
            this->Table[Y][Y] = axis.Y;
            this->Table[Z][Y] = axis.Z;
        }

        /** @brief SGet Z axis vector
         * @param axis Axis vector
         */
        void SetAxisZ(const Vector3D& axis)
        {
            this->Table[X][Z] = axis.X;
            this->Table[Y][Z] = axis.Y;
            this->Table[Z][Z] = axis.Z;
        }

        /** @brief Get origin point
         * @param point Origin point
         */
        void SetOrigin(const Vector3D& point)
        {
            this->Table[XYZ][X] = point.X;
            this->Table[XYZ][Y] = point.Y;
            this->Table[XYZ][Z] = point.Z;
        }

        /** @brief Convert matrix into an unit matrix
         */
        void Normalize()
        {
            slUnitMatrix((FIXED(*)[3])this->SglPtr());
        }
    };
}