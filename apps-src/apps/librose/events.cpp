/* $Id: events.cpp 46186 2010-09-01 21:12:38Z silene $ */
/*
   Copyright (C) 2003 - 2010 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "global.hpp"

#include "clipboard.hpp"
#include "cursor.hpp"
#include "events.hpp"
#include "sound.hpp"
#include "video.hpp"
#include "game_end_exceptions.hpp"
#include "display.hpp"
#include "preferences.hpp"
#include "gui/widgets/settings.hpp"
#include "posix2.h"
#include "base_instance.hpp"

#include "SDL.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <utility>
#include <vector>

base_finger::base_finger()
	: pinch_distance_(0)
	, mouse_motions_(0)
	, pinch_noisc_time_(100)
	, last_pinch_ticks_(0)
{}

#define PINCH_SQUARE_THRESHOLD		6400
void base_finger::process_event(const SDL_Event& event)
{
	int x, y, dx, dy;
	bool hit = false;
	Uint8 mouse_flags;
	Uint32 now = SDL_GetTicks();

	unsigned screen_width2 = gui2::settings::screen_width;
	unsigned screen_height2 = gui2::settings::screen_height;
#if (defined(__APPLE__) && TARGET_OS_IPHONE)
	if (gui2::twidget::hdpi) {
		screen_width2 /= gui2::twidget::hdpi_scale;
		screen_height2 /= gui2::twidget::hdpi_scale;
	}
#endif

	switch(event.type) {
	case SDL_FINGERDOWN:
		x = event.tfinger.x * screen_width2;
		y = event.tfinger.y * screen_height2;

		// posix_print("SDL_FINGERDOWN, (%i, %i)\n", x, y);

		if (!finger_coordinate_valid(x, y)) {
			return;
		}
		fingers_.push_back(tfinger(event.tfinger.fingerId, x, y, now));
		break;

	case SDL_FINGERMOTION:
		{
			int x1 = 0, y1 = 0, x2 = 0, y2 = 0, at = 0;
			x = event.tfinger.x * screen_width2;
			y = event.tfinger.y * screen_height2;
			dx = event.tfinger.dx * screen_width2;
			dy = event.tfinger.dy * screen_height2;

			for (std::vector<tfinger>::iterator it = fingers_.begin(); it != fingers_.end(); ++ it, at ++) {
				tfinger& finger = *it;
				if (finger.fingerId == event.tfinger.fingerId) {
					finger.x = x;
					finger.y = y;
					finger.active = now;
					hit = true;
				}
				if (at == 0) {
					x1 = finger.x;
					y1 = finger.y;
				} else if (at == 1) {
					x2 = finger.x;
					y2 = finger.y;
				}
			}
			if (!hit) {
				return;
			}
			if (!finger_coordinate_valid(x, y)) {
				return;
			}
			
			if (fingers_.size() == 1) {
				handle_swipe(x, y, dx, dy);

			} else if (fingers_.size() == 2) {
				// calculate distance between finger
				int distance = (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
				if (pinch_distance_ != gui2::twidget::npos) {
					int diff = pinch_distance_ - distance;
					if (abs(diff) >= PINCH_SQUARE_THRESHOLD * gui2::twidget::hdpi_scale) {
						pinch_distance_ = distance;
						if (now - last_pinch_ticks_ > pinch_noisc_time_) {
							last_pinch_ticks_ = now;
							handle_pinch(x, y, diff > 0);
						}
					}
				} else {
					pinch_distance_ = distance;
				}
			}
		}
		break;

	case SDL_FINGERUP:
		for (std::vector<tfinger>::iterator it = fingers_.begin(); it != fingers_.end(); ) {
			const tfinger& finger = *it;
			if (finger.fingerId == event.tfinger.fingerId) {
				it = fingers_.erase(it);
			} else if (now > finger.active + 5000) {
				it = fingers_.erase(it);
			} else {
				++ it;
			}
		}
		break;

	case SDL_MULTIGESTURE:
		// Now I don't use SDL logic, process multi-finger myself. Ignore it.
		break;

	case SDL_MOUSEBUTTONDOWN:
		mouse_motions_ = 0;
		pinch_distance_ = gui2::twidget::npos;

		// posix_print("SDL_MOUSEBUTTONDOWN, (%i, %i)\n", event.button.x, event.button.y);
		handle_mouse_down(event.button);
		break;

	case SDL_MOUSEBUTTONUP:
		// 1. once one finger up, thank this finger end.
		// 2. solve mouse_up nest.
		fingers_.clear();

		handle_mouse_up(event.button);
		break;

	case SDL_MOUSEMOTION:
		mouse_motions_ ++;
		handle_mouse_motion(event.motion);
		break;

	case SDL_MOUSEWHEEL:
		if (event.wheel.which == SDL_TOUCH_MOUSEID) {
			break;
		}
		mouse_flags = SDL_GetMouseState(&x, &y);
		if (!mouse_wheel_coordinate_valid(x, y)) {
			return;
		}
#ifdef _WIN32
		if (mouse_flags & SDL_BUTTON(SDL_BUTTON_LEFT) && abs(event.wheel.y) >= MOUSE_MOTION_THRESHOLD) {
			// left mouse + wheel vetical ==> pinch
			mouse_motions_ ++;
			Uint32 now = SDL_GetTicks();
			if (now - last_pinch_ticks_ > pinch_noisc_time_) {
				last_pinch_ticks_ = now;
				handle_pinch(x, y, event.wheel.y > 0);
			}
			
		} else
#endif
		{
			handle_mouse_wheel(event.wheel, x, y, mouse_flags);
		}
		break;
	}
}

bool base_finger::multi_gestures() const
{
	if (fingers_.size() < 2) {
		return false;
	}
	return true;
}

namespace events
{

bool ignore_finger_event;

struct context
{
	context() :
		handlers(),
		focused_handler(-1)
	{
	}

	void add_handler(handler* ptr);
	void remove_handler(handler* ptr);

	// pump require enum all handler, so can not use std::stack
	std::vector<handler*> handlers;
	int focused_handler;
};

void context::add_handler(handler* ptr)
{
	handlers.push_back(ptr);
}

void context::remove_handler(handler* ptr)
{
	// now only exsit tow handler.
	// when tow handler, must first is controller_base, second is gui2::event::thandler. leave must inverted sequence.
	VALIDATE(handlers.back() == ptr, null_str);

	handlers.pop_back();
}
 
static context contexts;
std::vector<pump_monitor*> pump_monitors;

pump_monitor::pump_monitor(bool auto_join)
	: has_joined_(false)
{
	if (auto_join) {
		join();
	}
}

pump_monitor::~pump_monitor()
{
	pump_monitors.erase(
		std::remove(pump_monitors.begin(), pump_monitors.end(), this),
		pump_monitors.end());
}

void pump_monitor::join()
{
	if (has_joined_) {
		return;
	}
	pump_monitors.push_back(this);
	has_joined_ = true;
}

handler::handler(const bool auto_join) : has_joined_(false)
{
	if (auto_join) {
		contexts.add_handler(this);
		has_joined_ = true;
	}
}

handler::~handler()
{
	if (has_joined_) {
		leave();
	}
}

void handler::join()
{
	// must not join more!
	VALIDATE(!has_joined_, null_str);

	// join self
	contexts.add_handler(this);
	has_joined_ = true;
}

void handler::leave()
{
	VALIDATE(has_joined_ && !contexts.handlers.empty(), null_str);

	contexts.remove_handler(this);
	has_joined_ = false;
}

void dump_events(const std::vector<SDL_Event>& events)
{
	std::map<int, int> dump;
	for (std::vector<SDL_Event>::const_iterator it = events.begin(); it != events.end(); ++ it) {
		int type = it->type;

		std::map<int, int>::iterator find = dump.find(type);
		if (dump.find(type) != dump.end()) {
			find->second ++;
		} else {
			dump.insert(std::make_pair(type, 1));
		}
	}
	std::stringstream ss;
	for (std::map<int, int>::const_iterator it = dump.begin(); it != dump.end(); ++ it) {
		if (it != dump.begin()) {
			ss << "; ";
		}
		ss << "(type: " << it->first << ", count: " << it->second << ")";
	}
	posix_print("%s\n", ss.str().c_str());
}

void pump()
{
	if (instance->terminating()) {
		// let main thread throw quit exception.
		throw CVideo::quit();
	}

	SDL_Event temp_event;
	int poll_count = 0;
	int begin_ignoring = 0;
	ignore_finger_event = false;

	std::vector<SDL_Event> events;
	// ignore user input events when receive SDL_WINDOWEVENT. include before and after.
	while (SDL_PollEvent(&temp_event)) {
		++ poll_count;
		if (!begin_ignoring && temp_event.type == SDL_WINDOWEVENT) {
			begin_ignoring = poll_count;
		} else if (begin_ignoring > 0 && temp_event.type >= INPUT_MASK_MIN && temp_event.type <= INPUT_MASK_MAX) {
			//ignore user input events that occurred after the window was activated
			continue;
		}
		events.push_back(temp_event);
	}

	if (events.size() > 10) {
		posix_print("------waring!! events.size(): %u, last_event: %i\n", events.size(), events.back().type);
		dump_events(events);
	}

	std::vector<SDL_Event>::iterator ev_it = events.begin();
	for (int i = 1; i < begin_ignoring; ++i) {
		if (ev_it->type >= INPUT_MASK_MIN && ev_it->type <= INPUT_MASK_MAX) {
			//ignore user input events that occurred before the window was activated
			ev_it = events.erase(ev_it);
		} else {
			++ev_it;
		}
	}

	std::vector<SDL_Event>::iterator ev_end = events.end();
	for (ev_it = events.begin(); ev_it != ev_end; ++ev_it){
		SDL_Event& event = *ev_it;
		switch (event.type) {
			case SDL_APP_WILLENTERFOREGROUND:
			case SDL_APP_DIDENTERFOREGROUND:
				// first these event maybe send before call SDL_SetAppEventHandler at base_instance.
				break;
			case SDL_APP_TERMINATING:
			case SDL_APP_WILLENTERBACKGROUND:
			case SDL_APP_DIDENTERBACKGROUND:
			case SDL_QUIT:
				VALIDATE(false, "this event should be processed by SDL_SetAppEventHandler.");
				break;

			case SDL_APP_LOWMEMORY:
				instance->handle_app_event(event.type);
				break;

			case SDL_WINDOWEVENT:
				if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {

				} else if (event.window.event == SDL_WINDOWEVENT_ENTER || event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
					cursor::set_focus(true);

				} else if (event.window.event == SDL_WINDOWEVENT_LEAVE || event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
					cursor::set_focus(false);

				} else if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
					// if the window must be redrawn, update the entire screen

				} else if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
					
				}
				break;

			case SDL_MOUSEMOTION: {
				//always make sure a cursor is displayed if the
				//mouse moves or if the user clicks
				cursor::set_focus(true);
				break;
			}

			case SDL_MOUSEBUTTONDOWN: {
				//always make sure a cursor is displayed if the
				//mouse moves or if the user clicks
				cursor::set_focus(true);
				if (event.button.button == SDL_BUTTON_LEFT && event.button.clicks == 2) {
					SDL_UserEvent user_event;
					user_event.type = DOUBLE_CLICK_EVENT;
					user_event.code = 0;
					user_event.data1 = reinterpret_cast<void*>(event.button.x);
					user_event.data2 = reinterpret_cast<void*>(event.button.y);
					::SDL_PushEvent(reinterpret_cast<SDL_Event*>(&user_event));
				}
				break;
			}

#if defined(_X11) && !defined(__APPLE__)
			case SDL_SYSWMEVENT: {
				//clipboard support for X11
				handle_system_event(event);
				break;
			}
#endif
		}

		const std::vector<handler*>& event_handlers = contexts.handlers;

		// when tow handler exists together, must first is controller_base, second is gui2::event::thandler. 
		// change resolution and resize window maybe make second dirty.
		for (size_t i1 = 0, i2 = event_handlers.size(); i1 != i2 && i1 < event_handlers.size(); ++i1) {
			event_handlers[i1]->handle_event(event);
			if (display::require_change_resolution) {
				display* disp = display::get_singleton();
				disp->change_resolution();
			}
		}

	}

	instance->webrtc_pump();

	// inform the pump monitors that an events::pump() has occurred
	for (size_t i1 = 0, i2 = pump_monitors.size(); i1 != i2 && i1 < pump_monitors.size(); ++i1) {
		pump_monitors[i1]->monitor_process();
	}
}

void raise_process_event()
{
	const std::vector<handler*>& event_handlers = contexts.handlers;

	//events may cause more event handlers to be added and/or removed,
	//so we must use indexes instead of iterators here.
	for (size_t i1 = 0, i2 = event_handlers.size(); i1 != i2 && i1 < event_handlers.size(); ++i1) {
		event_handlers[i1]->process_event();
	}
}

void raise_draw_event()
{
	const std::vector<handler*>& event_handlers = contexts.handlers;

	// events may cause more event handlers to be added and/or removed,
	// so we must use indexes instead of iterators here.
	for (size_t i1 = 0, i2 = event_handlers.size(); i1 != i2 && i1 < event_handlers.size(); ++i1) {
		event_handlers[i1]->draw();
	}
}

int discard(Uint32 event_mask_min, Uint32 event_mask_max)
{
	int discard_count = 0;
	SDL_Event temp_event;
	std::vector< SDL_Event > keepers;
	SDL_Delay(10);
	while (SDL_PollEvent(&temp_event) > 0) {
		if (temp_event.type >= event_mask_min && temp_event.type <= event_mask_max) {
			keepers.push_back( temp_event );
		} else {
			++ discard_count;
		}
	}

	//FIXME: there is a chance new events are added before kept events are replaced
	for (unsigned int i = 0; i < keepers.size(); ++i) {
		if (SDL_PushEvent(&keepers[i]) <= 0) {
			posix_print("failed to return an event to the queue.");
		}
	}

	return discard_count;
}

} //end events namespace
