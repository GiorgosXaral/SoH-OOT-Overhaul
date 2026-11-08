#pragma once

#include <libultraship/libultraship.h>

// SoH-OOT-Overhaul: Threads of Time, a window showing the journey so far, what to do next, and what's still
// ahead (veiled unless "Show the whole road" is on).
class ThreadsOfTimeWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};
