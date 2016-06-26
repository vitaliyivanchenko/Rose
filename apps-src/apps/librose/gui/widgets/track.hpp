/* $Id: button.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_WIDGETS_TRACK_HPP_INCLUDED
#define GUI_WIDGETS_TRACK_HPP_INCLUDED

#include "gui/widgets/control.hpp"
#include "gui/widgets/clickable.hpp"

namespace gui2 {

/**
 * Simple push button.
 */
class ttrack: public tcontrol
{
public:
	enum tstate { ENABLED, DISABLED, COUNT };
	enum tstate2 { NORMAL, FOCUSSED, PRESSED };

	ttrack();
	~ttrack();

	/***** ***** ***** ***** Inherited ***** ***** ***** *****/

	/** Inherited from tcontrol. */
	void set_active(const bool active);
		

	/** Inherited from tcontrol. */
	bool get_active() const { return state_ != DISABLED; }

	/** Inherited from tcontrol. */
	unsigned get_state() const { return state_; }

	/** Inherited from tclickable. */
	void connect_click_handler(const event::tsignal_function& signal)
	{
		connect_signal_mouse_left_click(*this, signal);
	}

	/** Inherited from tclickable. */
	void disconnect_click_handler(const event::tsignal_function& signal)
	{
		disconnect_signal_mouse_left_click(*this, signal);
	}

	void set_did_timer(const boost::function<void (ttrack&, const tpoint&, const bool)>& callback)
		{ did_timer_ = callback; }

	tpoint get_frame_offset() { return tpoint(last_x_offset_ + draw_offset_.x, last_y_offset_ + draw_offset_.y); }
	texture& background_texture() { return background_tex_; }
	tstate2 get_state2() const { return state2_; }

	void set_require_capture(bool val) { require_capture_ = val; }
	void set_timer_interval(int interval);

private:
	/***** ***** ***** setters / getters for members ***** ****** *****/
	void timer_handler();

	/** Inherited from tcontrol. */
	void impl_draw_background(
			  texture& frame_buffer
			, int x_offset
			, int y_offset);

	void clear_texture();
private:

	/**
	 * Possible states of the widget.
	 *
	 * Note the order of the states must be the same as defined in settings.hpp.
	 */
	void set_state(const tstate state);
	/**
	 * Current state of the widget.
	 *
	 * The state of the widget determines what to render and how the widget
	 * reacts to certain 'events'.
	 */
	tstate state_;

	void set_state2(const tstate2 state);

	tstate2 state2_;

	unsigned long timer_;
	bool drawn_;
	texture background_tex_;
	int last_x_offset_;
	int last_y_offset_;
	bool require_capture_;
	int timer_interval_;

	boost::function<void (ttrack&, const tpoint&, const bool)> did_timer_;

	/** Inherited from tcontrol. */
	const std::string& get_control_type() const;

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_mouse_enter(const event::tevent event, bool& handled);

	void signal_handler_mouse_leave(const event::tevent event, bool& handled);

	void signal_handler_left_button_down(
			const event::tevent event, bool& handled);

	void signal_handler_left_button_up(
			const event::tevent event, bool& handled);
};

} // namespace gui2

#endif

