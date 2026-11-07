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
            T magnitude = 0;
            for (i : m_values)
                magnitude += pow(i, 2);

            magnitude = sqrt(magnitude);

            return magnitude;
        }

        Vector normalize()
        {
            (*this) /= this->magnitude();
            return (*this);
        }

        Vector normalized() const
        {
            return (*this) / this->magnitude;
        }

        T at(std::size_t index) const
        {
            return m_values.at(index);
        }

        T operator[](std::size_t index) const
        {
            return m_values.at(index);
        }

        Vector add(const Vector& otherVector) const
        {
            std::array<T, size> newVals;
            
            for (std::size_t i = 0; i < newVals.size(); i++)
                newVals.at(i) = m_values.at(i) + otherVector[i];
            
            return Vector(newVals);
        }

        Vector sub(const Vector& otherVector) const
        {
            std::array<T, size> newVals;
            
            for (std::size_t i = 0; i < newVals.size(); i++)
                newVals.at(i) = m_values.at(i) - otherVector[i];
            
            return Vector(newVals);
        }

        Vector mul(const T value) const
        {
            
        }

        Vector div(const T value) const
        {

        }

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