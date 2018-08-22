//
// Created by agorev on 8/22/2018.
//

#include "fractal.hpp"

AbstractJuliaSet::AbstractJuliaSet(int w, int h) : m_points(w, column_t(h)),
		m_current_order(0),
		m_width(w),
		m_height(h),
		m_advanced(1)
{
}

void AbstractJuliaSet::init_fractal()
{
	for (auto y = 0; y != m_height; ++y)
	{
		for (auto x = 0; x != m_width; ++x)
		{
			m_points[x][y].c = c(x, y);
			m_points[x][y].z = z(x, y);
			m_points[x][y].order = m_current_order;
		}
	}
}

int AbstractJuliaSet::order(int x, int y) const { return m_points[x][y].order; }

int AbstractJuliaSet::max_order() const { return m_current_order; }

void AbstractJuliaSet::step()
{
	if (0 == m_advanced)
	{
		// all remaining points are divergent
		return;
	}

	constexpr auto BATCH_SIZE = 12;

	m_advanced = 0;
	const auto STEPS = std::min(BATCH_SIZE, MAX_ORDER - m_current_order);
	for (auto y = 0; y != m_height; ++y)
	{
		for (auto x = 0; x != m_width; ++x)
		{
			if (m_points[x][y].order == m_current_order)
			{
				auto i = 0;
				auto z = m_points[x][y].z * m_points[x][y].z + m_points[x][y].c;

				while (i != STEPS
						&& 2 >= std::abs(z))
				{
					m_points[x][y].z = z;
					++i;
					z = z * z + m_points[x][y].c;
					if (z == m_points[x][y].z)
					{
						i = 0;
						m_points[x][y].order = MAX_ORDER;
						break;
					}
				}

				if (0 != i)
				{
					m_points[x][y].order += i;
					++m_advanced;
				}
			}
		}
	}

	m_current_order += STEPS;
}

bool AbstractJuliaSet::done() const { return MAX_ORDER == m_current_order || 0 == m_advanced; }

MandelbrotFractal::MandelbrotFractal(int w, int h) : AbstractJuliaSet(w, h)
{
	init_fractal();
}

AbstractJuliaSet::complex_t MandelbrotFractal::c(int x, int y) const
{
	return {3 * static_cast<double>(x) / width() - 2.0, 2 * static_cast<double>(y) / height() - 1.0};
}

AbstractJuliaSet::complex_t MandelbrotFractal::z(int, int) const { return {0, 0}; }

JuliaSet::JuliaSet(int w, int h, const AbstractJuliaSet::complex_t& c) : AbstractJuliaSet(w, h), m_c(c)
{
	init_fractal();
}

AbstractJuliaSet::complex_t JuliaSet::c(int, int) const { return m_c; }

AbstractJuliaSet::complex_t JuliaSet::z(int x, int y) const
{
	return {4 * static_cast<double>(x) / width() - 2.0, 4 * static_cast<double>(y) / height() - 2.0};
}

Fractal::unique_ptr FractalsFactory::create_mandelbrot_fractal(int w, int h)
{
	return std::move(std::make_unique<MandelbrotFractal>(w, h));
}

Fractal::unique_ptr FractalsFactory::create_julia_set(int w, int h, AbstractJuliaSet::complex_t& c)
{
	return std::move(std::make_unique<JuliaSet>(w, h, c));
}

Fractal::unique_ptr FractalsFactory::create_julia_set(int w, int h, double alpha)
{
	return std::move(std::make_unique<JuliaSet>(w, h, AbstractJuliaSet::complex_t{std::cos(alpha), std::sin(alpha)}));
}