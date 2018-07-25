#include <algorithm>
#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <algorithm>

#include "progress_display.h"

inline void set_cursor_to(const COORD position) { SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), position); }

using namespace detail;

size_t progress_display::add_progress_bar(std::string description, const size_t max_value)
{
  if(_term_dimensions.y == _progress_bars.size())
    return 0;
  _progress_bars[_next_token_id] = progress_bar{std::move(description), 0, max_value, 1 + _progress_bars.size()};
  return _next_token_id++;
}

void progress_display::clear_to_right(const size_t x_pos, const size_t count)
{
  std::string clear_line(count, ' ');
  std::cout << clear_line;
}

void progress_display::remove_progress_bar(const size_t progress_bar_id) // todo: do not redraw right away => not threadsafe
{
  auto it = _progress_bars.find(progress_bar_id);
  if(it == cend(_progress_bars))
    return;

  const size_t removed_vertical_offset = it->second._vertical_offset;
  set_cursor_to_progress_bar_offset(_progress_bars.size());
  clear_to_right(0, _max_symbols_printed_on_draw);
  _progress_bars.erase(it);

  for(auto & kv : _progress_bars)
    if(kv.second._vertical_offset > removed_vertical_offset)
      --kv.second._vertical_offset;
  draw();
}

void progress_display::update_progress_bar(const size_t progress_bar_id, const size_t new_value)
{
  auto it = _progress_bars.find(progress_bar_id);
  if(it == cend(_progress_bars))
    return;
  if(_remove_completed && new_value >= it->second._max_value)
    remove_progress_bar(progress_bar_id);
  else
    it->second._current_value = std::min(new_value, it->second._max_value);
}

progress_display::progress_display()
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  _term_dimensions    = {static_cast<size_t>(csbi.srWindow.Right - csbi.srWindow.Left),
                      static_cast<size_t>(csbi.srWindow.Bottom - csbi.srWindow.Top)};
  _initial_cursor_pos = {static_cast<size_t>(csbi.dwCursorPosition.X), static_cast<size_t>(csbi.dwCursorPosition.Y)};
}

void progress_display::set_cursor_to_progress_bar_offset(const size_t offset)
{
  set_cursor_to({static_cast<SHORT>(_initial_cursor_pos.x), static_cast<SHORT>(_initial_cursor_pos.y + offset)});
}
void progress_display::reset_cursor()
{
  set_cursor_to({static_cast<SHORT>(_initial_cursor_pos.x), static_cast<SHORT>(_initial_cursor_pos.y)});
}
size_t progress_display::get_max_description_length() const
{
  size_t result = 0;
  for(auto & kv : _progress_bars)
    result = std::max(result, kv.second._description.length());
  return result;
}

void progress_display::draw()
{
  const size_t mdr = get_max_description_length();
  for(auto & kv : _progress_bars)
  {
    size_t         symbols_printed = 0;
    progress_bar & bar             = kv.second;
    const float    ratio           = static_cast<float>(bar._current_value) / bar._max_value;
    const size_t   progress_bar_size =
      static_cast<size_t>(static_cast<float>(_term_dimensions.x - mdr) * _progress_bar_size);
    set_cursor_to_progress_bar_offset(bar._vertical_offset);
    const size_t leftpad = mdr - bar._description.length();
    for(size_t i = 0; i < leftpad; ++i)
      std::cout << ' ';
    symbols_printed += leftpad;
    symbols_printed += printf_s("%s [", bar._description.c_str());
    char percents_buffer[16]{};
    symbols_printed += snprintf(percents_buffer, sizeof(percents_buffer), " %3.1f%%", ratio * 100.f);
    const size_t nfilled_symbols = static_cast<size_t>(progress_bar_size * ratio);
    const size_t nto_fill        = progress_bar_size;
    symbols_printed += nto_fill;
    for(size_t i = 0; i < nto_fill; ++i)
    {
      const bool fill_value = i < nfilled_symbols;
      std::cout << (fill_value ? _filled_char : _empty_char);
    }
    std::cout << ']' << percents_buffer;
    symbols_printed++;
    _max_symbols_printed_on_draw = std::max(_max_symbols_printed_on_draw, symbols_printed);
    clear_to_right(symbols_printed, _max_symbols_printed_on_draw - symbols_printed);
  }
}