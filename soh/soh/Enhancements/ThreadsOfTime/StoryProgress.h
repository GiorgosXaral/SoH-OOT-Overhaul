#pragma once

// SoH-OOT-Overhaul: the story of Ocarina of Time as a list of milestones, each with step-by-step
// directions, worked out from the save file (event flags, quest items, items, upgrades).
// Used by the Threads of Time window, and meant to be shared with Navi's hints.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace StoryProgress {

enum class Chapter {
    Child,
    Adult,
    Finale,
};

struct Step {
    std::string text;
    // Optional: when set, the step shows a tick once it returns true. Steps inside a dungeon that leave
    // no trace in the save have no check and are shown as plain directions.
    std::function<bool()> done;
};

struct Milestone {
    Chapter chapter;
    std::string title;  // the real name, shown once the milestone is reached
    std::string veiled; // a spoiler-free hint, shown for milestones still ahead
    std::string where;  // the main area
    std::vector<Step> steps;
    std::function<bool()> done;
    int32_t timestamp; // index into gSaveContext.ship.stats.itemTimestamp, or -1
};

// A side quest: always shown in full, since it doesn't spoil the story.
struct SideThread {
    std::string title;
    std::vector<Step> steps;
    std::function<bool()> done;
};

const std::vector<Milestone>& MainMilestones();
const std::vector<SideThread>& SideThreads();

// Index of the first main milestone not done yet, or -1 once the story is complete.
int32_t CurrentMilestoneIndex();

// Time the milestone was reached (tenths of a second of total play time), or 0 if not recorded.
uint32_t MilestoneTime(const Milestone& milestone);

} // namespace StoryProgress
