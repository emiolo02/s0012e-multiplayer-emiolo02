#pragma once
//------------------------------------------------------------------------------
/**
	Space game application

	(C) 20222 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/app.h"
#include "render/window.h"
#include "client.h"
#include "server.h"
#include "render/renderdevice.h"

namespace Game {
	class SpaceGameApp : public Core::App {
	public:
		/// constructor
		SpaceGameApp();

		/// destructor
		~SpaceGameApp();

		/// open app
		bool Open();

		/// run app
		void Run();

		/// exit app
		void Exit();

	private:
		/// show some ui things
		void RenderUI();

		std::unordered_map<uint32, SpaceShip> m_SpaceShips;
		std::unordered_map<uint32, Laser> m_Lasers;
		std::vector<std::pair<Render::ModelId, mat4> > m_Asteroids;
		Display::Window *window = nullptr;

		bool m_IsHost = false;
	};
} // namespace Game
