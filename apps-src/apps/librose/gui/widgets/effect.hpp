/* $Id: title_screen.hpp 48740 2011-03-05 10:01:34Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_WIDGETS_EFFECT_HPP_INCLUDED
#define GUI_WIDGETS_EFFECT_HPP_INCLUDED

#include "gui/widgets/control.hpp"
#include "gui/widgets/stacked_widget.hpp"
#include "area_anim.hpp"

class display;

namespace gui2 {

class ttrack;

namespace effect {
class trefresh
{
public:
	enum {norefresh, refreshing, refreshed};
	enum {track_normal, track_drag, track_springback};

	trefresh(display& disp)
		: disp_(disp)
		, widget_(NULL)
		, anim_type_(anim2::NONE)
		, mascot_anim_id_(INVALID_ANIM_ID)
		, mascot_anim_rect_(null_rect)
		, egg_gap_(2 * twidget::hdpi_scale)
		, min_egg_diameter_(6 * twidget::hdpi_scale)
		, shape_egg_diameter_(24 * twidget::hdpi_scale)
		, max_egg_diameter_(66 * twidget::hdpi_scale)
		, image_("misc/egg.png")
		, track_status_(track_normal)
		, refresh_(norefresh)
		, yoffset_at_start_(0)
		, springback_yoffset_(0)
		, springback_granularity_(twidget::npos)
		, refreshed_linger_ticks_(0)
	{}

	~trefresh() 
	{
		erase_mascot_anim();
	}

	void set_widget(ttrack* widget)
	{
		widget_ = widget;
	}

	int draw(const int yoffset);
	void set_refreshed() { refresh_ = refreshed; }

	int refresh_status() const { return refresh_; }
	// int track_status() const { return track_status_; }

	bool did_control_drag_detect(tcontrol* control, bool start, const twidget::tdrag_direction type);

	void set_refreshing_anim(int anim) { anim_type_ = anim; }
	void set_refreshed_text(const std::string& text) { refreshed_text_ = text; }

	void set_did_refreshing(const boost::function<void ()>& fn) { did_refreshing_ = fn; }
	void set_did_can_drag(const boost::function<bool ()>& fn) { did_can_drag_ = fn; }

private:
	void start_mascot_anim(int x, int y);
	void erase_mascot_anim();

private:
	display& disp_;
	ttrack* widget_;
	int anim_type_;
	std::string image_;
	int mascot_anim_id_;
	SDL_Rect mascot_anim_rect_;
	std::string refreshed_text_;

	int egg_gap_;
	int min_egg_diameter_;
	int shape_egg_diameter_;
	int max_egg_diameter_; // must equal mascot_anim_rect_.h

	boost::function<void ()> did_refreshing_;
	boost::function<bool ()> did_can_drag_;

	int track_status_;
	int springback_yoffset_;
	int yoffset_at_start_;
	int refresh_;
	int springback_granularity_;

	uint32_t refreshed_linger_ticks_;
};

class tfold
{
public:
	tfold(display& disp)
		: disp_(disp)
		, widget_(NULL)
		, draging_(false)
		, diff_(0, 0)
		, std_top_rect_(null_rect)
		, max_top_movable_(twidget::npos)
		, std_bottom_rect_(null_rect)
		, upward_(true)
		, top_grid_(NULL)
		, bottom_grid_(NULL)
	{}

	void set_widget(tstacked_widget* widget);

	void set_did_can_drag(const boost::function<bool (int x, int y)>& fn) { did_can_drag_ = fn; }
	void set_did_calculate_reserve(const boost::function<void (int& y, int& height)>& fn) { did_calculate_reserve_ = fn; }

	bool did_control_drag_detect(tcontrol* control, bool start, const twidget::tdrag_direction type);
	bool did_drag_coordinate(tcontrol* control, const tpoint& first, const tpoint& last);

	bool upward() const { return upward_; }
	bool draging() const { return draging_; }

private:
	void move(const int diff_y) const;

private:
	display& disp_;
	tstacked_widget* widget_;
	bool draging_;
	tpoint diff_;
	SDL_Rect std_top_rect_;
	SDL_Rect std_bottom_rect_;
	bool upward_;
	int max_top_movable_;
	int min_reserve_height_;
	tstacked_widget::tgrid3* top_grid_;
	tstacked_widget::tgrid3* bottom_grid_;

	boost::function<bool (int x, int y)> did_can_drag_;
	boost::function<void (int& y, int& height)> did_calculate_reserve_;
};

} // namespace effect
} // namespace gui2

#endif
