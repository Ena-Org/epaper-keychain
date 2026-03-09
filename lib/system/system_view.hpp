#pragma once

class SystemView
{
public:
  void init();

  bool needs_render() const;

  void render();

  void mark_dirty();

private:
  bool dirty_ = true;
};
