#include "system_view.hpp"

#include "epaper.hpp"

void SystemView::init()
{
  dirty_ = true;
}

bool SystemView::needs_render() const
{
  return dirty_;
}

void SystemView::render()
{
  EPaper::showSystemPlaceholder("System", "Style placeholder");
  dirty_ = false;
}

void SystemView::mark_dirty()
{
  dirty_ = true;
}
