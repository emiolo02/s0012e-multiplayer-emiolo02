//------------------------------------------------------------------------------
// main.cc
// (C) 2015-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "config.h"
#include "spacegameapp.h"

using namespace std::chrono;
using namespace std::chrono_literals;

milliseconds Time::start = 0ms;

int
main(int argc, const char **argv) {
	if (argc == 2) {
		Time::start = milliseconds(std::stoi(argv[1]));
	}
	Game::SpaceGameApp app;
	if (app.Open()) {
		app.Run();
		app.Close();
	}
	app.Exit();
}
