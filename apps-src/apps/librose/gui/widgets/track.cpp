/* $Id: button.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2008 - 2012 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "rose-lib"

#include "gui/widgets/track.hpp"

#include "gui/auxiliary/widget_definition/track.hpp"
#include "gui/auxiliary/window_builder/track.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/auxiliary/timer.hpp"

#include <boost/bind.hpp>

namespace gui2 {

REGISTER_WIDGET(track)

ttrack::ttrack()
	: tcontrol(COUNT)
	, state_(ENABLED)
	, state2_(NORMAL)
	, drawn_(false)
	, timer_(INVALID_TIMER_ID)
	, last_x_offset_(0)
	, last_y_offset_(0)
	, require_capture_(true)
	, timer_interval_(0)
{
	connect_signal<event::MOUSE_ENTER>(boost::bind(
				&ttrack::signal_handler_mouse_enter, this, _2, _3));
	connect_signal<event::MOUSE_LEAVE>(boost::bind(
				&ttrack::signal_handler_mouse_leave, this, _2, _3));

	connect_signal<event::LEFT_BUTTON_DOWN>(boost::bind(
				&ttrack::signal_handler_left_button_down, this, _2, _3));
	connect_signal<event::LEFT_BUTTON_UP>(boost::bind(
				&ttrack::signal_handler_left_button_up, this, _2, _3));
}

ttrack::~ttrack()
{
	if (timer_ != INVALID_TIMER_ID) {
		remove_timer(timer_);
	}
}

void ttrack::set_active(const bool active)
{ 
	if (get_active() != active) {
		set_state(active ? ENABLED : DISABLED); 
	}
};

void ttrack::set_state(const tstate state)
{
	if (state != state_) {
		state_ = state;
		set_dirty(true);
	}
}

void ttrack::set_state2(const tstate2 state)
{
	if (state != state2_) {
		state2_ = state;
		// set_dirty(true);
	}
}

void ttrack::impl_draw_background(texture& frame_buffer, int x_offset, int y_offset)
{
	tcontrol::impl_draw_background(frame_buffer, x_offset, y_offset);

	if (background_tex_) {
		int width, height;
		SDL_QueryTexture(background_tex_.get(), NULL, NULL, &width, &height);
		if (width != w_ || height != h_) {
			background_tex_ = NULL;
		}
	}
	if (!background_tex_.get()) {
		const SDL_Rect rc = get_rect();
		texture_from_texture(frame_buffer, background_tex_, &rc, 0, 0);
	}

	last_x_offset_ = x_offset;
	last_y_offset_ = y_offset;
	if (did_timer_) {
		did_timer_(*this, get_frame_offset(), false);
	}
}

void ttrack::timer_handler()
{
	if (did_timer_ && background_tex_) {
		const SDL_Rect rect = get_rect();
		texture_clip_rect_setter clip(&rect);
		did_timer_(*this, get_frame_offset(), true);
	}	
}

void ttrack::set_timer_interval(int interval) 
{ 
	VALIDATE(interval >= 0, null_str);
	if (timer_interval_ != interval) {
		if (timer_ != INVALID_TIMER_ID) {
			remove_timer(timer_);
			timer_ = INVALID_TIMER_ID;
		}
		if (interval) {
			timer_ = add_timer(interval, boost::bind(&ttrack::timer_handler, this), true);
		}
		timer_interval_ = interval;
	}
}

void ttrack::clear_texture()
{
	tcontrol::clear_texture();
	if (background_tex_.get()) {
		background_tex_ = NULL;
	}
}

const std::string& ttrack::get_control_type() const
{
	static const std::string type = "track";
	return type;
}

void ttrack::signal_handler_mouse_enter(
		const event::tevent event, bool& handled)
{
	if (!get_active()) {
		handled = true;
		return;
	}

	set_state2(FOCUSSED);
	handled = true;
}

void ttrack::signal_handler_mouse_leave(
		const event::tevent event, bool& handled)
{
	if (!get_active()) {
		handled = true;
		return;
	}
	set_state2(NORMAL);
	handled = true;
}

void ttrack::signal_handler_left_button_down(
		const event::tevent event, bool& handled)
{
	twindow* window = get_window();
	if (window && require_capture_) {
		window->mouse_capture();
	}

	set_state2(PRESSED);
	handled = true;
}

void ttrack::signal_handler_left_button_up(
		const event::tevent event, bool& handled)
{
	set_state2(FOCUSSED);
	handled = true;
}

} // namespace gui2
