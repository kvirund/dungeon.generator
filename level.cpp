#include "level.hpp"

void Level::generate(std::size_t w, std::size_t h)
{
	m_field = std::move(field_t(w, column_t(h, CT_VOID)));

	m_width = w;
	m_height = h;

	if (0 < width() && 0 < height())
	{
		auto count = w + h;
		for (auto i = 0; i != count; ++i)
		{
			m_field[rand() % width()][rand() % height()] = CT_CONCRETE;
		}
	}
}
