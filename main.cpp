#include "level.hpp"

#include <SDL.h>
#include <SDL_video.h>
#include <SDL_ttf.h>

#include <vector>
#include <sstream>
#include <functional>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>
#include <list>
#include <mutex>
#include <future>
#include <complex>

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

Profiler::duration_t Profiler::delta() const
{
	return std::chrono::duration_cast<duration_t>(std::chrono::high_resolution_clock::now() - m_start);
}

class ProfilerWithOutput : public Profiler
{
public:
	ProfilerWithOutput(duration_t &output) : m_output(output) {}

	~ProfilerWithOutput() { m_output = delta(); }

private:
	duration_t &m_output;
};

class SDLApplication
{
public:
	SDLApplication();
	~SDLApplication();

	int run();

private:
	static int SDLCALL watch(void *user_data, SDL_Event *event);

	int loop();
	void render();
	void limit_fps(const Profiler::duration_t &duration, const double desired_fps = 26.0) const;
	void print_stats() const;
	void update();
	void draw_level() const;
	void draw_thread();
	void print_off();
	void measure_average_frame_processing_time();

	SDL_Window *m_window;
	SDL_Renderer *m_renderer;
	TTF_Font *m_font;

	long double m_average_frame_processing_time;
	Profiler m_frames_profiler;

	std::atomic<bool> m_quitting;
	bool m_limit_fps;
	bool m_print_fps;

	Level m_level;
	double m_left_offset;
	double m_top_offset;

	double m_moving_x;
	double m_moving_y;

	double m_zoom_factor;

	bool m_dragging;
	SDL_Point m_last_mouse_position;

	SDL_Texture *m_to_print_off;
	SDL_Surface *m_to_paint_on;
	std::mutex m_renderer_mutex;
};

SDLApplication::SDLApplication() : m_window(nullptr),
		m_renderer(nullptr),
		m_font(nullptr),
		m_average_frame_processing_time(0.0),
		m_quitting(false),
		m_limit_fps(true),
		m_print_fps(true),
		m_top_offset(0),
		m_left_offset(0),
		m_moving_x(0),
		m_moving_y(0),
		m_zoom_factor(12),
		m_dragging(false),
		m_last_mouse_position{0, 0},
		m_to_print_off(nullptr),
		m_to_paint_on(nullptr)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
	{
		std::stringstream ss;
		ss << "Failed to initialize SDL: " << SDL_GetError();
		SDL_Log("%s", ss.str().c_str());
		throw std::runtime_error(ss.str());
	}

	if (-1 == TTF_Init())
	{
		std::stringstream ss;
		ss << "TTF_Init error: " << TTF_GetError();
		SDL_Log("%s", ss.str().c_str());
		throw std::runtime_error(ss.str());
	}

	m_font = TTF_OpenFont("arial.ttf", 12);
	if (nullptr == m_font)
	{
		std::stringstream ss;
		ss << "Couldn't load font: " << TTF_GetError();
		SDL_Log("%s", ss.str().c_str());
		throw std::runtime_error(ss.str());
	}

	m_level.generate(80, 25);
}

