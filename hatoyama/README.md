Base library for all ReC98 modern-system ports. Split into these parts:

* `logic`: Basic code with minimal dependencies. Can be used by headless game logic libraries.
* `api`: Common, DLL-exported game logic API code.
* `app`: Platform code that you'd want to use in both CLIs and the game itself. Depends on SDL.
* `engine`: Full-blown, heavy-dependency code required for running an actual game.
