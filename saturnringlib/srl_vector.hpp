#pragma once

#include "srl_fxp.hpp"

namespace SRL::Types
{
	/** @brief 2D vector
	 */
	struct Vector2D
	{
		/** @brief X axis component
		 */
		Fxp X;

		/** @brief Y axis component
		 */
		Fxp Y;

		/** @brief Construct a new Vector2D object
		 */
		constexpr Vector2D() : X(), Y() { }
		
		/** @brief Construct a new Vector3D object
		 * @param vector Other vector
		 */
		constexpr Vector2D(const Vector2D& vector) : Vector2D(vector.X, vector.Y) { }
		
		/** @brief Constructor to create a new 2D vector out of double values
		 * @param data Vector data (byte array of two Sint32 values)
		 */
		constexpr Vector2D(void* data) : Vector2D()
		{
			this->X = Fxp::FromRaw(((Sint32*)data)[0]);
			this->Y = Fxp::FromRaw(((Sint32*)data)[1]);
		}

		/** @brief Constructor to create a new 2D vector out of double values
		 * @param sglVector SGL vector data
		 */
		constexpr Vector2D(VECTOR sglVector) : Vector2D()
		{
			this->X = Fxp::FromRaw(sglVector[0]);
			this->Y = Fxp::FromRaw(sglVector[1]);
		}

		/** @brief Constructor to create a new 2D vector out of double values
		 * @param x X component
		 * @param y Y component
		 */
		consteval Vector2D(const double& x, const double& y) : X(x), Y(y) { }
		
		/** @brief Constructor to create a new 2D vector out of float values
		 * @param x X component
		 * @param y Y component
		 */
		consteval Vector2D(const float& x, const float& y) : X(x), Y(y) { }
		
		/** @brief Constructor to create a new 2D vector out of fixed point values
		 * @param x X component
		 * @param y Y component
		 */
		constexpr Vector2D(const Fxp& x, const Fxp& y) : X(x), Y(y) { }

		/** @brief Get approximate length of the vector
		 * @return Vector length
		 */
		virtual Fxp ApproximateLength()
		{
			Fxp x = Math::Abs(this->X);
			Fxp y = Math::Abs(this->Y);
			
			constexpr Vector2D alphaBeta =
				Vector2D
				(
					0.9398086351723256, // Alpha
					0.38928148272372454 // Beta
				);

			return Vector2D(Math::Max(x, y), Math::Min(x, y)).Dot(alphaBeta);
		}

		/** @brief Normalize vector with approximate length
		 */
		virtual void ApproximateNormalize()
		{
			Fxp length = this->ApproximateLength();

			if (length != 0.0)
			{
				this->X /= length;
				this->Y /= length;
			}
		}
		
		/** @brief Get dot product of two vectors
		 * @return Vector length
		 */
		constexpr Fxp Dot(const Vector2D& other)
		{
			return (this->X * other.X) + (this->Y * other.Y);
		}

		/** @brief Get length of the vector
		 * @return Vector length
		 */
		constexpr virtual Fxp Length()
		{
			return this->Dot(*this).Sqrt();
		}

		/** @brief Find distance to other vector
		 * @param vector Other vector
		 * @return Distance to vector
		 */
		constexpr virtual Fxp DistanceTo(const Vector2D& vector)
		{
			return (*this - vector).Length();
		}

		/** @brief Normalize vector
		 */
		constexpr virtual void Normalize()
		{
			Fxp length = this->Length();

			if (length != 0.0)
			{
				this->X /= length;
				this->Y /= length;
			}
		}
		/** @brief Indexer operator
		 * @param index Component index
		 * @return Component reference
		 */
		Fxp& operator[](size_t index)
		{
			switch (index)
			{
			case 0:
				return this->X;
			
			default:
			case 1:
				return this->Y;
			}
		}
 
 		/** @brief Multiply each coordinate by a scalar value.
		 * @param fxp The scalar value to multiply by.
		 * @return The resulting vector.
		 */
		constexpr Vector2D operator*(const Fxp& fxp) const
		{
			return Vector2D(this->X * fxp, this->Y * fxp);
		}

