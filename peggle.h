#pragma once
#include <cstdint>
#include <cfloat>

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
		// actually the peg list is all physics objects, not just pegs. oops
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
		// x,y and vx,vy stuff is only for the ball, not valid on bricks
		inline constexpr std::uintptr_t X = 0xEC;
		inline constexpr std::uintptr_t Y = 0xF0;
		inline constexpr std::uintptr_t VX = 0xFC;
		inline constexpr std::uintptr_t VY = 0x100;
	}

	inline std::uintptr_t GetBoard() {
		std::uintptr_t game = Utilities::ReadPtr(GameBase, 0);
		return game ? Utilities::ReadPtr(game, Game::Board) : 0;
	}

	struct Vec2 { float x, y; };

	inline Vec2 GetVectorToNearestPeg(std::uintptr_t ball) {
		float ballX = Utilities::ReadFloat(ball, PhysObj::X);
		float ballY = Utilities::ReadFloat(ball, PhysObj::Y);

		Vec2 nearestPegVector = { 0, 0 };
		float nearestPegDistance = FLT_MAX;

		// traversing the list
		std::uintptr_t head = Utilities::ReadPtr(GetBoard() + Board::Objects, PegList::Head);
		for (std::uintptr_t node = Utilities::ReadPtr(head, Node::Next); node != head; node = Utilities::ReadPtr(node, Node::Next)) {
			std::uintptr_t peg = Utilities::ReadPtr(node, Node::Value);

			if (Utilities::ReadPtr(peg, PhysObj::PegInfo) == 0) continue;  // not a peg
			if (Utilities::ReadByte(peg, PhysObj::Active) == 0) continue;  // despawned

			float pegX = (Utilities::ReadFloat(peg, PhysObj::MinX) + Utilities::ReadFloat(peg, PhysObj::MaxX)) / 2;
			float pegY = (Utilities::ReadFloat(peg, PhysObj::MinY) + Utilities::ReadFloat(peg, PhysObj::MaxY)) / 2;
			float dist = (pegX - ballX) * (pegX - ballX) + (pegY - ballY) * (pegY - ballY);

			// vector from the ball to the closest peg so far
			if (dist < nearestPegDistance) {
				nearestPegDistance = dist;
				nearestPegVector = { pegX - ballX, pegY - ballY };
			}
		}
		return nearestPegVector;
	}
}

namespace Hooks {

	// FUN_00475600 - set physobj velocity
	// void __thiscall FUN_00475600(int param_1,undefined4 param_2,undefined4 param_3)
	inline void _fastcall HookedSetBallVelocity(std::uintptr_t ball, std::uintptr_t edx) {
		// edx is just a dummy that we don't need to use.
		// it is there because the compiler will not let us write a _thiscall here

		// override ball velocity here
	}

	// FUN_0047A450 - ball update
	// void __fastcall FUN_0047A450(int param_1)
	inline void _fastcall HookedBallUpdate(std::uintptr_t ball, std::uintptr_t edx) {
		// override ball update here
	}
}