SDLApplication::~SDLApplication()
{
	if (nullptr != m_to_print_off)
	{
		SDL_DestroyTexture(m_to_print_off);
	}

	if (nullptr != m_to_paint_on)
	{
		SDL_FreeSurface(m_to_paint_on);
	}

	TTF_Quit();
	SDL_DelEventWatch(&SDLApplication::watch, this);

	if (nullptr != m_renderer)
	{
		SDL_DestroyRenderer(m_renderer);
	}

	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

int SDLApplication::run()
{
	SDL_DisplayMode current;
	SDL_GetCurrentDisplayMode(0, &current);

	m_window = SDL_CreateWindow("Dungeon generator",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			current.w,
			current.h,
			SDL_WINDOW_FULLSCREEN);
	m_renderer = SDL_CreateRenderer(m_window, -1, 0);

	if (nullptr == m_renderer)
	{
		std::stringstream ss;
		ss << "Couldn't create windows rendered: " << SDL_GetError();
		SDL_Log("%s", ss.str().c_str());

		return 1;
	}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	const auto rmask = 0xff000000;
	const auto gmask = 0x00ff0000;
	const auto bmask = 0x0000ff00;
	const auto amask = 0x000000ff;
#else
	const auto rmask = 0x000000ff;
	const auto gmask = 0x0000ff00;
	const auto bmask = 0x00ff0000;
	const auto amask = 0xff000000;
#endif

	m_to_paint_on = SDL_CreateRGBSurface(0, current.w, current.h, 32, rmask, gmask, bmask, amask);
	if (nullptr == m_to_paint_on)
	{
		std::stringstream ss;
		ss << "Couldn't create surface to paint on: " << SDL_GetError();
		SDL_Log("%s", ss.str().c_str());

		return 1;
	}

	SDL_AddEventWatch(&SDLApplication::watch, this);

	return loop();
}

int SDLCALL SDLApplication::watch(void *user_data, SDL_Event *event)
{
	constexpr auto MOVE_COEFFICIENT = 12.0;

	SDLApplication *application = reinterpret_cast<SDLApplication *>(user_data);
	switch (event->type)
	{
	case SDL_KEYUP:
		switch (event->key.keysym.scancode)
		{
		case SDL_SCANCODE_ESCAPE:
			application->m_quitting = true;
			break;

		case SDL_SCANCODE_A:
		case SDL_SCANCODE_LEFT:
		case SDL_SCANCODE_D:
		case SDL_SCANCODE_RIGHT:
		case SDL_SCANCODE_W:
		case SDL_SCANCODE_UP:
		case SDL_SCANCODE_S:
		case SDL_SCANCODE_DOWN:
			application->m_moving_x = 0;
			application->m_moving_y = 0;
			break;

		default:
			break;
		}
		break;

	case SDL_KEYDOWN:
		switch (event->key.keysym.scancode)
		{
		case SDL_SCANCODE_A:
		case SDL_SCANCODE_LEFT:
			application->m_moving_x = -MOVE_COEFFICIENT;
			break;

		case SDL_SCANCODE_D:
		case SDL_SCANCODE_RIGHT:
			application->m_moving_x = MOVE_COEFFICIENT;
			break;

		case SDL_SCANCODE_W:
		case SDL_SCANCODE_UP:
			application->m_moving_y = -MOVE_COEFFICIENT;
			break;

		case SDL_SCANCODE_S:
		case SDL_SCANCODE_DOWN:
			application->m_moving_y = MOVE_COEFFICIENT;
			break;

		default:
			break;
		}
		break;

	case SDL_MOUSEWHEEL:
	{
		const auto ZOOM_CHANGE_FACTOR = 1.0 + .3 / application->m_zoom_factor;

		constexpr auto MAX_ZOOM = 100.0;
		constexpr auto MIN_ZOOM = 1.0;
		if (0 < event->wheel.y
				&& application->m_zoom_factor < MAX_ZOOM)
		{
			application->m_zoom_factor *= std::pow(ZOOM_CHANGE_FACTOR, event->wheel.y);
			if (application->m_zoom_factor > MAX_ZOOM)
			{
				application->m_zoom_factor = MAX_ZOOM;
			}
		}
		else if (0 > event->wheel.y
				&& application->m_zoom_factor > MIN_ZOOM)
		{
			application->m_zoom_factor *= std::pow(ZOOM_CHANGE_FACTOR, event->wheel.y);
			if (application->m_zoom_factor < MIN_ZOOM)
			{
				application->m_zoom_factor = MIN_ZOOM;
			}
		}

		break;
	}

	default:
		break;
	}

	return 1;
}

int SDLApplication::loop()
{
	std::thread draw_thread(&SDLApplication::draw_thread, this);

	int result = 0;
	while (!m_quitting)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				m_quitting = true;
				result = 1;
			}
		}

		update();

		render();

		measure_average_frame_processing_time();
	}

	draw_thread.join();

	return result;
}

