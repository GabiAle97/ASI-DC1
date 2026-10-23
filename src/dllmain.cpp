#include <Windows.h>
#include <algorithm>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <sstream>
#include <MinHook.h>

// ============================================================
// Speedhack
// ============================================================

static std::atomic<float> g_speed{1.0f};
static std::atomic<float> g_cutscene_speed{1.0f};
static std::atomic<float> g_doorskip_speed{1.0f};
static DWORD g_baseTime = 0;
static LARGE_INTEGER g_baseQPC = {0};

typedef DWORD (WINAPI* timeGetTime_t)();
typedef BOOL (WINAPI* QPC_t)(LARGE_INTEGER*);

static timeGetTime_t o_timeGetTime = nullptr;
static QPC_t o_QueryPerformanceCounter = nullptr;

DWORD WINAPI hk_timeGetTime() {
    DWORD now = o_timeGetTime();
    if (g_baseTime == 0) g_baseTime = now;
    return g_baseTime + static_cast<DWORD>((now - g_baseTime) * g_speed.load());
}

BOOL WINAPI hk_QueryPerformanceCounter(LARGE_INTEGER* lp) {
    BOOL ok = o_QueryPerformanceCounter(lp);
    if (!ok) return ok;
    if (g_baseQPC.QuadPart == 0) g_baseQPC = *lp;

    LONGLONG delta = lp->QuadPart - g_baseQPC.QuadPart;
    lp->QuadPart = g_baseQPC.QuadPart + static_cast<LONGLONG>(delta * g_speed.load());
    return TRUE;
}

void InitSpeedhack() {
    if (MH_Initialize() != MH_OK) return;

    MH_CreateHookApi(L"winmm.dll", "timeGetTime", &hk_timeGetTime, (LPVOID*)&o_timeGetTime);
    MH_CreateHookApi(L"kernel32.dll", "QueryPerformanceCounter", &hk_QueryPerformanceCounter, (LPVOID*)&o_QueryPerformanceCounter);

    MH_EnableHook(MH_ALL_HOOKS);
}

void SetGameSpeed(float multiplier) {
    g_speed = multiplier;
}

void RefreshGameSpeed() {
    g_speed = (std::max)(g_cutscene_speed.load(), g_doorskip_speed.load());
}

void SetCutsceneSpeed(float multiplier) {
    g_cutscene_speed = multiplier;
    RefreshGameSpeed();
}

void SetDoorSkipSpeed(float multiplier) {
    g_doorskip_speed = multiplier;
    RefreshGameSpeed();
}

// ============================================================
// Memory helpers
// ============================================================


template<typename T>
inline T Read(uintptr_t address) {
    return *reinterpret_cast<volatile T*>(address);
}

template<typename T>
inline void Write(uintptr_t address, T value) {
    *reinterpret_cast<volatile T*>(address) = value;
}

inline void WriteBytes(uintptr_t address, const uint8_t* data, size_t size) {
    memcpy(reinterpret_cast<void*>(address), data, size);
}

inline uint16_t ReadSmallInteger(uintptr_t address) {
    return Read<uint16_t>(address);
}

inline void WriteSmallInteger(uintptr_t address, uint16_t value) {
    Write<uint16_t>(address, value);
}

inline uint32_t ReadInteger(uintptr_t address) {
    return Read<uint32_t>(address);
}

inline void WriteInteger(uintptr_t address, uint32_t value) {
    Write<uint32_t>(address, value);
}

inline uint8_t ReadByte(uintptr_t address) {
    return Read<uint8_t>(address);
}

inline void WriteByte(uintptr_t address, uint8_t value) {
    Write<uint8_t>(address, value);
}

// ============================================================
// Addresses
// ============================================================

