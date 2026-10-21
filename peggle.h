#pragma once
#include <cstdint>
#include <utility>
#include <vector>

namespace Utilities {

	inline std::uintptr_t ReadPtr(std::uintptr_t address, std::uintptr_t offset) {
		return *reinterpret_cast<std::uintptr_t*>(address + offset);
	}

	inline float ReadFloat(std::uintptr_t address, std::uintptr_t offset) {
		return *reinterpret_cast<float*>(address + offset);
	}

	inline std::uint8_t ReadByte(std::uintptr_t address, std::uintptr_t offset) {
		return *reinterpret_cast<std::uint8_t*>(address + offset);
	}

	inline void WriteFloat(std::uintptr_t address, std::uintptr_t offset, float value) {
		*reinterpret_cast<float*>(address + offset) = value;
	}
}

namespace Peggle {

	inline constexpr std::uintptr_t GameBase = 0x6873AC;
	inline constexpr std::uintptr_t BallVTable = 0x5F19B4;
	inline constexpr std::uintptr_t BallSetVel = BallVTable + 0x4C; // should contain 0x475600
	inline constexpr std::uintptr_t BallUpdate = BallVTable + 0x58; // should contain 0x47A450

	// OFFSETS
	namespace Game { inline constexpr std::uintptr_t Board = 0x7B8; }

	namespace Board { 
		inline constexpr std::uintptr_t Objects = 0x190; 
		inline constexpr std::uintptr_t Balls = 0x19C; }

	namespace PegList {
		// is a std::list
		inline constexpr std::uintptr_t Head = 0x4;
		inline constexpr std::uintptr_t Size = 0x8;
	}

	namespace Node { // a node in the linked list of pegs
		inline constexpr std::uintptr_t Next = 0x0;
		inline constexpr std::uintptr_t Value = 0x8;
	}

	namespace PhysObj {
		inline constexpr std::uintptr_t MinX = 0x14;
		inline constexpr std::uintptr_t MinY = 0x18;
		inline constexpr std::uintptr_t MaxX = 0x1C;
		inline constexpr std::uintptr_t MaxY = 0x20;
		inline constexpr std::uintptr_t Active = 0x25;
		inline constexpr std::uintptr_t PegInfo = 0xD0;
		inline constexpr std::uintptr_t X = 0xEC;
		inline constexpr std::uintptr_t Y = 0xF0;
		inline constexpr std::uintptr_t VX = 0xFC;
		inline constexpr std::uintptr_t VY = 0x100;
	}

	namespace PegInfo {
		inline constexpr std::uintptr_t Type = 0x10;
		inline constexpr std::uintptr_t Lit = 0x14;
	}

	namespace PegType { // values of PegInfo::Type
		inline constexpr int Blue = 1;
		inline constexpr int Orange = 2;
		inline constexpr int Purple = 3;
		inline constexpr int Green = 4;
	}

	inline std::uintptr_t GetBoard() {
		std::uintptr_t game = Utilities::ReadPtr(GameBase, 0);
		return game ? Utilities::ReadPtr(game, Game::Board) : 0;
	}

	// One peg, copied out of game memory.
	struct Peg {
		std::uintptr_t obj;  // address of the peg's PhysObj
		int type;            // PegType::Blue / Orange / Purple / Green
		bool lit;            // hit this shot
		bool active;         // false = despawned (pegs are never removed from the list)
		float x, y;          // centre of the bounding box
	};

	// First live ball, or 0 if no ball is in play.
	inline std::uintptr_t GetFirstBall() {
		std::uintptr_t board = GetBoard();
		if (board == 0)
			return 0;

		std::uintptr_t list = board + Board::Balls;
		std::uintptr_t head = Utilities::ReadPtr(list, PegList::Head);
		std::uintptr_t first = Utilities::ReadPtr(head, Node::Next);
		if (first == head) // empty list: head points back to itself
			return 0;
		return Utilities::ReadPtr(first, Node::Value);
	}

	// access the list of pegs
	// Walks Board+0x190 (every physics object) and returns the ones that are pegs.
	// Empty if no board exists (menus, loading).
	inline std::vector<Peg> TraversePegList() {
		std::vector<Peg> pegs;

		std::uintptr_t board = GetBoard();
		if (board == 0)
			return pegs;

		std::uintptr_t list = board + Board::Objects;
		std::uintptr_t head = Utilities::ReadPtr(list, PegList::Head);
		if (head == 0)
			return pegs;

		// Safety cap: if we ever read a bad pointer, stop instead of looping forever.
		int guard = 0;
		for (std::uintptr_t node = Utilities::ReadPtr(head, Node::Next);
			 node != head && node != 0 && guard < 10000;
			 node = Utilities::ReadPtr(node, Node::Next), ++guard)
		{
			std::uintptr_t obj = Utilities::ReadPtr(node, Node::Value);
			std::uintptr_t info = Utilities::ReadPtr(obj, PhysObj::PegInfo);
			if (info == 0) // walls, bucket, the ball itself, ...
				continue;

			Peg peg;
			peg.obj = obj;
			peg.type = static_cast<int>(Utilities::ReadPtr(info, PegInfo::Type));
			peg.lit = Utilities::ReadByte(info, PegInfo::Lit) != 0;
			peg.active = Utilities::ReadByte(obj, PhysObj::Active) != 0;
			peg.x = (Utilities::ReadFloat(obj, PhysObj::MinX) + Utilities::ReadFloat(obj, PhysObj::MaxX)) * 0.5f;
			peg.y = (Utilities::ReadFloat(obj, PhysObj::MinY) + Utilities::ReadFloat(obj, PhysObj::MaxY)) * 0.5f;
			pegs.push_back(peg);
		}
		return pegs;
	}

	// find the one closest to the ball and return its coords
	// ball: the ball's address (in the hooks, it's the `this` pointer the game passes in ECX).
	// Only considers pegs that are still on the board and not yet hit.
	// orangeOnly: if true, ignore blue/purple/green pegs.
	// Returns false (and leaves outPos untouched) if there is no such peg.
	inline bool GetNearestPegPosition(std::uintptr_t ball, std::pair<float, float>& outPos, bool orangeOnly = false) {
		if (ball == 0)
			return false;

		float ballX = Utilities::ReadFloat(ball, PhysObj::X);
		float ballY = Utilities::ReadFloat(ball, PhysObj::Y);

		bool found = false;
		float bestDistSq = 0.0f;

		for (const Peg& peg : TraversePegList()) {
			if (!peg.active || peg.lit)
				continue;
			if (orangeOnly && peg.type != PegType::Orange)
				continue;

			float dx = peg.x - ballX;
			float dy = peg.y - ballY;
			float distSq = dx * dx + dy * dy; // squared distance is enough to compare; no sqrt needed

			if (!found || distSq < bestDistSq) {
				found = true;
				bestDistSq = distSq;
				outPos = { peg.x, peg.y };
			}
		}
		return found;
	}
}

namespace Hooks {

	// FUN_00475600 - set physobj velocity
	// void __thiscall PhysObj_SetVelocity_vf4C(int param_1,undefined4 param_2,undefined4 param_3)
	inline constexpr void _fastcall HookBallVelocity() {
		// override ball velocity here
	}

	// FUN_0047A450 - ball update
	// void __fastcall Ball_Update_vf58(int param_1)

}