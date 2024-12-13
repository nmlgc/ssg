/*
 *   Input via SDL events
 *
 */

#include "platform/input.h"
#include "game/enum_flags.h"
#include <SDL_events.h>
#include <SDL_joystick.h>
#include <assert.h>

// Do scancodes and key modes still fit into 16 bits each?
static_assert(SDL_NUM_SCANCODES <= std::numeric_limits<uint16_t>::max());
static_assert(KMOD_SCROLL <= std::numeric_limits<uint16_t>::max());

enum class KEY_MOD : uint8_t {
	_HAS_BITFLAG_OPERATORS,
	NONE = 0x00,
	LALT = 0x01,
};

// In-game scancode, received from SDL.
struct KEY_SCANCODE {
	uint16_t scancode;
	KEY_MOD mod;
};

// Static key binding.
struct KEY_BIND {
	uint16_t scancode;

	// Modifiers that *must* match.
	KEY_MOD mod_must = KEY_MOD::NONE;

	// Modifiers that *must not* match. Should contain all modifiers of other
	// bindings that share the same [scancode] to allow the matching to work
	// independently of the order within the binding array.
	KEY_MOD mod_filter = KEY_MOD::NONE;

	constexpr KEY_BIND(
		SDL_Scancode scancode,
		KEY_MOD mod_must = KEY_MOD::NONE,
		KEY_MOD mod_filter = KEY_MOD::NONE
	) : scancode(scancode), mod_must(mod_must), mod_filter(mod_filter) {
	}

	bool Matches(const KEY_SCANCODE& sc) const {
		return (
			(scancode == sc.scancode) &&
			!(sc.mod & mod_filter) &&
			((mod_must & sc.mod) == mod_must)
		);
	}
};

static constexpr std::pair<KEY_BIND, INPUT_BITS> KeyBindings[] = {
	{ SDL_SCANCODE_UP,    	KEY_UP },
	{ SDL_SCANCODE_DOWN,  	KEY_DOWN },
	{ SDL_SCANCODE_LEFT,  	KEY_LEFT },
	{ SDL_SCANCODE_RIGHT, 	KEY_RIGHT },
	{ SDL_SCANCODE_KP_1,  	KEY_DLEFT },
	{ SDL_SCANCODE_KP_2,  	KEY_DOWN },
	{ SDL_SCANCODE_KP_3,  	KEY_DRIGHT },
	{ SDL_SCANCODE_KP_4,  	KEY_LEFT },
	{ SDL_SCANCODE_KP_6,  	KEY_RIGHT },
	{ SDL_SCANCODE_KP_7,  	KEY_ULEFT },
	{ SDL_SCANCODE_KP_8,  	KEY_UP },
	{ SDL_SCANCODE_KP_9,  	KEY_URIGHT },
	{ SDL_SCANCODE_Z,     	KEY_TAMA },
	{ SDL_SCANCODE_X,     	KEY_BOMB },
	{ SDL_SCANCODE_LSHIFT,	KEY_SHIFT },
	{ SDL_SCANCODE_ESCAPE,	KEY_ESC },

	{ { SDL_SCANCODE_RETURN, KEY_MOD::NONE, KEY_MOD::LALT }, KEY_RETURN },
};
static constexpr std::pair<KEY_BIND, INPUT_SYSTEM_BITS> SystemKeyBindings[] = {
	{ SDL_SCANCODE_P,    	SYSKEY_SNAPSHOT },
	{ SDL_SCANCODE_LCTRL,	SYSKEY_SKIP },
	{ SDL_SCANCODE_RCTRL,	SYSKEY_SKIP },
	{ SDL_SCANCODE_F,    	SYSKEY_BGM_FADE },
	{ SDL_SCANCODE_D,    	SYSKEY_BGM_DEVICE },
	{ SDL_SCANCODE_F10,  	SYSKEY_GRP_SCALE_DOWN },
	{ SDL_SCANCODE_F11,  	SYSKEY_GRP_SCALE_UP },
	{ SDL_SCANCODE_F9,   	SYSKEY_GRP_SCALE_MODE },
	{ SDL_SCANCODE_F8,   	SYSKEY_GRP_TURBO },

	{ { SDL_SCANCODE_RETURN, KEY_MOD::LALT }, SYSKEY_GRP_FULLSCREEN },
};