void SDLApplication::measure_average_frame_processing_time()
{
	const auto last_frame_processing = m_frames_profiler.delta().count();
	constexpr long double alpha = 0.5;
	m_average_frame_processing_time = alpha * m_average_frame_processing_time + (1.0 - alpha) * last_frame_processing;
	m_frames_profiler.reset();
}

void SDLApplication::render()
{
	Profiler::duration_t duration;
	{
		ProfilerWithOutput profiler(duration);
		std::lock_guard<std::mutex> guard(m_renderer_mutex);

		constexpr SDL_Color background = {0, 0, 0, 0};
		SDL_SetRenderDrawColor(m_renderer, background.r, background.g, background.b, background.a);
		SDL_RenderClear(m_renderer);

		if (false)
		{
			draw_level();
		}

		if (nullptr != m_to_print_off)
		{
			SDL_RenderCopy(m_renderer, m_to_print_off, nullptr, nullptr);
		}

		if (m_print_fps)
		{
			print_stats();
		}

		SDL_RenderPresent(m_renderer);
	}

	if (m_limit_fps)
	{
		limit_fps(duration);
	}
}

void SDLApplication::draw_level() const
{
	const auto left_offset = static_cast<int>(m_left_offset);
	const auto top_offset = static_cast<int>(m_top_offset);
	constexpr SDL_Color cell_colors[] = {
			{12, 12,  12, 0},
			{0,  128, 0,  0}
	};
	for (auto x = 0; x != m_level.width(); ++x)
	{
		for (auto y = 0; y != m_level.height(); ++y)
		{
			SDL_Rect rect = {
					static_cast<int>(ceil(left_offset + x * m_zoom_factor)),
					static_cast<int>(ceil(top_offset + y * m_zoom_factor)),
					static_cast<int>(ceil(m_zoom_factor)),
					static_cast<int>(ceil(m_zoom_factor))};
			const auto &color = cell_colors[static_cast<int>(m_level.cell(x, y))];
			SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
			SDL_RenderFillRect(m_renderer, &rect);
		}
	}
}

void SDLApplication::print_stats() const
{
	std::list<std::string> strings;
	std::stringstream ss;
	const auto fps = 0.0 == m_average_frame_processing_time ? -1.0 : (1.0 / m_average_frame_processing_time);
	ss << "FPS: " << std::fixed << std::setprecision(2) << fps;
	strings.push_back(ss.str());

	ss.str("");
	ss << "left offset: " << m_left_offset;
	strings.push_back(ss.str());

	ss.str("");
	ss << "top offset: " << m_top_offset;
	strings.push_back(ss.str());

	ss.str("");
	ss << "zoom factor: " << m_zoom_factor;
	strings.push_back(ss.str());

	constexpr SDL_Color color = {255, 255, 255};

	SDL_Rect dest_rect = {12, 12, 0, 0};
	for (const auto &string: strings)
	{
		SDL_Surface *surface = TTF_RenderText_Solid(m_font, string.c_str(), color);
		SDL_Texture *texture = SDL_CreateTextureFromSurface(m_renderer, surface);

		SDL_QueryTexture(texture, nullptr, nullptr, &dest_rect.w, &dest_rect.h);

		SDL_RenderCopy(m_renderer, texture, nullptr, &dest_rect);
		dest_rect.y += dest_rect.h;

		SDL_DestroyTexture(texture);
		SDL_FreeSurface(surface);
	}
}

void SDLApplication::limit_fps(const Profiler::duration_t &duration, const double desired_fps) const
{
	const auto period = std::chrono::duration<long double>(1.0 / desired_fps);
	if (period > duration)
	{
		const auto sleep_period = period - duration;
		/*SDL_Log("sleeping for %.6f seconds; last render step duration: %.6f; period: %.6f",
				sleep_period.count(),
				duration.count(),
				period.count());/**/
		std::this_thread::sleep_for(sleep_period);
	}
}

void SDLApplication::update()
{
	const auto time_passed = m_frames_profiler.delta().count();
	m_left_offset += m_moving_x * time_passed;
	m_top_offset += m_moving_y * time_passed;

	int delta_x = 0;
	int delta_y = 0;
	const auto buttons = SDL_GetRelativeMouseState(&delta_x, &delta_y);
	if (SDL_BUTTON(SDL_BUTTON_LEFT) & buttons)    // dragging screen
	{
		m_left_offset += delta_x;
		m_top_offset += delta_y;
	}
}