		/** @brief Divide each coordinate by a scalar value.
		 * @param fxp The scalar value to divide by.
		 * @return The resulting vector.
		 */
		constexpr Vector2D operator/(const Fxp& fxp) const
		{
			return Vector2D(this->X / fxp, this->Y / fxp);
		}
		
		/** @brief Check if two vectors are not equal.
		 * @param vec Vector to compare.
		 * @return True if not equal, false otherwise.
		 */
		constexpr bool operator!=(const Vector2D& vec) const
		{
			return this->X != vec.X && this->Y != vec.Y;
		}

		/** @brief Bitwise right shift operator.
		 * @param shiftAmount The number of positions to shift.
		 * @return The resulting vector.
		 */
		constexpr Vector2D operator>>(const size_t& shiftAmount)
		{
			return Vector2D(this->X >> shiftAmount, this->Y >> shiftAmount);
		}

		/** @brief Bitwise right shift assignment operator.
		 * @param shiftAmount The number of positions to shift.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector2D& operator>>=(const size_t& shiftAmount)
		{
			this->X >>= shiftAmount;
			this->Y >>= shiftAmount;
			return *this;
		}

		/** @brief Bitwise left shift operator.
		 * @param shiftAmount The number of positions to shift.
		 * @return The resulting vector.
		 */
		constexpr Vector2D operator<<(const size_t& shiftAmount)
		{
			return Vector2D(this->X << shiftAmount, this->Y << shiftAmount);
		}

		/** @brief Bitwise left shift assignment operator.
		 * @param shiftAmount The number of positions to shift.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector2D& operator<<=(const size_t& shiftAmount)
		{
			this->X <<= shiftAmount;
			this->Y <<= shiftAmount;
			return *this;
		}

		/** @brief Unary negation operator.
		 * @return A new vector with negated coordinates.
		 */
		constexpr Vector2D operator-() const
		{
			return Vector2D(-this->X, -this->Y);
		}

		/** @brief Binary addition operator.
		 * @param vec Vector to add.
		 * @return The sum as vector.
		 */
		constexpr Vector2D operator+(const Vector2D& vec) const
		{
			return Vector2D(this->X + vec.X, this->Y + vec.Y);
		}

		/** @brief Binary subtraction operator.
		 * @param vec Vector to subtract.
		 * @return The difference as vector.
		 */
		constexpr Vector2D operator-(const Vector2D& vec) const
		{
			return Vector2D(this->X - vec.X, this->Y - vec.Y);
		}

		/** @brief Compound addition assignment operator.
		 * @param vec Vector to add.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector2D operator+=(const Vector2D& vec)
		{
			this->X += vec.X;
			this->Y += vec.Y;
			return *this;
		}

		/** @brief Compound subtraction assignment operator.
		 * @param vec Vector to subtract.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector2D operator-=(const Vector2D& vec)
		{
			this->X -= vec.X;
			this->Y -= vec.Y;
			return *this;
		}
	};

	/** @brief 3D vector
	 */
	struct Vector3D : public Vector2D
	{
		/** @brief Z axis component
		 */
		Fxp Z;

		/** @brief Construct a new Vector3D object
		 */
		constexpr Vector3D() : Vector2D(), Z() { }
		
		/** @brief Construct a new Vector3D object
		 * @param vector Other vector
		 */
		constexpr Vector3D(const Vector2D& vector) : Vector2D(vector.X, vector.Y), Z() { }
		
		/** @brief Construct a new Vector3D object
		 * @param vector Other vector
		 */
		constexpr Vector3D(const Vector3D& vector) : Vector2D(vector), Z(vector.Z) { }
		
		/** @brief Constructor to create a new 3D vector out of double values
		 * @param data Vector data (byte array of three Sint32 values)
		 */
		constexpr Vector3D(void* data) : Vector3D()
		{
			this->X = Fxp::FromRaw(((Sint32*)data)[0]);
			this->Y = Fxp::FromRaw(((Sint32*)data)[1]);
			this->Z = Fxp::FromRaw(((Sint32*)data)[2]);
		}

		/** @brief Constructor to create a new 2D vector out of double values
		 * @param sglVector SGL vector data
		 */
		constexpr Vector3D(VECTOR sglVector) : Vector3D()
		{
			this->X = Fxp::FromRaw(sglVector[0]);
			this->Y = Fxp::FromRaw(sglVector[1]);
			this->Z = Fxp::FromRaw(sglVector[3]);
		}

