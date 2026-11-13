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
#include <ostream>
#include <cmath>
#include <algorithm>
#include <initializer_list>

template <typename T, std::size_t size>
class Vector
{
    private:
        std::array<T, size> m_values{};

    public:

        Vector() = default;

        Vector(std::array<T, size> values)
            : m_values(values)
        {}

        Vector(std::initializer_list<T> list) 
        {
            std::copy_n(list.begin(), std::min(size, list.size()), m_values.begin());
        }

        T magnitude() const
        {
            T magnitude = 0;
            for (T i : m_values)
                magnitude += (i * i);

            magnitude = sqrt(magnitude);

            return magnitude;
        }

        Vector& normalize()
        {
            (*this) /= this->magnitude();
            return (*this);
        }

        Vector normalized() const
        {
            return (*this) / this->magnitude();
        }

        T at(std::size_t index) const
        {
            return m_values.at(index);
        }

        T operator[](std::size_t index) const
        {
            return m_values.at(index);
        }

        void set(std::size_t index, T value)
        {
            m_values.at(index) = value;
        }

        T& operator[](std::size_t index)
        {
            return m_values.at(index);
        }

        std::size_t dimension() const
        {
            return m_values.size();
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
            std::array<T, size> newVals;
            
            for (std::size_t i = 0; i < newVals.size(); i++)
                newVals.at(i) = m_values.at(i) * value;
            
            return Vector(newVals);
        }

        Vector div(const T value) const
        {
            if (value == 0)
                return Vector();

            std::array<T, size> newVals;
            
            for (std::size_t i = 0; i < newVals.size(); i++)
                newVals.at(i) = m_values.at(i) / value;
            
            return Vector(newVals);
        }

        Vector operator+(const Vector& otherVector) const
        {
            return this->add(otherVector);
        }

        Vector operator-(const Vector& otherVector) const
        {
            return this->sub(otherVector);
        }

        Vector operator*(const T value) const
        {
            return this->mul(value);
        }

        Vector operator/(const T value) const
        {
            return this->div(value);
        }

        void operator+=(const Vector& otherVector)
        {
            (*this) = this->add(otherVector);
        }

        void operator-=(const Vector& otherVector)
        {
            (*this) = this->sub(otherVector);
        }

        void operator*=(const T value)
        {
            (*this) = this->mul(value);
        }

        void operator/=(const T value)
        {
            (*this) = this->div(value);
        }

        T dot(const Vector& otherVector) const
        {
            T productValue = 0;

            for (std::size_t i = 0; i < m_values.size(); i++)
                productValue += (m_values[i] * otherVector[i]);

            return productValue;
        }

        Vector projected(const Vector& otherVector) const
        {
            if (otherVector.dot(otherVector) == 0)
                return Vector();

            
            T coefficient = (this->dot(otherVector)) / (otherVector.dot(otherVector));

            return (otherVector * coefficient);
        }

        Vector& project(const Vector& otherVector)
        {
            (*this) = this->projected(otherVector);
            return (*this);
        }

        friend std::ostream& operator<<(std::ostream& os, const Vector& v)
        {
            for (std::size_t i = 0; i < v.dimension(); i++)
            {
                os << v[i];

                if (i != (v.dimension() - 1))
                    os << ", ";
            }

            return os;
        }

        friend Vector operator*(const T value, const Vector& v)
        {
            return v * value;
        }

};