void SDLApplication::draw_thread()
{
	using complex_t = std::complex<double>;
	using point_t = struct
	{
		complex_t z;
		complex_t c;
		int order;
	};
	using column_t = std::vector<point_t>;
	std::vector<column_t> points(m_to_paint_on->w, column_t(m_to_paint_on->h));

	constexpr auto MAX_ORDER = 1000;
	int current_order = 0;
	SDL_LockSurface(m_to_paint_on);
	std::size_t offset = 0;
	for (auto y = 0; y != m_to_paint_on->h; ++y)
	{
		for (auto x = 0; x != m_to_paint_on->w; ++x)
		{
			points[x][y].c = {
					3 * static_cast<double>(x) / m_to_paint_on->w - 2.0,
					2 * static_cast<double>(y) / m_to_paint_on->h - 1.0};
			points[x][y].z = {0, 0};
			points[x][y].order = current_order;

			const Uint8 part = 255 - static_cast<Uint8>(std::min(255.0 * points[x][y].order / current_order, 255.0));
			reinterpret_cast<SDL_Color *>(m_to_paint_on->pixels)[offset] = {part, part, part, 255};
			++offset;
		}
	}
	SDL_UnlockSurface(m_to_paint_on);

	print_off();

	bool done = false;
	while (!m_quitting)
	{
		if (done)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		else
		{
			constexpr auto BATCH_SIZE = 10;
			int advanced = 0;
			SDL_LockSurface(m_to_paint_on);
			std::size_t offset = 0;
			for (auto y = 0; y != m_to_paint_on->h; ++y)
			{
				for (auto x = 0; x != m_to_paint_on->w; ++x)
				{
					if (points[x][y].order == current_order)
					{
						auto i = 0;
						auto z = points[x][y].z * points[x][y].z + points[x][y].c;
						while (i != BATCH_SIZE
								&& 2 >= std::abs(z))
						{
							points[x][y].z = z;
							++i;
							z = z * z + points[x][y].c;
							if (z == points[x][y].z)
							{
								i = 0;
								points[x][y].order = MAX_ORDER;
								break;
							}
						}

						if (0 != i)
						{
							points[x][y].order += i;
							++advanced;
						}
					}

					//const auto &z = points[x][y].z;
					//const auto log_zn = std::log(z.real() * z.real() + z.imag() * z.imag()) / 2;
					//const auto nu = log(log_zn / log(2)) / log(2);

					const Uint8 part = 255 - static_cast<Uint8>(std::min(255.0 * points[x][y].order / current_order, 255.0));
					reinterpret_cast<SDL_Color *>(m_to_paint_on->pixels)[offset] = {part, part, part, 255};
					++offset;
				}
			}
			current_order += BATCH_SIZE;
			SDL_UnlockSurface(m_to_paint_on);

			done = current_order >= MAX_ORDER || 0 == advanced;

			print_off();

			SDL_Log("Printed Mandelbrot set of order %d; advanced: %d points", current_order, advanced);
			if (done)
			{
				SDL_Log("Done");
			}
		}
	}
}

void SDLApplication::print_off()
{
	std::lock_guard<std::mutex> guard(m_renderer_mutex);

	if (nullptr != m_to_print_off)
	{
		SDL_DestroyTexture(m_to_print_off);
	}

	m_to_print_off = SDL_CreateTextureFromSurface(m_renderer, m_to_paint_on);
	if (nullptr == m_to_print_off)
	{
		std::stringstream ss;
		ss << "Couldn't create texture to print off: " << SDL_GetError() << std::endl << "Quitting.";
		SDL_Log("%s", ss.str().c_str());

		m_quitting = true;
	}
}

int SDL_main(int argc, char **argv)
{
	try
	{
		SDLApplication application;

		return application.run();
	}
	catch (...)
	{
		return 1;
	}
}