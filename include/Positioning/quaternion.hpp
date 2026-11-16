/**
 * @file quaternion.hpp
 * @author Ezra Jones - ezra.jones@usu.edu
 * @brief General Quaternion Class to Handle 3D Rotations
 * @version 1.0 (@LastMod : )
 * @date 2026-11-13
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#pragma once

#include "vector.hpp"
#include "Util/angle.hpp"
#include <cmath>

template <typename T>
class Quaternion
{
    private:

        T w, x, y, z;

    public:

        Quaternion(T w, T x, T y, T z)
            : w(w), x(x), y(y), z(z)
        {}

        Quaternion(Vector<T, 3> axis, Angle<T> angle)
        {
            w = std::cos(angle.getTheta());
            axis.normalize();
            x = std::sin(angle.getTheta()) * axis.at(0);
            y = std::sin(angle.getTheta()) * axis.at(1);
            z = std::sin(angle.getTheta()) * axis.at(2);
        }

        Quaternion operator*(Quaternion otherQuaternion){

        }

};