template <class Bits> void Key_Flip(Bits& key_data, uint8_t state, Bits bits)
{
	if(state == SDL_PRESSED) {
		key_data |= bits;
	} else if(state == SDL_RELEASED) {
		key_data &= ~bits;
	}
}

struct JOYPAD {
	SDL_Joystick* joystick;
	uint8_t axis_x;
	uint8_t axis_y;
	INPUT_PAD_BUTTON button_pressed_last = 0;
	uint8_t buttons_held = 0;
};

static std::vector<JOYPAD> Pads;

void Pad_Open(int device_index)
{
	auto* joystick = SDL_JoystickOpen(device_index);
	if(!joystick) {
		return;
	}

	// SDL provides two abstractions for joypads:
	//
	// • SDL_Joystick, which uses numbered axes or buttons with no semantic
	//   meaning, but works with every joypad ever
	// • SDL_GameController, which provides a standardized interface for the
	//   typical kind of modern gamepad with two analog sticks, a D-pad, and at
	//   least 4 face and 2 shoulder buttons, but doesn't work with joypads
	//   that don't match this standard
	//
	// For buttons, SDL_Joystick's numbered IDs are exactly what we want. WinMM
	// does the same, and the Joy Pad menu is designed around that. For axes,
	// however, there is no UI, and SDL_Joystick provides no way of identifying
	// which axis IDs correspond to a joypad's "main" X/Y axis, so we can only
	// guess. Luckily, all joypads we've tested map their main X axis to ID 0
	// and their main Y axis to ID 1.
	//
	// But we can still *try* opening a joypad via SDL_GameController. If that
	// succeeds, we can correct this initial guess with the actual SDL_Joystick
	// axis IDs (= the "bind") for the standard X and Y axes:
	//
	// 	https://discourse.libsdl.org/t/difference-between-joysticks-and-game-controllers/24028/2
	decltype(JOYPAD::axis_x) axis_x = 0;
	decltype(JOYPAD::axis_y) axis_y = 1;

	if(SDL_IsGameController(device_index)) {
		auto* controller = SDL_GameControllerOpen(device_index);
		if(controller) {
			SDL_GameControllerButtonBind bind;
			bind = SDL_GameControllerGetBindForAxis(
				controller, SDL_CONTROLLER_AXIS_LEFTX
			);
			if(bind.bindType == SDL_CONTROLLER_BINDTYPE_AXIS) {
				axis_x = bind.value.axis;
			}
			bind = SDL_GameControllerGetBindForAxis(
				controller, SDL_CONTROLLER_AXIS_LEFTY
			);
			if(bind.bindType == SDL_CONTROLLER_BINDTYPE_AXIS) {
				axis_y = bind.value.axis;
			}
			SDL_GameControllerClose(controller);
		}
	}
	Pads.emplace_back(JOYPAD{
		.joystick = joystick,
		.axis_x = axis_x,
		.axis_y = axis_y,
	});
}

std::vector<JOYPAD>::iterator Pad_Find(SDL_JoystickID id)
{
	auto* joystick = SDL_JoystickFromInstanceID(id);
	const auto it = std::ranges::find_if(Pads, [joystick](const auto& pad) {
		return (pad.joystick == joystick);
	});
	assert(it != Pads.end());
	return it;
}

bool Key_Start(void)
{
	// No need to call Pad_Open() here. SDL will send a SDL_JOYDEVICEADDED
	// event for every joystick attached at startup.

	SDL_StopTextInput();
	return true;
}

