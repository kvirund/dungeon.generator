//
// Created by agorev on 8/22/2018.
//

#ifndef DUNGEON_GENERATOR_FRACTAL_HPP
#define DUNGEON_GENERATOR_FRACTAL_HPP

#include <memory>
#include <complex>
#include <vector>
#include <algorithm>

class Fractal
{
public:
	using unique_ptr = std::unique_ptr<Fractal>;

	virtual ~Fractal() = default;

	virtual int order(int x, int y) const = 0;
	virtual int max_order() const = 0;
	virtual bool done() const = 0;
	virtual void step() = 0;
};

class AbstractJuliaSet : public Fractal
{
public:
	using complex_t = std::complex<double>;

	AbstractJuliaSet(int w, int h);

	int order(int x, int y) const override;
	int max_order() const override;
	bool done() const override;
	void step() override;

protected:
	static constexpr auto MAX_ORDER = 1000;

	using point_t = struct
	{
		complex_t z;
		complex_t c;
		int order;
	};
	using column_t = std::vector<point_t>;

	virtual complex_t z(int x, int y) const = 0;
	virtual complex_t c(int x, int y) const = 0;

	int width() const { return m_width; }

	int height() const { return m_height; }

	void init_fractal();

private:
	std::vector <column_t> m_points;
	int m_current_order;
	int m_width;
	int m_height;
	int m_advanced;
};

class MandelbrotFractal : public AbstractJuliaSet
{
public:
	MandelbrotFractal(int w, int h);

private:
	complex_t c(int x, int y) const override;
	complex_t z(int x, int y) const override;
};

class JuliaSet : public AbstractJuliaSet
{
public:
	JuliaSet(int w, int h, const AbstractJuliaSet::complex_t& c);

private:
	complex_t c(int x, int y) const override;
	complex_t z(int x, int y) const override;

	complex_t m_c;
};

class FractalsFactory
{
public:
	static Fractal::unique_ptr create_mandelbrot_fractal(int w, int h);
	static Fractal::unique_ptr create_julia_set(int w, int h, AbstractJuliaSet::complex_t& c);
	static Fractal::unique_ptr create_julia_set(int w, int h, double alpha);
};

#endif //DUNGEON_GENERATOR_FRACTAL_HPP