constexpr uintptr_t flag_addr             = 0x006D3F64;
constexpr uintptr_t nods_flag_addr        = 0x006D3F53;
constexpr uintptr_t system_addr           = 0x006D3E68;
constexpr uintptr_t cutscene_real_addr    = 0x006D3EB0;
constexpr uintptr_t door_anim_addr        = 0x0067B8A8;
constexpr uintptr_t doorskip_addr         = 0x006D3E6C;
constexpr uintptr_t room_addr             = 0x006DD8F0;
constexpr uintptr_t key_item_addr         = 0x006DD9DD;
constexpr uintptr_t coord_x_addr          = 0x006D58AC;
constexpr uintptr_t coord_y_addr          = 0x006D58A8;
constexpr uintptr_t coord_o_addr          = 0x006D58B2;
constexpr uintptr_t anim_addr             = 0x006D58C5;
constexpr uintptr_t entity_flag_addr      = 0x006D58B8;
constexpr uintptr_t float_304_addr        = 0x00677D28;
constexpr uintptr_t flag_crane            = 0x006DCE5E;
constexpr uintptr_t flag_action_done      = 0x006D3E7C;
constexpr uintptr_t flag_action_done_bis  = 0x006D3E80;
constexpr uintptr_t flag_reja             = 0x006DC09B;
constexpr uintptr_t flag_colision_reja    = 0x006DD930;
constexpr uintptr_t flag_caja             = 0x006D5B2D;
constexpr uintptr_t timer_addr            = 0x006DD8F4;
constexpr uintptr_t prev_room_addr         = 0x006DD0BC;

// ============================================================
// State
// ============================================================

static std::atomic<bool> g_running{false};
static std::atomic<bool> g_enabled{false};
static std::atomic<bool> g_doorskip{false};
static std::atomic<bool> g_f2_pressed{false};
static std::atomic<bool> g_f11_pressed{false};
static std::atomic<DWORD> g_hotkey_thread_id{0};
static HMODULE g_module_handle = nullptr;

static int saved_x = 0;
static int saved_y = 0;
static int saved_o = 0;
static bool pending_skip = false;
static bool pending_restore = false;
static int capture_frame = 0;
static int skip_flag = 0;
static int counter = 0;
static bool skip_allow = false;

static int room_610 = 0;
static int room_106 = 0;
static int room_405 = 0;
static int room_30C = 0;
static int room_304 = 0;
static int room_30E = 0;
static int room_202 = 0;
static int room_30B = 0;
static int room_10F = 0;
static int room_205 = 0;
static int room_600 = 0;
static int room_608 = 0;
static int room_609 = 0;
static int room_601 = 0;
static int room_50F = 0;
static int room_60F = 0;
static int room_509 = 0;
static int room_409 = 0;
static int room_40C = 0;
static int room_406 = 0;
static int room_309 = 0;
static int room_50B = 0;
static int room_112 = 0;
static int room_103 = 0;
static int room_10D = 0;

static int puzzle_cicle = 0;
static bool teleport = false;
static bool second_teleport = false;
static bool teleport_after_cutscene = false;
static bool special_skip_allow = false;
static int counter_flag_zero = 0;
static int counter_roomset = 0;

static bool timer_enabled = false;
static uint32_t timer_start_value = 0;
static uint32_t real_time_start = 0;
static constexpr int timer_units_per_second = 60;
static constexpr float speedhack_multiplier = 5000.0f;

// Puzzle rooms (from original script)
static const std::unordered_map<uint16_t, bool> PUZZLE_ROOMS = {
    {0x010F, true}, {0x0202, true}, {0x030B, true}, {0x0106, true},
    {0x030E, true}, {0x0405, true}, {0x0113, true}, {0x0309, true},
    {0x0304, true}, {0x030C, true}, {0x0201, true}, {0x060F, true},
    {0x0403, true}, {0x0307, true}, {0x0205, true}, {0x0600, true},
    {0x0608, true}, {0x0609, true}, {0x0601, true}, {0x0604, true},
    {0x0509, true}, {0x050A, true}, {0x050F, true}, {0x0610, true},
    {0x0613, true}, {0x0409, true}, {0x0614, true}, {0x0611, true}
};

// ============================================================
// Helper
// ============================================================

inline bool IsPuzzleRoom(uint16_t room) {
    return PUZZLE_ROOMS.find(room) != PUZZLE_ROOMS.end();
}

inline uint32_t GetTickCountSafe() {
    return GetTickCount();
}