void Key_End(void)
{
	for(auto& pad : Pads) {
		SDL_JoystickClose(pad.joystick);
	}
	Pads.clear();
}

void Key_Read(void)
{
	static INPUT_BITS Key_Data_Real = 0;

	SDL_Event event;
	while(SDL_PeepEvents(
		&event, 1, SDL_GETEVENT, SDL_KEYDOWN, SDL_JOYDEVICEREMOVED
	) == 1) {
		switch(event.type) {
		case SDL_JOYAXISMOTION: {
			auto& pad = *Pad_Find(event.jaxis.which);

			// The original WinMM backend did this without even taking the
			// range reported by joyGetDevCaps() into account. However, that
			// function returns a range of [0, 65535] with all joypads I've
			// tried, which matches SDL's fixed [-32768, 32767] range, so we
			// can do it here as well.
			const auto v = (event.jaxis.value >> 11);
			if(event.jaxis.axis == pad.axis_x) {
				Pad_Data &= ~(KEY_LEFT | KEY_RIGHT);
				Pad_Data |= ((v <= -4) ? KEY_LEFT : ((v >= 4) ? KEY_RIGHT : 0));
			} else if(event.jaxis.axis == pad.axis_y) {
				Pad_Data &= ~(KEY_UP | KEY_DOWN);
				Pad_Data |= ((v <= -4) ? KEY_UP : ((v >= 4) ? KEY_DOWN : 0));
			}
			break;
		}

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP: {
			// SDL's numbering starts at 0.
			const INPUT_PAD_BUTTON id = (event.jbutton.button + 1);
			for(const auto& binding : Key_PadBindings) {
				if(id == binding.first) {
					Key_Flip(Pad_Data, event.jbutton.state, binding.second);
				}
			}

			auto& pad = *Pad_Find(event.jbutton.which);
			using HELD = std::numeric_limits<decltype(pad.button_pressed_last)>;
			if(event.jbutton.state == SDL_PRESSED) {
				if(pad.buttons_held < HELD::max()) {
					pad.buttons_held++;
				}
				pad.button_pressed_last = id;
			} else if(event.jbutton.state == SDL_RELEASED) {
				if(pad.buttons_held > HELD::min()) {
					pad.buttons_held--;
				}
			}
			break;
		}

		case SDL_KEYDOWN:
		case SDL_KEYUP: {
			const auto mod = SDL_GetModState();

			const KEY_SCANCODE scancode = {
				.scancode = static_cast<uint16_t>(event.key.keysym.scancode),
				.mod = ((mod & KMOD_LALT) ? KEY_MOD::LALT : KEY_MOD::NONE),
			};
			for(const auto& binding : KeyBindings) {
				if(binding.first.Matches(scancode)) {
					Key_Flip(Key_Data_Real, event.key.state, binding.second);
				}
			}
			for(const auto& binding : SystemKeyBindings) {
				if(binding.first.Matches(scancode)) {
					Key_Flip(SystemKey_Data, event.key.state, binding.second);
				}
			}
			break;
		}

		case SDL_JOYDEVICEREMOVED: {
			const auto pad = Pad_Find(event.jdevice.which);
			SDL_JoystickClose(pad->joystick);
			if(pad != (Pads.end() - 1)) {
				*pad = Pads.back();
			}
			Pads.pop_back();
			break;
		}

		case SDL_JOYDEVICEADDED:
			Pad_Open(event.jdevice.which);
			break;

		default:
			break;
		}
	}
	Key_Data = (Key_Data_Real | Pad_Data);
}

std::optional<INPUT_PAD_BUTTON> Key_PadSingle(void)
{
	for(const auto& pad : Pads) {
		if(pad.buttons_held > 1) {
			return 0;
		} else if(pad.buttons_held == 1) {
			return pad.button_pressed_last;
		}
	}
	return std::nullopt;
}
