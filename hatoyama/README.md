Base library for all ReC98 modern-system ports. Split into these parts:

* `logic`: Basic code with minimal dependencies. Can be used by headless game logic libraries.
* `api`: Common, DLL-exported game logic API code.
* `engine`: Full-blown, heavy-dependency code required for running an actual game.
