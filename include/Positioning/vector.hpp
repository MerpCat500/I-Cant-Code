/**
 * @file vector.hpp
 * @author Ezra Jones - ezra.jones@usu.edu
 * @brief General Vector Class to Handle Positioning
 * @version 1.0 (@LastMod : )
 * @date 2026-11-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once
#include <array>
#include <cstddef>

template <typename T, std::size_t size>
class Vector
{
    private:
        std::array<T, size> m_values;

    public:

        Vector(std::array<T, size> values)
            : m_values(values)
        {}

        T magnitude() const
        {
            
        }

        Vector normalize();
        Vector normalized() const;

        T operator[](std::size_t index);

        Vector add(const Vector& otherVector) const;
        Vector sub(const Vector& otherVector) const;
        Vector mul(const Vector& otherVector) const;
        Vector div(const Vector& otherVector) const;

        Vector operator+(const Vector& otherVector) const;
        Vector operator-(const Vector& otherVector) const;
        Vector operator*(const Vector& otherVector) const;
        Vector operator/(const Vector& otherVector) const;

        void operator+=(const Vector& otherVector);
        void operator-=(const Vector& otherVector);
        void operator*=(const Vector& otherVector);
        void operator/=(const Vector& otherVector);

        T dot(const Vector& otherVector) const;
        Vector cross(const Vector& otherVector) const;


        friend std::ostream& operator<<(std::ostream& os, const Vector& v)

};