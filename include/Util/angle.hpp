/**
 * @file angle.hpp
 * @author Ezra Jones - ezra.jones@usu.edu
 * @brief Angle class for handling rotations and units
 * @version 1.0 (@LastMod : )
 * @date 2026-11-15
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <cmath>
#include <numbers>

template <typename T>
class Angle
{
    public:
        static constexpr T PI = std::numbers::pi_v<T>;
        static constexpr T TWO_PI = T{2} * std::numbers::pi_v<T>;
        static constexpr T DEG_TO_RAD = (PI / T{180});
        static constexpr T RAD_TO_DEG = (T{180} / PI);

    private:

        T m_theta = T{0};


    public:

        Angle() = default;
        
        explicit Angle(T theta, bool isRadians = true)
        {
            m_theta = (isRadians) ? theta : (theta * DEG_TO_RAD);
        }

        T getTheta(bool isRadians = true) const
        {
            return (isRadians) ? m_theta : (m_theta * RAD_TO_DEG);
        }

        Angle& setTheta(T theta, bool isRadians = true)
        {
            m_theta = (isRadians) ? theta : (theta * DEG_TO_RAD);
            return (*this);
        }

        Angle wrappedAngle() const
        {
            T wrapped = std::fmod(m_theta + PI, TWO_PI);

            if (wrapped < T{0}) {
                wrapped += TWO_PI;
            }

            return Angle(wrapped - PI);
        }

        Angle& wrapAngle()
        {
            m_theta = wrappedAngle().getTheta();
            return (*this);
        }
};