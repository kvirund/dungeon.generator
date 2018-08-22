//
// Created by agorev on 8/22/2018.
//

#ifndef DUNGEON_GENERATOR_TIME_UTILS_HPP
#define DUNGEON_GENERATOR_TIME_UTILS_HPP

#include <chrono>

class Profiler
{
public:
	using duration_t = std::chrono::duration<long double>;

	Profiler() : m_start(std::chrono::high_resolution_clock::now()) {}

	duration_t delta() const;

	void reset() { m_start = std::chrono::high_resolution_clock::now(); }

private:
	std::chrono::high_resolution_clock::time_point m_start;
};

inline Profiler::duration_t Profiler::delta() const
{
	return std::chrono::duration_cast<duration_t>(std::chrono::high_resolution_clock::now() - m_start);
}

class ProfilerWithOutput : public Profiler
{
public:
	ProfilerWithOutput(duration_t& output) : m_output(output) {}

	~ProfilerWithOutput() { m_output = delta(); }

private:
	duration_t& m_output;
};

#endif //DUNGEON_GENERATOR_TIME_UTILS_HPP
