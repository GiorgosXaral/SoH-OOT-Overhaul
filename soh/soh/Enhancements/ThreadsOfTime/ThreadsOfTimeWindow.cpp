#include "ThreadsOfTimeWindow.h"
#include "StoryProgress.h"

#include <cstdio>
#include <string>
#include <vector>

#include <imgui.h>
#include "soh/cvar_prefixes.h"
#include "soh/SohGui/ImGuiUtils.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
}

using namespace StoryProgress;

#define TOT_CVAR_WHOLE_ROAD CVAR_ENHANCEMENT("ThreadsOfTime.ShowWholeRoad")

static const ImVec4 kColorDone = ImVec4(0.55f, 0.85f, 0.55f, 1.0f);
static const ImVec4 kColorNow = ImVec4(1.0f, 0.85f, 0.35f, 1.0f);
static const ImVec4 kColorAhead = ImVec4(0.55f, 0.55f, 0.6f, 1.0f);
static const ImVec4 kColorDim = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);

static std::string FormatPlayTime(uint32_t tenths) {
    const uint32_t sec = tenths / 10;
    char buf[32];
    snprintf(buf, sizeof(buf), "%u:%02u:%02u", sec / 3600, (sec / 60) % 60, sec % 60);
    return buf;
}

// ---- Tapestry ---------------------------------------------------------------------------------

struct TapestryIcon {
    bool owned;
    std::string texture;
    std::string textureFaded;
    const char* name;
};

static TapestryIcon QuestIcon(uint32_t questItem, const char* name) {
    const auto& entry = questMapping.at(questItem);
    return { CHECK_QUEST_ITEM(questItem) != 0, entry.name, entry.nameFaded, name };
}

static TapestryIcon ItemIcon(uint32_t item, bool owned, const char* name) {
    const auto& entry = itemMapping.at(item);
    return { owned, entry.name, entry.nameFaded, name };
}

static void DrawIconRow(const std::vector<TapestryIcon>& icons, bool revealAll) {
    const float size = 36.0f;
    for (size_t i = 0; i < icons.size(); i++) {
        const TapestryIcon& icon = icons[i];
        if (i > 0) {
            ImGui::SameLine();
        }
        ImGui::Image(Ship::Context::GetInstance()->GetWindow()->GetGui()->GetTextureByName(
                         icon.owned ? icon.texture : icon.textureFaded),
                     ImVec2(size, size));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", (icon.owned || revealAll) ? icon.name : "???");
        }
    }
}

static void DrawTapestry(bool revealAll) {
    ImGui::SeparatorText("The Tapestry");

    DrawIconRow({ QuestIcon(QUEST_KOKIRI_EMERALD, "Kokiri's Emerald"), QuestIcon(QUEST_GORON_RUBY, "Goron's Ruby"),
                  QuestIcon(QUEST_ZORA_SAPPHIRE, "Zora's Sapphire") },
                revealAll);
    ImGui::SameLine(0.0f, 24.0f);
    DrawIconRow(
        {
            ItemIcon(ITEM_SWORD_KOKIRI, CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_KOKIRI) != 0,
                     "Kokiri Sword"),
            ItemIcon(ITEM_OCARINA_TIME, INV_CONTENT(ITEM_OCARINA_TIME) == ITEM_OCARINA_TIME, "Ocarina of Time"),
            ItemIcon(ITEM_SWORD_MASTER, CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_MASTER) != 0,
                     "Master Sword"),
        },
        revealAll);

    DrawIconRow({ QuestIcon(QUEST_MEDALLION_LIGHT, "Light Medallion"),
                  QuestIcon(QUEST_MEDALLION_FOREST, "Forest Medallion"),
                  QuestIcon(QUEST_MEDALLION_FIRE, "Fire Medallion"),
                  QuestIcon(QUEST_MEDALLION_WATER, "Water Medallion"),
                  QuestIcon(QUEST_MEDALLION_SHADOW, "Shadow Medallion"),
                  QuestIcon(QUEST_MEDALLION_SPIRIT, "Spirit Medallion") },
                revealAll);
    ImGui::SameLine(0.0f, 24.0f);
    DrawIconRow(
        {
            ItemIcon(ITEM_ARROW_LIGHT, INV_CONTENT(ITEM_ARROW_LIGHT) == ITEM_ARROW_LIGHT, "Light Arrows"),
            ItemIcon(ITEM_SWORD_BGS, CHECK_OWNED_EQUIP(EQUIP_TYPE_SWORD, EQUIP_INV_SWORD_BIGGORON) != 0,
                     "Biggoron's Sword"),
        },
        revealAll);
}