		/** @brief Constructor to create a new 3D vector out of double values
		 * @param x X component
		 * @param y Y component
		 * @param z Z component
		 */
		consteval Vector3D(const double& x, const double& y, const double& z = 0.0) : Vector2D(x, y), Z(z) { }

		/** @brief Constructor to create a new 3D vector out of float values
		 * @param x X component
		 * @param y Y component
		 * @param z Z component
		 */
		consteval Vector3D(const float& x, const float& y, const float& z = 0.0f) : Vector2D(x, y), Z(z) { }
		
		/** @brief Constructor to create a new 3D vector out of fixed point values
		 * @param x X component
		 * @param y Y component
		 * @param z Z component
		 */
		constexpr Vector3D(const Fxp& x, const Fxp& y, const Fxp& z = 0.0) : Vector2D(x, y), Z(z) { }
		
		/** @brief Indexer operator
		 * @param index Component index
		 * @return Component reference
		 */
		Fxp& operator[](size_t index)
		{
			switch (index)
			{
			case 0:
				return this->X;
			
			case 1:
				return this->Y;

			default:
			case 2:
				return this->Z;
			}
		}
 
		/** @brief Multiply each coordinate by a scalar value.
		 * @param fxp The scalar value to multiply by.
		 * @return The resulting vector.
		 */
		constexpr Vector3D operator*(const Fxp& fxp) const
		{
			return Vector3D(this->X * fxp, this->Y * fxp, this->Z * fxp);
		}

		/** @brief Divide each coordinate by a scalar value.
		 * @param fxp The scalar value to divide by.
		 * @return The resulting vector.
		 */
		constexpr Vector3D operator/(const Fxp& fxp) const
		{
			return Vector3D(this->X / fxp, this->Y / fxp, this->Z / fxp);
		}
		
		/** @brief Check if two vectors are not equal.
		 * @param vec Vector to compare.
		 * @return True if not equal, false otherwise.
		 */
		constexpr bool operator!=(const Vector2D& vec) const
		{
			return this->X != vec.X && this->Y != vec.Y && this->Z != 0.0;
		}

		/** @brief Check if two vectors are not equal.
		 * @param vec Vector to compare.
		 * @return True if not equal, false otherwise.
		 */
		constexpr bool operator!=(const Vector3D& vec) const
		{
			return this->X != vec.X && this->Y != vec.Y && this->Z != vec.Z;
		}

		/** @brief Bitwise right shift operator.
		 * @param shiftAmount The number of positions to shift.
		 * @return The resulting vector.
		 */
		constexpr Vector3D operator>>(const size_t& shiftAmount)
		{
			return Vector3D(this->X >> shiftAmount, this->Y >> shiftAmount, this->Z >> shiftAmount);
		}

		/** @brief Bitwise right shift assignment operator.
		 * @param shiftAmount The number of positions to shift.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector3D& operator>>=(const size_t& shiftAmount)
		{
			this->X >>= shiftAmount;
			this->Y >>= shiftAmount;
			this->Z >>= shiftAmount;
			return *this;
		}

		/** @brief Bitwise left shift operator.
		 * @param shiftAmount The number of positions to shift.
		 * @return The resulting vector.
		 */
		constexpr Vector3D operator<<(const size_t& shiftAmount)
		{
			return Vector3D(this->X << shiftAmount, this->Y << shiftAmount, this->Z << shiftAmount);
		}

		/** @brief Bitwise left shift assignment operator.
		 * @param shiftAmount The number of positions to shift.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector3D& operator<<=(const size_t& shiftAmount)
		{
			this->X <<= shiftAmount;
			this->Y <<= shiftAmount;
			this->Z <<= shiftAmount;
			return *this;
		}

		/** @brief Unary negation operator.
		 * @return A new vector with negated coordinates.
		 */
		constexpr Vector3D operator-() const
		{
			return Vector3D(-this->X, -this->Y, -this->Z);
		}

		/** @brief Binary addition operator.
		 * @param vec Vector to add.
		 * @return The sum as vector.
		 */
		constexpr Vector3D operator+(const Vector2D& vec) const
		{
			return Vector3D(this->X + vec.X, this->Y + vec.Y, this->Z);
		}

