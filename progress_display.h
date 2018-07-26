#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <unordered_map>

namespace detail {
struct progress_bar
{
  std::string        _description;
  std::atomic_size_t _current_value;
  size_t             _max_value;
  size_t             _vertical_offset;

  size_t _prev_symbols_drawn;

  inline progress_bar & operator=(const progress_bar & rhs)
  {
    std::atomic_init(&_current_value, rhs._current_value);
    _description        = rhs._description;
    _max_value          = rhs._max_value;
    _vertical_offset    = rhs._vertical_offset;
    _prev_symbols_drawn = rhs._prev_symbols_drawn;
    return *this;
  }
};

struct vec2
{
  size_t x, y;
};
} // namespace detail

class progress_display final
{
public:
  progress_display();
  progress_display(const progress_display &) = delete;
  progress_display & operator=(const progress_display &) = delete;
  progress_display(progress_display &&)                  = default;

  size_t add_progress_bar(std::string description, const size_t max_progress_value);
  void   tick_progress_bar_total(const size_t progress_bar_id);
  void   tick_progress_bar_current(const size_t progress_bar_id);
  void   remove_progress_bar(const size_t progress_bar_id);
  void   draw();

private:
  void   set_cursor_to_progress_bar_offset(const size_t vertical_offset, const size_t horizontal_offset = 0);
  void   reset_cursor();
  size_t get_max_description_length() const;
  void   clear_to_right(const size_t count);

  size_t                                           _next_token_id = 1;
  std::unordered_map<size_t, detail::progress_bar> _progress_bars;

  detail::vec2 _term_dimensions;
  detail::vec2 _initial_cursor_pos;

  size_t _max_symbols_printed_on_draw = 0;

  const float _progress_bar_size = 0.2f;
  const bool  _remove_completed  = true;
  const char  _filled_char       = 'x';
  const char  _empty_char        = '_';
  const bool  _use_percents      = false;
};