// ---- Steps ------------------------------------------------------------------------------------

// Steps are in order, so everything up to the last step with a tick is done too, and the step right after
// it is what to do next.
static void DrawSteps(const std::vector<Step>& steps, bool milestoneDone, bool highlightNext) {
    int32_t lastDone = -1;
    if (milestoneDone) {
        lastDone = static_cast<int32_t>(steps.size()) - 1;
    } else {
        for (size_t i = 0; i < steps.size(); i++) {
            if (steps[i].done && steps[i].done()) {
                lastDone = static_cast<int32_t>(i);
            }
        }
    }

    for (size_t i = 0; i < steps.size(); i++) {
        const Step& step = steps[i];
        const bool done = static_cast<int32_t>(i) <= lastDone;
        const bool isNext = highlightNext && (static_cast<int32_t>(i) == lastDone + 1);

        ImGui::PushID(static_cast<int>(i));
        if (step.done) {
            bool shown = done;
            ImGui::BeginDisabled();
            ImGui::Checkbox("##step", &shown);
            ImGui::EndDisabled();
            ImGui::SameLine();
        } else {
            ImGui::Bullet();
        }

        ImGui::PushStyleColor(ImGuiCol_Text, done ? kColorDim : (isNext ? kColorNow : ImGui::GetStyleColorVec4(ImGuiCol_Text)));
        if (isNext) {
            ImGui::TextWrapped("Next: %s", step.text.c_str());
        } else {
            ImGui::TextWrapped("%s", step.text.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
}

// ---- Next objective ---------------------------------------------------------------------------

static void DrawNextObjective() {
    ImGui::SeparatorText("Next Objective");

    const int32_t current = CurrentMilestoneIndex();
    if (current < 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, kColorDone);
        ImGui::TextWrapped("Hyrule is at peace. Your journey is complete - but there may still be secrets to find.");
        ImGui::PopStyleColor();
        return;
    }

    const Milestone& milestone = MainMilestones()[current];
    ImGui::PushStyleColor(ImGuiCol_Text, kColorNow);
    ImGui::TextWrapped("%s", milestone.title.c_str());
    ImGui::PopStyleColor();
    ImGui::TextDisabled("Where: %s", milestone.where.c_str());
    ImGui::Spacing();
    DrawSteps(milestone.steps, false, true);
}

// ---- Timeline ---------------------------------------------------------------------------------

static const char* ChapterName(Chapter chapter) {
    switch (chapter) {
        case Chapter::Child:
            return "Child";
        case Chapter::Adult:
            return "Adult";
        case Chapter::Finale:
        default:
            return "Finale";
    }
}

static void DrawTimeline(bool revealAll) {
    ImGui::SeparatorText("The Timeline");

    const auto& milestones = MainMilestones();
    const int32_t current = CurrentMilestoneIndex();

    for (Chapter chapter : { Chapter::Child, Chapter::Adult, Chapter::Finale }) {
        int32_t total = 0;
        int32_t done = 0;
        for (const Milestone& milestone : milestones) {
            if (milestone.chapter == chapter) {
                total++;
                done += milestone.done() ? 1 : 0;
            }
        }

        char header[64];
        snprintf(header, sizeof(header), "%s  (%d/%d)###chapter%d", ChapterName(chapter), done, total,
                 static_cast<int>(chapter));
        const bool chapterHasCurrent = (current >= 0) && (milestones[current].chapter == chapter);
        if (!ImGui::CollapsingHeader(header, chapterHasCurrent ? ImGuiTreeNodeFlags_DefaultOpen : 0)) {
            continue;
        }

        for (size_t i = 0; i < milestones.size(); i++) {
            const Milestone& milestone = milestones[i];
            if (milestone.chapter != chapter) {
                continue;
            }
            const bool isDone = milestone.done();
            const bool isNow = static_cast<int32_t>(i) == current;
            const bool revealed = isDone || isNow || revealAll;

            std::string label;
            ImVec4 color;
            if (isDone) {
                label = "Done:  " + milestone.title;
                const uint32_t time = MilestoneTime(milestone);
                if (time != 0) {
                    label += "   (" + FormatPlayTime(time) + ")";
                }
                color = kColorDone;
            } else if (isNow) {
                label = "Now:   " + milestone.title;
                color = kColorNow;
            } else {
                label = "Ahead: " + (revealAll ? milestone.title : milestone.veiled);
                color = kColorAhead;
            }
            label += "###milestone" + std::to_string(i);

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!revealed) {
                flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
            ImGui::PopStyleColor();

            if (revealed && open) {
                ImGui::TextDisabled("Where: %s", milestone.where.c_str());
                DrawSteps(milestone.steps, isDone, isNow);
                ImGui::TreePop();
            }
        }
    }
}

// ---- Side threads -----------------------------------------------------------------------------

static void DrawSideThreads() {
    ImGui::SeparatorText("Side Threads");

    const auto& threads = SideThreads();
    for (size_t i = 0; i < threads.size(); i++) {
        const SideThread& thread = threads[i];
        const bool done = thread.done();

        std::string label = (done ? "Done:  " : "") + thread.title + "###side" + std::to_string(i);
        ImGui::PushStyleColor(ImGuiCol_Text, done ? kColorDone : ImGui::GetStyleColorVec4(ImGuiCol_Text));
        const bool open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
        ImGui::PopStyleColor();

        if (open) {
            DrawSteps(thread.steps, done, !done);
            ImGui::TreePop();
        }
    }
}

// ---- Collection -------------------------------------------------------------------------------

static void DrawCollection() {
    ImGui::SeparatorText("Collection");

    static const uint32_t kSongs[] = {
        QUEST_SONG_LULLABY, QUEST_SONG_EPONA, QUEST_SONG_SARIA,  QUEST_SONG_SUN,      QUEST_SONG_TIME,
        QUEST_SONG_STORMS,  QUEST_SONG_MINUET, QUEST_SONG_BOLERO, QUEST_SONG_SERENADE, QUEST_SONG_REQUIEM,
        QUEST_SONG_NOCTURNE, QUEST_SONG_PRELUDE,
    };
    int32_t songs = 0;
    for (uint32_t song : kSongs) {
        songs += CHECK_QUEST_ITEM(song) ? 1 : 0;
    }

    const int32_t heartContainers = gSaveContext.healthCapacity / 16;
    const int32_t heartPieces = (gSaveContext.inventory.questItems & 0xF0000000) >> 28;

    if (ImGui::BeginTable("totCollection", 2, ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Hearts");
        ImGui::TableNextColumn();
        ImGui::Text("%d / 20   (+%d/4 pieces toward the next)", heartContainers, heartPieces);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Gold Skulltulas");
        ImGui::TableNextColumn();
        ImGui::Text("%d / 100", gSaveContext.inventory.gsTokens);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("Songs");
        ImGui::TableNextColumn();
        ImGui::Text("%d / 12", songs);

        ImGui::EndTable();
    }
}

// ---- Window -----------------------------------------------------------------------------------

void ThreadsOfTimeWindow::DrawElement() {
    if (!GameInteractor::IsSaveLoaded()) {
        ImGui::TextWrapped("Load a save file to see your journey.");
        return;
    }

    if (IS_RANDO) {
        ImGui::PushStyleColor(ImGuiCol_Text, kColorAhead);
        ImGui::TextWrapped("Threads of Time follows the original story. In a Randomizer save, items are in other "
                           "places, so the directions won't match.");
        ImGui::PopStyleColor();
    }

    bool revealAll = CVarGetInteger(TOT_CVAR_WHOLE_ROAD, 0) != 0;
    if (ImGui::Checkbox("Show the whole road", &revealAll)) {
        CVarSetInteger(TOT_CVAR_WHOLE_ROAD, revealAll ? 1 : 0);
        Ship::Context::GetInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reveal the names of every milestone still ahead (spoilers).");
    }

    DrawTapestry(revealAll);
    DrawNextObjective();
    DrawTimeline(revealAll);
    DrawSideThreads();
    DrawCollection();
}