		/** @brief Binary addition operator.
		 * @param vec Vector to add.
		 * @return The sum as vector.
		 */
		constexpr Vector3D operator+(const Vector3D& vec) const
		{
			return Vector3D(this->X + vec.X, this->Y + vec.Y, this->Z + vec.Z);
		}

		/** @brief Binary subtraction operator.
		 * @param vec Vector to subtract.
		 * @return The difference as vector.
		 */
		constexpr Vector3D operator-(const Vector2D& vec) const
		{
			return Vector3D(this->X - vec.X, this->Y - vec.Y, this->Z);
		}

		/** @brief Binary subtraction operator.
		 * @param vec Vector to subtract.
		 * @return The difference as vector.
		 */
		constexpr Vector3D operator-(const Vector3D& vec) const
		{
			return Vector3D(this->X - vec.X, this->Y - vec.Y, this->Z - vec.Z);
		}

		/** @brief Compound addition assignment operator.
		 * @param vec Vector to add.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector3D operator+=(const Vector2D& vec)
		{
			this->X += vec.X;
			this->Y += vec.Y;
			return *this;
		}

		/** @brief Compound addition assignment operator.
		 * @param vec Vector to add.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector3D operator+=(const Vector3D& vec)
		{
			this->X += vec.X;
			this->Y += vec.Y;
			this->Z += vec.Z;
			return *this;
		}

		/** @brief Compound subtraction assignment operator.
		 * @param vec Vector to subtract.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector3D operator-=(const Vector2D& vec)
		{
			this->X -= vec.X;
			this->Y -= vec.Y;
			return *this;
		}

		/** @brief Compound subtraction assignment operator.
		 * @param vec Vector to subtract.
		 * @return Reference to the modified vector.
		 */
		constexpr Vector3D operator-=(const Vector3D& vec)
		{
			this->X -= vec.X;
			this->Y -= vec.Y;
			this->Z -= vec.Z;
			return *this;
		}
		/** @brief Get approximate length of the vector
		 * @return Vector length
		 */
		constexpr Fxp ApproximateLength() override
		{
			Fxp x = Math::Abs(this->X);
			Fxp y = Math::Abs(this->Y);
			Fxp z = Math::Abs(this->Z);
		
			Fxp max = Math::Max(Math::Max(x, y), z);
			Fxp min = Math::Min(Math::Min(x, y), z);
			Fxp mid = (y < x) ? ((y < z) ? ((z < x) ? z : x) : y) : ((x < z) ? ((z < y) ? z : y) : x);

			constexpr Vector3D alphaBetaGama =
				Vector3D
				(
					0.9398086351723256, // Alpha
					0.38928148272372454, // Beta
					0.2987061876143797 // Gama
				);

			return Vector3D(max, mid, min).Dot(alphaBetaGama);
		}

		/** @brief Normalize vector with approximate length
		 */
		constexpr void ApproximateNormalize() override
		{
			Fxp length = this->ApproximateLength();

			if (length != 0.0)
			{
				this->X /= length;
				this->Y /= length;
				this->Z /= length;
			}
		}

		/** @brief Calculate the cross product of this object and another Vec3 object.
		 * @param vec The Vec3 object to calculate the cross product with.
		 * @return The cross product as an Vec3 object.
		 */
		constexpr Vector3D Cross(const Vector3D& vec) const
		{
			return Vector3D(this->Z * vec.Y - this->Y * vec.Z,
				this->X * vec.Z - this->Z * vec.X,
				this->Y * vec.X - this->X * vec.Y);
		}

		/** @brief Get dot product of two vectors
		 * @return Vector length
		 */
		constexpr Fxp Dot(const Vector3D& other)
		{
			return (this->X * other.X) + (this->Y * other.Y) + (this->Z * other.Z);
		}

		/** @brief Find distance to other vector
		 * @param vector Other vector
		 * @return Distance to vector
		 */
		constexpr virtual Fxp DistanceTo(const Vector3D& vector)
		{
			return (*this - vector).Length();
		}
		
		/** @brief Normalize vector
		 */
		constexpr void Normalize() override
		{
			Fxp length = this->Length();

			if (length != 0.0)
			{
				this->X /= length;
				this->Y /= length;
				this->Z /= length;
			}
		}
	};
}