// ============================================================
// Main logic
// ============================================================

void SkipCutsceneLogic() {
    if (!g_enabled) return;

    uint8_t flag = ReadByte(flag_addr);
    uint8_t nods_flag = ReadByte(nods_flag_addr);
    uint8_t sys = ReadByte(system_addr);
    uint8_t real = ReadByte(cutscene_real_addr);
    uint16_t room_id = ReadSmallInteger(room_addr);
    uint16_t prev_room_id = ReadSmallInteger(prev_room_addr);
    uint8_t crane_init = ReadByte(flag_crane);
    uint16_t action_done = ReadSmallInteger(flag_action_done);
    uint8_t action_done_bis = ReadByte(flag_action_done_bis);
    uint8_t caja_move = ReadByte(flag_caja);
    uint32_t colision = ReadInteger(flag_colision_reja);
    uint8_t puzzle_bat = ReadByte(0x006DD936);

    if (room_id == 0x0600) {
        timer_enabled = false;
        SetCutsceneSpeed(1.0f);
    }

    // Reset on room 0
    if (room_id == 0x0000) {
        saved_x = saved_y = saved_o = 0;
        pending_skip = pending_restore = false;
        capture_frame = skip_flag = counter = 0;
        skip_allow = false;
        room_106 = room_405 = room_30C = room_304 = room_30E = 0;
        room_202 = room_30B = room_10F = room_205 = room_600 = 0;
        room_608 = room_609 = room_601 = room_50F = room_60F = 0;
        room_610 = room_509 = room_409 = room_40C = room_406 = 0;
        room_309 = room_50B = room_112 = room_103 = room_10D = 0;
        puzzle_cicle = counter_roomset = 0;
        timer_enabled = teleport = second_teleport = false;
        teleport_after_cutscene = special_skip_allow = false;
    }

    // Specific room fixes
    if (room_id == 0x010C && room_10D == 3) {
        WriteByte(0x006DD935, 0x9E);
        WriteByte(0x006DD9D1, 0x02);
    }
    if (room_id == 0x0112 && prev_room_id == 0x0114) {
        WriteByte(0x006DD936, 0x0D);
    }
    if (room_id == 0x010A && counter_roomset == 0) {
        WriteByte(0x006DD934, 0x60);
        counter_roomset++;
    }

    if ((room_id == 0x050A || room_id == 0x0509 || room_id == 0x050B) &&
        (action_done == 0x0012 || action_done == 0x1014 || action_done == 0x1016 || action_done == 0x1002 || action_done == 0x0002)) {
        WriteSmallInteger(flag_action_done, 0x1000);
    }

    if (room_id == 0x0509 && action_done == 0x1012) {
        WriteSmallInteger(flag_action_done, 0x1000);
    }

    if (room_id == 0x050B && action_done == 0x1012) {
        WriteSmallInteger(flag_action_done, 0x1000);
    }

    if (room_id == 0x0611 && real != 0 && action_done == 0x1006) {
        WriteSmallInteger(flag_action_done, 0x1004);
    }

    if (room_id == 0x0105 && room_106 == 2) {
        WriteByte(flag_reja, 0xF3);
        const uint8_t data[] = {0x00, 0x28, 0x5E, 0x71};
        WriteBytes(flag_colision_reja, data, 4);
    }

    if (room_id == 0x010D && room_106 == 2) {
        const uint8_t data[] = {0x02, 0x38, 0x5F, 0x71};
        WriteBytes(flag_colision_reja, data, 4);
    }

    if (room_id == 0x0307) {
        const uint8_t data[] = {0x00, 0x3A, 0x5F, 0x71};
        WriteBytes(flag_colision_reja, data, 4);
    }

    if (room_id == 0x0304 && flag == 4 && real == 0 && room_304 == 0) {
        real = 4;
        room_304++;
    }

    if (room_id == 0x0107 && colision == 0x344E2800) {
        const uint8_t data[] = {0x00, 0x28, 0x4E, 0x74};
        WriteBytes(flag_colision_reja, data, 4);
    }

    if (room_id == 0x0105 && colision == 0x04400000) {
        const uint8_t data[] = {0x00, 0x00, 0x44, 0x04};
        WriteBytes(flag_colision_reja, data, 4);
    }

    if (room_id == 0x0309 && room_309 == 0) {
        teleport_after_cutscene = true;
    }

    // Timer / speedhack logic (simplified - speedhack needs proper implementation)
    if (room_id == 0x0615 || room_id == 0x0402 || room_id == 0x030C ||
        room_id == 0x050A || room_id == 0x050B || room_id == 0x0509 ||
        room_id == 0x0614 || room_id == 0x0611 || room_id == 0x0508 ||
        room_id == 0x0408 || room_id == 0x0609) {

        bool should_speed = false;
        if (crane_init == 1 ||
            ((room_id == 0x0402 || room_id == 0x030C || room_id == 0x0615) && caja_move == 255) ||
            (((room_id == 0x050A && real != 0) || (room_id == 0x050A && flag == 2) ||
              (room_id == 0x0509 && real != 0) || (room_id == 0x0509 && flag == 2) ||
              (room_id == 0x050B && real != 0) || (room_id == 0x050B && flag == 2)) && action_done == 0x0010) ||
            (((room_id == 0x050A && real != 0) || (room_id == 0x050A && flag == 2) ||
              (room_id == 0x0509 && real != 0) || (room_id == 0x0509 && flag == 2) ||
              (room_id == 0x050B && real != 0) || (room_id == 0x050B && flag == 2)) && nods_flag == 0) ||
              
            ((room_id == 0x0408 || room_id == 0x0609) && action_done_bis == 4) ||
            (room_id == 0x0614 && real != 0) ||
            (room_id == 0x0508 && action_done_bis == 6) ||
            (room_id == 0x0611 && real != 0)) {
            should_speed = true;
        }

        if (should_speed) {
            if (!timer_enabled) {
                timer_enabled = true;
                timer_start_value = ReadInteger(timer_addr);
                real_time_start = GetTickCountSafe();
            }
            SetCutsceneSpeed(speedhack_multiplier);
            if (room_id == 0x0508 && action_done_bis == 6) {
                if (puzzle_cicle < 4) {
                    puzzle_cicle++;
                } else {
                    SetCutsceneSpeed(1.0f);
                }
            }
            if (room_id == 0x0609 && action_done_bis == 4) {
                if (puzzle_cicle < 6) {
                    puzzle_cicle++;
                } else {
                    SetCutsceneSpeed(1.0f);
                }
            }
        } else {
            if (timer_enabled) {
                uint32_t elapsed_real_ms = GetTickCountSafe() - real_time_start;
                uint32_t elapsed_slow_ms = elapsed_real_ms / 5;
                uint32_t elapsed_units = static_cast<uint32_t>(elapsed_slow_ms * timer_units_per_second / 1000.0f + 0.5f);
                uint32_t final_value = timer_start_value + elapsed_units + 30;
                WriteInteger(timer_addr, final_value);
                timer_enabled = false;
                SetCutsceneSpeed(1.0f);
                puzzle_cicle = 0;
            }
        }
    }

    // Rising edge of real cutscene
    if (real != 0 && !pending_skip && !pending_restore) {
        skip_allow = false;

        if (room_id == 0x010F) {
            if (room_10F == 1) skip_allow = true;
            if (room_10F == 0 && puzzle_bat == 5) {
                skip_allow = true;
                WriteByte(0x006DD936, 0x05);
                WriteByte(0x006DD937, 0x02);
            }
        }
        if (room_id == 0x0304) skip_allow = true;
        if (room_id == 0x0106 && (room_106 == 0 || room_106 == 1)) skip_allow = true;
        if (room_id == 0x0405 && (room_405 == 0 || room_405 == 2)) skip_allow = true;
        if (room_id == 0x030C && (room_30C <= 1 || room_30C == 2)) skip_allow = true;
        if (room_id == 0x030E && room_30E == 0) skip_allow = true;
        if (room_id == 0x0202) skip_allow = true;
        if (room_id == 0x030B && room_30B == 1) skip_allow = true;
        if (room_id == 0x0205 && (room_205 == 1 || room_205 == 2)) skip_allow = true;
        if (room_id == 0x0600 && (room_600 == 2 || room_600 == 3)) skip_allow = true;
        if (room_id == 0x0600 && room_600 == 1) special_skip_allow = true;
        if (room_id == 0x0608) {
            skip_allow = true;
            if (room_608 == 0) {
                WriteByte(0x006DD942, 0x54);
                WriteByte(0x006DD94F, 0x7F);
            }
        }
        if (room_id == 0x050F) {
            if (room_50F == 0) skip_allow = true;
            if (room_50F == 1) teleport_after_cutscene = true;
        }
        if (room_id == 0x0609 && (room_609 == 0 || room_609 == 1 || room_609 == 3)) {
            skip_allow = true;
            if (room_609 == 0) {
                second_teleport = true;
                WriteByte(0x006DD94A, 0xC9);
            }
        }
        if (room_id == 0x0609 && room_609 == 3) {
            const uint8_t data[] = {0x6A, 0xBA, 0x5F, 0x73};
            WriteBytes(flag_colision_reja, data, 4);
        }
        if (room_id == 0x060F) skip_allow = true;
        if (room_id == 0x0610 && room_610 == 0) skip_allow = true;
        if (room_id == 0x0307) special_skip_allow = true;
        if (room_id == 0x0409) teleport_after_cutscene = true;
        if (room_id == 0x030E && room_30E == 0) teleport_after_cutscene = true;
        if (room_id == 0x0509 && room_509 == 0) skip_allow = true;

        if (!IsPuzzleRoom(room_id) || skip_allow) {
            saved_x = ReadSmallInteger(coord_x_addr);
            saved_y = ReadSmallInteger(coord_y_addr);
            saved_o = ReadSmallInteger(coord_o_addr);
            pending_skip = true;
            capture_frame = 0;
            skip_allow = false;

            // Key item writes
            if (room_id == 0x030C && room_30C == 1) {
                const uint8_t data[] = {0x00, 0x60, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
                WriteBytes(key_item_addr, data, 9);
            }
            if (room_id == 0x0202 && room_202 == 0) {
                const uint8_t data[] = {0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04};
                WriteBytes(key_item_addr, data, 9);
            }
            if (room_id == 0x050E) {
                const uint8_t data[] = {0x00, 0x60, 0x15, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
                WriteBytes(key_item_addr, data, 9);
            }
            if (room_id == 0x0610 && room_610 == 0) {
                const uint8_t data[] = {0x00, 0x20, 0x1D, 0x04, 0x00, 0x10, 0x01, 0x00, 0x00};
                WriteBytes(key_item_addr, data, 9);
            }

            if (room_id == 0x0304) {
                if (room_304 == 0) {
                    saved_x = 5752;
                    saved_y = 1704;
                }
                if (room_304 == 2) teleport = true;
            }
            if (room_id == 0x060F && (room_60F == 0 || room_60F == 1)) {
                teleport_after_cutscene = true;
            }
            if (room_id == 0x040C) teleport_after_cutscene = true;
            if (room_id == 0x0112 && room_112 == 0) teleport_after_cutscene = true;
            if (room_id == 0x0103) {
                if (room_103 == 0) {
                    const uint8_t data[] = {0x00, 0x00, 0x46, 0x04};
                    WriteBytes(flag_colision_reja, data, 4);
                }
                if (room_103 == 1) {
                    const uint8_t data[] = {0x00, 0x00, 0x4E, 0x04};
                    WriteBytes(flag_colision_reja, data, 4);
                }
            }
        }
        skip_flag = 1;
    }

    // Timer freeze while enabled
    if (timer_enabled) {
        WriteInteger(timer_addr, timer_start_value);
    }

    // Second teleport
    if (second_teleport) {
        if (room_id == 0x0609 && room_609 == 0 && capture_frame >= 10) {
            WriteSmallInteger(coord_x_addr, 65042);
            WriteSmallInteger(coord_y_addr, 4477);
            WriteSmallInteger(coord_o_addr, 2888);
            second_teleport = false;
        }
    }

    // Teleport after cutscene
    if (teleport_after_cutscene) {
        if (room_id == 0x060F && room_60F == 0) {
            const uint8_t data[] = {0x0C, 0x03};
            WriteBytes(room_addr, data, 2);
            room_60F++;
        }
        if (room_id == 0x0409 && room_409 == 0) {
            const uint8_t data[] = {0x0A, 0x04};
            WriteBytes(room_addr, data, 2);
            room_409++;
        }
        if (room_id == 0x040C && room_40C == 0) {
            const uint8_t data[] = {0x00, 0x06};
            WriteBytes(room_addr, data, 2);
            room_40C++;
        }
        if (room_id == 0x030E && room_30E == 0) {
            const uint8_t data[] = {0x09, 0x03};
            WriteBytes(room_addr, data, 2);
            room_309++;
        }
        if (room_id == 0x0309 && room_309 == 0) {
            const uint8_t data[] = {0x0E, 0x03};
            WriteBytes(room_addr, data, 2);
        }
        if (room_id == 0x0112 && room_112 == 0) {
            const uint8_t data[] = {0x0D, 0x01};
            WriteBytes(room_addr, data, 2);
            room_112++;
        }
        if (room_id == 0x050F && room_50F == 1) {
            const uint8_t data[] = {0x09, 0x06};
            WriteBytes(room_addr, data, 2);
            room_50B++;
        }
        special_skip_allow = true;
        teleport_after_cutscene = false;
    }

    // Teleport logic
    if (teleport) {
        uint8_t flag_zero = ReadByte(float_304_addr);
        if (flag_zero != 0) {
            counter_flag_zero++;
            if (counter_flag_zero == 5) {
                if (room_id == 0x0304) {
                    const uint8_t data[] = {0x01, 0x03};
                    WriteBytes(room_addr, data, 2);
                }
                special_skip_allow = true;
                teleport = false;
                counter_flag_zero = 0;
            }
        }
    }

    // Room counter increments on cutscene end
    if (real == 0 && skip_flag == 1) {
        skip_flag = 0;
        if (room_id == 0x0202) room_202++;
        if (room_id == 0x030B) room_30B++;
        if (room_id == 0x010F) room_10F++;
        if (room_id == 0x030C) room_30C++;
        if (room_id == 0x030E) room_30E++;
        if (room_id == 0x0106) room_106++;
        if (room_id == 0x0405) room_405++;
        if (room_id == 0x0205) room_205++;
        if (room_id == 0x0600) room_600++;
        if (room_id == 0x050F) room_50F++;
        if (room_id == 0x0610) room_610++;
        if (room_id == 0x0609) room_609++;
        if (room_id == 0x0608) room_608++;
        if (room_id == 0x0509) room_509++;
        if (room_id == 0x0304) room_304++;
        if (room_id == 0x0406) room_406++;
        if (room_id == 0x0103) room_103++;
        if (room_id == 0x010D) room_10D++;
    }

    // Pending skip
    if (pending_skip) {
        const uint8_t sys_data[] = {0x06, 0x00, 0x00, 0x00, 0x00};
        WriteBytes(system_addr, sys_data, 5);
        WriteByte(flag_addr, 0x00);
        pending_skip = false;

        if (room_id == 0x030C && room_30C == 0) {
            saved_x = 64701;
            saved_y = 8602;
            saved_o = 3112;
        }
        if (room_id == 0x0609 && room_609 == 0) {
            saved_x = 65090;
            saved_y = 64751;
            saved_o = 130;
        }
        if (room_id == 0x0205 && room_205 == 1) {
            saved_o = 1936;
        }
        pending_restore = true;
        capture_frame = 0;
    }

    // Special skip allow
    if (special_skip_allow) {
        const uint8_t sys_data[] = {0x06, 0x00, 0x00, 0x00, 0x00};
        WriteBytes(system_addr, sys_data, 5);
        special_skip_allow = false;

        if (room_id == 0x0600) {
            saved_x = 58055;
            saved_y = 8125;
        }
        if (room_id == 0x0304) {
            saved_x = 5493;
            saved_y = 11198;
        }
        if (room_id == 0x0307) {
            saved_x = 1204;
            saved_y = 62598;
            saved_o = 1000;
        }
        if (room_id == 0x030E) {
            saved_x = 64867;
            saved_y = 56048;
            saved_o = 32;
        }
        if (room_id == 0x0409) {
            saved_x = 63790;
            saved_y = 61289;
            saved_o = 1146;
        }
        if (room_id == 0x060F && room_60F == 1) {
            saved_x = 64701;
            saved_y = 8602;
            saved_o = 3112;
        }
        pending_restore = true;
        capture_frame = 0;
    }

    // Pending restore
    if (pending_restore) {
        uint32_t door_val = ReadInteger(door_anim_addr);
        if (room_id == 0x0301) {
            saved_o = 3170;
            WriteSmallInteger(coord_o_addr, saved_o);
        }
        if (sys == 3 && door_val == 0) {
            if (room_id != 0x050B) {
                WriteSmallInteger(coord_x_addr, saved_x);
                WriteSmallInteger(coord_y_addr, saved_y);
                if (room_id != 0x0109) {
                    WriteSmallInteger(coord_o_addr, saved_o);
                }
            }
            capture_frame++;
            if (capture_frame >= 10) {
                const uint8_t anim_data[] = {0x00, 0x00};
                WriteBytes(anim_addr, anim_data, 2);
                WriteByte(entity_flag_addr, 0x03);
                pending_restore = false;
                saved_x = saved_y = saved_o = 0;
            }
        }
    }
}

// ============================================================
// Door skip (integrated into main loop)
// ============================================================

static int door_cicle = 0;

void ProcessDoorSkip() {
    if (!g_doorskip) {
        door_cicle = 0;
        SetDoorSkipSpeed(1.0f);
        return;
    }

    uint32_t door_event = ReadInteger(doorskip_addr);
    uint16_t room_id = ReadSmallInteger(room_addr);

    if (door_event == 1 && room_id != 0x0000) {
        door_cicle = 0;
        SetDoorSkipSpeed(8.0f);
    } else {
        door_cicle = 0;
        SetDoorSkipSpeed(1.0f);
    }
}

// ============================================================
// Worker & Hotkey threads
// ============================================================

void WorkerThread() {
    while (g_running) {
        if (g_enabled) {
            SkipCutsceneLogic();
        } else {
            timer_enabled = false;
            SetCutsceneSpeed(1.0f);
        }
        ProcessDoorSkip();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void HotkeyThread() {
    // Crear la cola de mensajes antes de publicar el ID del hilo.
    MSG message{};
    PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    g_hotkey_thread_id = GetCurrentThreadId();

    HHOOK keyboard_hook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        [](int code, WPARAM w_param, LPARAM l_param) -> LRESULT {
            if (code == HC_ACTION) {
                const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(l_param);
                const bool key_up = (key->flags & LLKHF_UP) != 0;

                if (key->vkCode == VK_F2) {
                    if (key_up) {
                        g_f2_pressed = false;
                    } else if (!g_f2_pressed.exchange(true)) {
                        g_enabled = !g_enabled.load();
                    }
                } else if (key->vkCode == VK_F11) {
                    if (key_up) {
                        g_f11_pressed = false;
                    } else if (!g_f11_pressed.exchange(true)) {
                        const bool enable_doorskip = !g_doorskip.load();
                        g_doorskip = enable_doorskip;
                        if (!enable_doorskip) {
                            SetDoorSkipSpeed(1.0f);
                        }
                    }
                }
            }
            return CallNextHookEx(nullptr, code, w_param, l_param);
        },
        g_module_handle,
        0);

    if (keyboard_hook) {
        while (g_running && GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        UnhookWindowsHookEx(keyboard_hook);
    }
    g_hotkey_thread_id = 0;
}

// ============================================================
// On-screen status overlay
// ============================================================

static constexpr wchar_t OVERLAY_CLASS_NAME[] = L"DinoCrisisSkipStatusOverlay";
static HWND g_overlay_window = nullptr;

BOOL CALLBACK FindGameWindowCallback(HWND window, LPARAM parameter) {
    DWORD process_id = 0;
    GetWindowThreadProcessId(window, &process_id);

    const LONG_PTR extended_style = GetWindowLongPtrW(window, GWL_EXSTYLE);
    if (process_id == GetCurrentProcessId() && window != g_overlay_window &&
        IsWindowVisible(window) && GetWindow(window, GW_OWNER) == nullptr &&
        (extended_style & WS_EX_TOOLWINDOW) == 0) {
        *reinterpret_cast<HWND*>(parameter) = window;
        return FALSE;
    }
    return TRUE;
}

LRESULT CALLBACK OverlayWindowProc(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    switch (message) {
    case WM_TIMER: {
        if (!g_running) {
            PostQuitMessage(0);
            return 0;
        }
        HWND game_window = nullptr;
        EnumWindows(FindGameWindowCallback, reinterpret_cast<LPARAM>(&game_window));
        HWND foreground_window = GetForegroundWindow();
        DWORD foreground_process_id = 0;
        if (foreground_window) {
            GetWindowThreadProcessId(foreground_window, &foreground_process_id);
        }
        if (!game_window || IsIconic(game_window) || !IsWindowVisible(game_window) ||
            foreground_process_id != GetCurrentProcessId()) {
            ShowWindow(window, SW_HIDE);
            return 0;
        }

        POINT client_origin{0, 0};
        if (ClientToScreen(game_window, &client_origin)) {
            SetWindowPos(window, HWND_TOPMOST, client_origin.x + 12, client_origin.y + 12,
                0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        InvalidateRect(window, nullptr, TRUE);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        SetBkMode(dc, TRANSPARENT);

        HFONT font = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ previous_font = SelectObject(dc, font);

        SetTextColor(dc, g_enabled.load() ? RGB(80, 255, 120) : RGB(190, 190, 190));
        TextOutW(dc, 0, 0, g_enabled.load() ? L"CUTSCENE SKIP: ON" : L"CUTSCENE SKIP: OFF",
            g_enabled.load() ? 17 : 18);
        SetTextColor(dc, g_doorskip.load() ? RGB(80, 255, 120) : RGB(190, 190, 190));
        TextOutW(dc, 0, 22, g_doorskip.load() ? L"DOORSKIP: ON" : L"DOORSKIP: OFF",
            g_doorskip.load() ? 12 : 13);

        SelectObject(dc, previous_font);
        DeleteObject(font);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(window, 1);
        return 0;
    default:
        return DefWindowProcW(window, message, w_param, l_param);
    }
}

void OverlayThread() {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = OverlayWindowProc;
    window_class.hInstance = instance;
    window_class.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    window_class.lpszClassName = OVERLAY_CLASS_NAME;
    RegisterClassExW(&window_class);

    g_overlay_window = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        OVERLAY_CLASS_NAME, L"Dino Crisis skip status", WS_POPUP,
        0, 0, 260, 48, nullptr, nullptr, instance, nullptr);
    if (!g_overlay_window) return;

    SetLayeredWindowAttributes(g_overlay_window, RGB(0, 0, 0), 0, LWA_COLORKEY);
    SetTimer(g_overlay_window, 1, 150, nullptr);

    MSG message{};
    while (g_running && GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    DestroyWindow(g_overlay_window);
    g_overlay_window = nullptr;
    UnregisterClassW(OVERLAY_CLASS_NAME, instance);
}

// ============================================================
// DLL Entry
// ============================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_module_handle = hModule;
        DisableThreadLibraryCalls(hModule);
        InitSpeedhack();
        g_running = true;
        std::thread(WorkerThread).detach();
        std::thread(HotkeyThread).detach();
        std::thread(OverlayThread).detach();
        break;
    case DLL_PROCESS_DETACH:
        g_running = false;
        g_enabled = false;
        g_doorskip = false;
        if (const DWORD hotkey_thread_id = g_hotkey_thread_id.load()) {
            PostThreadMessageW(hotkey_thread_id, WM_QUIT, 0, 0);
        }
        break;
    }
    return TRUE;
}
