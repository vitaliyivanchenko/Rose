/* $Id: helper.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_AUXILIARY_WINDOW_BUILDER_HELPER_HPP_INCLUDED
#define GUI_AUXILIARY_WINDOW_BUILDER_HELPER_HPP_INCLUDED

#include "gui/widgets/scrollbar_container.hpp"

#include <string>
#include <vector>

class config;

namespace gui2 {

namespace implementation {

/**
 * Returns the vertical alignment.
 *
 * @param v_align                 The string representing the alignment.
 *
 * @returns                       The alignment.
 */
unsigned get_v_align(const std::string& v_align);

/**
 * Returns the horizontal alignment.
 *
 * @param h_align                 The string representing the alignment.
 *
 * @returns                       The alignment.
 */
unsigned get_h_align(const std::string& h_align);

twidget::torientation get_orientation(const std::string& orientation);

twidget::tdrag_direction get_drag_direction(const std::string& pan);

unsigned get_drag(const std::string& drag);
std::string form_drag_str(unsigned flags);

/**
 * Returns the border flags.
 *
 * @param border                  The string representing the border flags.
 *
 * @returns                       The border flags.
 */
unsigned get_border(const std::vector<std::string>& border);

/**
 * Returns the placement/resize flags.
 *
 * @param cfg                     The config to look for flags for.
 *
 * @returns                       The placement/resize flags.
 */
unsigned read_flags(const config& cfg);

/**
 * Returns the scrollbar mode flags.
 *
 * @param scrollbar_mode          The string representing the scrollbar_mode.
 *
 * @returns                       The scrollbar mode flags.
 */
tscrollbar_container::tscrollbar_mode
		get_scrollbar_mode(const std::string& scrollbar_mode);

/**
 * Returns the return value for a widget.
 *
 * If there's a valid retval_id that will be returned.
 * Else if there's a retval that's returned.
 * Else it falls back to the id.
 */
int get_retval(const std::string& retval_id
		, const int retval
		, const std::string& id);

} // namespace implementation

} // namespace gui2

#endif
