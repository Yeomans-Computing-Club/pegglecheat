#pragma once
#include <cstdint>

namespace Utilities {

	inline std::uintptr_t ReadPtr(std::uintptr_t address, std::uintptr_t offset) {
		return *reinterpret_cast<std::uintptr_t*>(address + offset);
	}

	inline float ReadFloat(std::uintptr_t address, std::uintptr_t offset) {
		return *reinterpret_cast<float*>(address + offset);
	}

	inline void WriteFloat(std::uintptr_t address, std::uintptr_t offset, float value) {
		*reinterpret_cast<float*>(address + offset) = value;
	}

	inline void WriteByte(std::uintptr_t address, std::uintptr_t offset, std::uint8_t value) {
		*reinterpret_cast<std::uint8_t*>(address + offset) = value;
	}
}

namespace Peggle {

	inline constexpr std::uintptr_t GameBase = 0x6873AC;

	// OFFSETS
	namespace Game { inline constexpr std::uintptr_t Board = 0x7B8; }
	namespace Board { inline constexpr std::uintptr_t LogicMgr = 0x154; }
	namespace LogicMgr {
		inline constexpr std::uintptr_t AimAngle = 0xE0;  // the player's aim; the search sweeps around this
		inline constexpr std::uintptr_t BestAngle = 0xE4; // written by the best shot search
		inline constexpr std::uintptr_t AutoFire = 0xEC;  // the search sets this so the CPU fires; we fire ourselves
	}

	inline std::uintptr_t GetBoard() {
		std::uintptr_t game = Utilities::ReadPtr(GameBase, 0);
		return game ? Utilities::ReadPtr(game, Game::Board) : 0;
	}

	// FUNCTIONS WE HOOK
	inline constexpr std::uintptr_t GetTunableInt = 0x523C70;   // int GetTunableInt(int, key, defaultValue)
	inline constexpr std::uintptr_t GetTunableFloat = 0x523CD0; // float GetTunableFloat(int, key, defaultValue)
	inline constexpr std::uintptr_t FireGun = 0x484530;         // void __thiscall Gun::Fire(Ball** ballOut, int)

	// the keys the Zen ball's search uses to look up how many angles to try, and how far apart
	inline constexpr std::uintptr_t ZenSweepCountKey = 0x5EB9B4; // "AIMgr.cpp18", default 120
	inline constexpr std::uintptr_t ZenSweepStepKey = 0x5EB978;  // "AIMgr.cpp19", default 0.001 radians

	// FUNCTIONS WE CALL
	// void __thiscall LogicMgr::RunBestShotSearch(bool nudge) - the Zen ball's search, finishes in one frame
	typedef void(__fastcall* RunBestShotSearchFn)(std::uintptr_t logicMgr, std::uintptr_t edx, bool nudge);
	inline RunBestShotSearchFn RunBestShotSearch = (RunBestShotSearchFn)0x467B80;

	// void __thiscall Gun::SetAngle(float angle, bool) - the Zen ball passes true
	typedef void(__fastcall* SetGunAngleFn)(std::uintptr_t gun, std::uintptr_t edx, float angle, bool unknown);
	inline SetGunAngleFn SetGunAngle = (SetGunAngleFn)0x4843A0;
}

namespace Hooks {
	// edx is just a dummy in the __fastcall hooks.
	// it is there because the compiler will not let us write a _thiscall here

	// gun angles: 0 points right, counterclockwise, so straight down is 270 degrees (3pi/2 radians).
	// the gun can aim anywhere except the arc over the top from 7 to 173 degrees: 194 degrees, or +-1.693 radians around straight down.
	// the Zen search tries +-120 angles 0.001 apart (+-7 degrees); we widen it to cover the gun's whole range.
	inline float SweepStep = 0.002f;                     // smaller = finer search, but slower
	inline int SweepCount = (int)(1.693f / SweepStep);  // angles on each side of straight down

	typedef int(__cdecl* GetTunableIntFn)(int unused, std::uintptr_t key, int defaultValue);
	inline GetTunableIntFn OriginalGetTunableInt;

	inline int __cdecl HookedGetTunableInt(int unused, std::uintptr_t key, int defaultValue) {
		if (key == Peggle::ZenSweepCountKey) return SweepCount;
		return OriginalGetTunableInt(unused, key, defaultValue);
	}

	typedef float(__cdecl* GetTunableFloatFn)(int unused, std::uintptr_t key, float defaultValue);
	inline GetTunableFloatFn OriginalGetTunableFloat;

	inline float __cdecl HookedGetTunableFloat(int unused, std::uintptr_t key, float defaultValue) {
		if (key == Peggle::ZenSweepStepKey) return SweepStep;
		return OriginalGetTunableFloat(unused, key, defaultValue);
	}

	// every shot: run the Zen search centered on straight down, aim the gun at the best angle, then fire
	typedef void(__fastcall* FireGunFn)(std::uintptr_t gun, std::uintptr_t edx, std::uintptr_t ballOut, int unknown);
	inline FireGunFn OriginalFireGun;

	inline void __fastcall HookedFireGun(std::uintptr_t gun, std::uintptr_t edx, std::uintptr_t ballOut, int unknown) {
		std::uintptr_t logicMgr = Utilities::ReadPtr(Peggle::GetBoard(), Peggle::Board::LogicMgr);
		float playerAim = Utilities::ReadFloat(logicMgr, Peggle::LogicMgr::AimAngle);

		Utilities::WriteFloat(logicMgr, Peggle::LogicMgr::AimAngle, 4.7124f); // straight down (270 degrees)
		Peggle::RunBestShotSearch(logicMgr, 0, false);
		Utilities::WriteFloat(logicMgr, Peggle::LogicMgr::AimAngle, playerAim);
		Utilities::WriteByte(logicMgr, Peggle::LogicMgr::AutoFire, 0);
		Peggle::SetGunAngle(gun, 0, Utilities::ReadFloat(logicMgr, Peggle::LogicMgr::BestAngle), true);

		OriginalFireGun(gun, edx, ballOut, unknown);
	}
}
