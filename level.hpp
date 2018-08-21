#ifndef DUNGEON_GENERATOR_LEVEL_H
#define DUNGEON_GENERATOR_LEVEL_H

#include <vector>

class Level
{
public:
	enum CellType
	{
		CT_VOID,
		CT_CONCRETE
	};

	Level(): m_width(0), m_height(0) {}

	void generate(std::size_t w, std::size_t h);

	auto width() const { return m_width; }
	auto height() const { return m_height; }
	auto cell(std::size_t x, std::size_t y) const { return m_field[x][y]; }

private:
	using column_t = std::vector<CellType>;
	using field_t = std::vector<column_t>;

	field_t m_field;
	std::size_t m_width;
	std::size_t m_height;
};

#endif //DUNGEON_GENERATOR_LEVEL_H
