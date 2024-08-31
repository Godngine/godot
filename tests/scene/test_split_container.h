/**************************************************************************/
/*  test_split_container.h                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#ifndef TEST_SPLIT_CONTAINER_H
#define TEST_SPLIT_CONTAINER_H

#include "scene/gui/split_container.h"
#include "scene/main/window.h"

#include "tests/test_macros.h"

namespace TestSplitContainer {

static inline void check_positions(SplitContainer *sc, Vector<int> positions, int sep, bool horizontal = true) {
	// Checks the rects of each split container child.
	CHECK(sc->get_child_count(false) == positions.size() + 1);
	int last_pos = 0;
	for (int i = 0; i < sc->get_child_count(false); i++) {
		// Assuming there is no invalid children.
		Control *c = Object::cast_to<Control>(sc->get_child(i, false));
		int pos = i >= positions.size() ? sc->get_size()[horizontal ? 0 : 1] : positions[i];
		Rect2 rect;
		if (horizontal) {
			rect.size.y = sc->get_size().y;
			rect.position.x = last_pos;
			rect.size.x = pos - last_pos;
		} else {
			rect.size.x = sc->get_size().x;
			rect.position.y = last_pos;
			rect.size.y = pos - last_pos;
		}
		CHECK_MESSAGE(c->get_rect() == rect, vformat("Child %s is the wrong size.", i));
		last_pos = pos + sep;
	}
}

static inline void check_position(SplitContainer *sc, int position, int sep, bool horizontal = true) {
	check_positions(sc, { position }, sep, horizontal);
}

static inline void check_positions_rtl(SplitContainer *sc, Vector<int> positions, int sep) {
	// Checks the rects of each split container child. Right to left layout.
	CHECK(sc->get_child_count(false) == positions.size() + 1);
	int last_pos = sc->get_size().x;
	for (int i = 0; i < sc->get_child_count(false); i++) {
		// Assuming there is no invalid children.
		Control *c = Object::cast_to<Control>(sc->get_child(i, false));
		int pos = i >= positions.size() ? 0 : sc->get_size().x - positions[i];
		Rect2 rect;
		rect.size.y = sc->get_size().y;
		rect.position.x = pos;
		rect.size.x = last_pos - pos;
		CHECK_MESSAGE(c->get_rect() == rect, vformat("Child %s is the wrong size.", i));
		last_pos = pos - sep;
	}
}

static inline void check_position_rtl(SplitContainer *sc, int position, int sep) {
	check_positions_rtl(sc, { position }, sep);
}

TEST_CASE("[SceneTree][SplitContainer] add children") {
	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_singleton()->get_root()->add_child(split_container);

	SUBCASE("[SplitContainer] one child") {
		Control *child_a = memnew(Control);
		split_container->add_child(child_a);
		MessageQueue::get_singleton()->flush();

		// One child will fill the entire area.
		CHECK(child_a->get_rect() == split_container->get_rect());

		split_container->set_vertical(true);
		CHECK(child_a->get_rect() == split_container->get_rect());

		memdelete(child_a);
	}

	SUBCASE("[SplitContainer] preserve split offset") {
		// The split offset is preserved through adding, removing, and changing visibility of children.
		split_container->set_split_offset(100);
		CHECK(split_container->get_split_offset() == 100);

		Control *child_a = memnew(Control);
		split_container->add_child(child_a);
		MessageQueue::get_singleton()->flush();
		CHECK(split_container->get_split_offset() == 100);

		Control *child_b = memnew(Control);
		split_container->add_child(child_b);
		MessageQueue::get_singleton()->flush();
		CHECK(split_container->get_split_offset() == 100);

		child_a->hide();
		MessageQueue::get_singleton()->flush();
		CHECK(split_container->get_split_offset() == 100);

		child_b->hide();
		MessageQueue::get_singleton()->flush();
		CHECK(split_container->get_split_offset() == 100);

		child_b->show();
		MessageQueue::get_singleton()->flush();
		CHECK(split_container->get_split_offset() == 100);

		child_a->show();
		MessageQueue::get_singleton()->flush();
		CHECK(split_container->get_split_offset() == 100);

		split_container->remove_child(child_a);
		MessageQueue::get_singleton()->flush();
		CHECK(split_container->get_split_offset() == 100);

		split_container->add_child(child_a);
		MessageQueue::get_singleton()->flush();
		CHECK(split_container->get_split_offset() == 100);

		memdelete(child_a);
		memdelete(child_b);
	}

	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] dragger visibility") {
	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_singleton()->get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	Control *child_b = memnew(Control);
	split_container->add_child(child_a);
	split_container->add_child(child_b);
	SplitContainerDragger *dragger = Object::cast_to<SplitContainerDragger>(split_container->get_child(2, true));

	split_container->add_theme_constant_override("autohide", 0);
	MessageQueue::get_singleton()->flush();

	const int sep_constant = split_container->get_theme_constant("separation");
	const Size2i separator_size = Size2i(MAX(sep_constant, split_container->get_theme_icon("h_grabber")->get_width()), MAX(sep_constant, split_container->get_theme_icon("v_grabber")->get_height()));

	SUBCASE("[SplitContainer] visibility based on child count") {
		split_container->remove_child(child_a);
		split_container->remove_child(child_b);
		MessageQueue::get_singleton()->flush();

		// No children, not visible.
		CHECK_FALSE(dragger->is_visible());

		// Add one child, not visible.
		split_container->add_child(child_a);
		MessageQueue::get_singleton()->flush();
		CHECK_FALSE(dragger->is_visible());

		// Two children, visible.
		split_container->add_child(child_b);
		MessageQueue::get_singleton()->flush();
		CHECK(dragger->is_visible());

		// Remove a child, not visible.
		split_container->remove_child(child_b);
		MessageQueue::get_singleton()->flush();
		CHECK_FALSE(dragger->is_visible());
	}

	SUBCASE("[SplitContainer] set dragger visibility") {
		split_container->set_dragger_visibility(SplitContainer::DRAGGER_HIDDEN);
		MessageQueue::get_singleton()->flush();
		check_position(split_container, 0, separator_size.x);
		// Can't check the visibility since it happens in draw.

		split_container->set_dragger_visibility(SplitContainer::DRAGGER_HIDDEN_COLLAPSED);
		MessageQueue::get_singleton()->flush();
		check_position(split_container, 0, 0);

		split_container->set_dragger_visibility(SplitContainer::DRAGGER_VISIBLE);
		MessageQueue::get_singleton()->flush();
		check_position(split_container, 0, separator_size.x);
	}

	SUBCASE("[SplitContainer] not visible when collapsed") {
		split_container->set_collapsed(true);
		MessageQueue::get_singleton()->flush();
		CHECK_FALSE(dragger->is_visible());

		split_container->set_collapsed(false);
		MessageQueue::get_singleton()->flush();
		CHECK(dragger->is_visible());
	}

	memdelete(child_a);
	memdelete(child_b);
	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] collapsed") {
	DisplayServerMock *DS = (DisplayServerMock *)(DisplayServer::get_singleton());

	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_singleton()->get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	split_container->add_child(child_a);
	Control *child_b = memnew(Control);
	split_container->add_child(child_b);
	MessageQueue::get_singleton()->flush();

	const int sep_constant = split_container->get_theme_constant("separation");
	const Size2i separator_size = Size2i(MAX(sep_constant, split_container->get_theme_icon("h_grabber")->get_width()), MAX(sep_constant, split_container->get_theme_icon("v_grabber")->get_height()));

	SUBCASE("[SplitContainer] dragging and cursor") {
		split_container->set_collapsed(true);

		// Cursor is default.
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(1, 1), MouseButtonMask::NONE, Key::NONE);
		CHECK(DS->get_cursor_shape() == DisplayServer::CURSOR_ARROW);

		// Dragger is disabled, cannot drag.
		SEND_GUI_MOUSE_BUTTON_EVENT(Point2(1, 1), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
		MessageQueue::get_singleton()->flush();
		check_position(split_container, 0, separator_size.x);
		CHECK(split_container->get_split_offset() == 0);
		SEND_GUI_MOUSE_MOTION_EVENT(Point2(10, 1), MouseButtonMask::LEFT, Key::NONE);
		MessageQueue::get_singleton()->flush();
		check_position(split_container, 0, separator_size.x);
		CHECK(split_container->get_split_offset() == 0);
	}

	SUBCASE("[SplitContainer] no expand flags") {
		int def_pos = 0;

		split_container->set_split_offset(10);
		MessageQueue::get_singleton()->flush();
		check_position(split_container, def_pos + 10, separator_size.x);

		split_container->set_collapsed(true);
		MessageQueue::get_singleton()->flush();

		// The split offset is treated as 0 when collapsed.
		check_position(split_container, def_pos, separator_size.x);
		CHECK(split_container->get_split_offset() == 10);
	}

	SUBCASE("[SplitContainer] first child expanded") {
		int def_pos = split_container->get_size().x - separator_size.x;
		child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		split_container->set_split_offset(-10);
		MessageQueue::get_singleton()->flush();

		check_position(split_container, def_pos - 10, separator_size.x);

		split_container->set_collapsed(true);
		MessageQueue::get_singleton()->flush();

		// The split offset is treated as 0 when collapsed.
		check_position(split_container, def_pos, separator_size.x);
		CHECK(split_container->get_split_offset() == -10);
	}

	SUBCASE("[SplitContainer] second child expanded") {
		int def_pos = 0;
		child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		split_container->set_split_offset(10);
		MessageQueue::get_singleton()->flush();

		check_position(split_container, def_pos + 10, separator_size.x);

		split_container->set_collapsed(true);
		MessageQueue::get_singleton()->flush();

		// The split offset is treated as 0 when collapsed.
		check_position(split_container, def_pos, separator_size.x);
		CHECK(split_container->get_split_offset() == 10);
	}

	SUBCASE("[SplitContainer] both children expanded") {
		int def_pos = (split_container->get_size().y - separator_size.y) / 2;
		child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		split_container->set_split_offset(10);
		MessageQueue::get_singleton()->flush();

		check_position(split_container, def_pos + 10, separator_size.x);

		split_container->set_collapsed(true);
		MessageQueue::get_singleton()->flush();

		// The split offset is treated as 0 when collapsed.
		check_position(split_container, def_pos, separator_size.x);
		CHECK(split_container->get_split_offset() == 10);
	}

	memdelete(child_a);
	memdelete(child_b);
	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] cursor shape") {
	DisplayServerMock *DS = (DisplayServerMock *)(DisplayServer::get_singleton());

	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_singleton()->get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	split_container->add_child(child_a);
	Control *child_b = memnew(Control);
	split_container->add_child(child_b);
	MessageQueue::get_singleton()->flush();

	Point2 on_dragger = Point2(1, 1);
	Point2 not_on_dragger = Point2(50, 50);

	// Default cursor shape.
	SEND_GUI_MOUSE_MOTION_EVENT(not_on_dragger, MouseButtonMask::NONE, Key::NONE);
	CHECK(DS->get_cursor_shape() == DisplayServer::CURSOR_ARROW);

	// Horizontal cursor shape.
	SEND_GUI_MOUSE_MOTION_EVENT(on_dragger, MouseButtonMask::NONE, Key::NONE);
	CHECK(DS->get_cursor_shape() == DisplayServer::CURSOR_HSPLIT);

	// Vertical cursor shape.
	split_container->set_vertical(true);
	SEND_GUI_MOUSE_MOTION_EVENT(on_dragger, MouseButtonMask::NONE, Key::NONE);
	CHECK(DS->get_cursor_shape() == DisplayServer::CURSOR_VSPLIT);

	// Move off, default cursor shape.
	SEND_GUI_MOUSE_MOTION_EVENT(not_on_dragger, MouseButtonMask::NONE, Key::NONE);
	CHECK(DS->get_cursor_shape() == DisplayServer::CURSOR_ARROW);

	memdelete(child_a);
	memdelete(child_b);
	memdelete(split_container);
}

TEST_CASE("[SceneTree][SplitContainer] two children") {
	SplitContainer *split_container = memnew(SplitContainer);
	split_container->set_size(Size2(500, 500));
	SceneTree::get_singleton()->get_root()->add_child(split_container);
	Control *child_a = memnew(Control);
	Control *child_b = memnew(Control);
	split_container->add_child(child_a);
	split_container->add_child(child_b);
	MessageQueue::get_singleton()->flush();

	const int sep_constant = split_container->get_theme_constant("separation");
	const Size2i separator_size = Size2i(MAX(sep_constant, split_container->get_theme_icon("h_grabber")->get_width()), MAX(sep_constant, split_container->get_theme_icon("v_grabber")->get_height()));

	SUBCASE("[SplitContainer] minimum size") {
		// Minimum size is the sum of both children's minimum sizes and the separator depending on the vertical axis.
		child_a->set_custom_minimum_size(Size2(100, 200));
		child_b->set_custom_minimum_size(Size2(100, 200));
		MessageQueue::get_singleton()->flush();

		Size2 min_size = split_container->get_minimum_size();
		CHECK(min_size.x == 200 + separator_size.x);
		CHECK(min_size.y == 200);

		split_container->set_vertical(true);
		MessageQueue::get_singleton()->flush();
		min_size = split_container->get_minimum_size();
		CHECK(min_size.x == 100);
		CHECK(min_size.y == 400 + separator_size.y);
	}

	SUBCASE("[SplitContainer] default position") {
		SUBCASE("[SplitContainer] vertical") {
			// Make sure clamping the split offset doesn't change it or the position.
			split_container->set_vertical(true);

			// No expand flags set.
			MessageQueue::get_singleton()->flush();
			int def_pos = 0;
			check_position(split_container, def_pos, separator_size.y, false);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.y, false);

			// First expand flags set.
			child_a->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_v_size_flags(Control::SIZE_FILL);
			MessageQueue::get_singleton()->flush();
			def_pos = split_container->get_size().y - separator_size.y;
			check_position(split_container, def_pos, separator_size.y, false);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.y, false);

			// Second expand flags set.
			child_a->set_v_size_flags(Control::SIZE_FILL);
			child_b->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();
			def_pos = 0;
			check_position(split_container, 0, separator_size.y, false);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.y, false);

			// Both expand flags set.
			child_a->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_v_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();
			def_pos = (split_container->get_size().y - separator_size.y) / 2;
			check_position(split_container, def_pos, separator_size.y, false);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.y, false);

			// Unequal stretch ratios.
			child_a->set_stretch_ratio(2.0);
			MessageQueue::get_singleton()->flush();
			def_pos = (split_container->get_size().y * 2 / 3) - separator_size.y / 2;
			check_position(split_container, def_pos, separator_size.y, false);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.y, false);
		}

		SUBCASE("[SplitContainer] right to left") {
			split_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
			split_container->set_position(Point2(0, 0));

			// No expand flags set.
			MessageQueue::get_singleton()->flush();
			int def_pos = 0;
			check_position_rtl(split_container, def_pos, separator_size.y);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position_rtl(split_container, def_pos, separator_size.y);

			// First expand flags set.
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_FILL);
			MessageQueue::get_singleton()->flush();
			def_pos = split_container->get_size().y - separator_size.y;
			check_position_rtl(split_container, def_pos, separator_size.y);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position_rtl(split_container, def_pos, separator_size.y);

			// Second expand flags set.
			child_a->set_h_size_flags(Control::SIZE_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();
			def_pos = 0;
			check_position_rtl(split_container, 0, separator_size.y);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position_rtl(split_container, def_pos, separator_size.y);

			// Both expand flags set.
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();
			def_pos = (split_container->get_size().y - separator_size.y) / 2;
			check_position_rtl(split_container, def_pos, separator_size.y);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position_rtl(split_container, def_pos, separator_size.y);

			// Unequal stretch ratios.
			child_a->set_stretch_ratio(2.0);
			MessageQueue::get_singleton()->flush();
			def_pos = (split_container->get_size().y * 2 / 3) - separator_size.y / 2;
			check_position_rtl(split_container, def_pos, separator_size.y);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position_rtl(split_container, def_pos, separator_size.y);
		}

		SUBCASE("[SplitContainer] no expand flags") {
			int def_pos = 0;

			check_position(split_container, def_pos, separator_size.x);

			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.x);

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			def_pos = 400;
			check_position(split_container, def_pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == def_pos);
			check_position(split_container, def_pos, separator_size.x);

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			def_pos = split_container->get_size().x - 400 - separator_size.x;
			check_position(split_container, def_pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == def_pos);
			check_position(split_container, def_pos, separator_size.x);

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			MessageQueue::get_singleton()->flush();
			def_pos = 200;
			check_position(split_container, def_pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == def_pos);
			check_position(split_container, def_pos, separator_size.x);
		}

		SUBCASE("[SplitContainer] first child expanded") {
			const int def_pos = split_container->get_size().x - separator_size.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			check_position(split_container, def_pos, separator_size.x);

			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.x);

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.x);

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			int pos = split_container->get_size().x - 400 - separator_size.x;
			check_position(split_container, pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			check_position(split_container, pos, separator_size.x);

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			MessageQueue::get_singleton()->flush();
			pos = 200;
			check_position(split_container, pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			check_position(split_container, pos, separator_size.x);
		}

		SUBCASE("[SplitContainer] second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			check_position(split_container, def_pos, separator_size.x);

			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.x);

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			def_pos = 400;
			check_position(split_container, def_pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == def_pos);
			check_position(split_container, def_pos, separator_size.x);

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			def_pos = 500 - 400 - separator_size.x;
			check_position(split_container, def_pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == def_pos);
			check_position(split_container, def_pos, separator_size.x);

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			MessageQueue::get_singleton()->flush();
			def_pos = 200;
			check_position(split_container, def_pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == def_pos);
			check_position(split_container, def_pos, separator_size.x);
		}

		SUBCASE("[SplitContainer] both children expanded") {
			const int def_pos = (split_container->get_size().x - separator_size.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			check_position(split_container, def_pos, separator_size.x);

			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.x);

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			int pos = 400;
			check_position(split_container, pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			check_position(split_container, pos, separator_size.x);

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			pos = split_container->get_size().x - 400 - separator_size.x;
			check_position(split_container, pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			check_position(split_container, pos, separator_size.x);

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			MessageQueue::get_singleton()->flush();
			pos = 200;
			check_position(split_container, pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			check_position(split_container, pos, separator_size.x);
		}

		SUBCASE("[SplitContainer] unequal stretch ratios") {
			const int def_pos = (split_container->get_size().x * 2 / 3) - separator_size.x / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_a->set_stretch_ratio(2.0);
			MessageQueue::get_singleton()->flush();

			check_position(split_container, def_pos, separator_size.x);

			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 0);
			check_position(split_container, def_pos, separator_size.x);

			// Minimum sizes affect default position.

			// First child with minimum size.
			child_a->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			int pos = 400;
			check_position(split_container, pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			check_position(split_container, pos, separator_size.x);

			// Second child with minimum size.
			child_a->set_custom_minimum_size(Size2(0, 0));
			child_b->set_custom_minimum_size(Size2(400, 0));
			MessageQueue::get_singleton()->flush();
			pos = split_container->get_size().x - 400 - separator_size.x;
			check_position(split_container, pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			check_position(split_container, pos, separator_size.x);

			// Both children with minimum size.
			child_a->set_custom_minimum_size(Size2(200, 0));
			child_b->set_custom_minimum_size(Size2(288, 0));
			MessageQueue::get_singleton()->flush();
			pos = 200;
			check_position(split_container, pos, separator_size.x);
			split_container->clamp_split_offset();
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == pos - def_pos);
			check_position(split_container, pos, separator_size.x);
		}
	}

	SUBCASE("[SplitContainer] set split offset") {
		SUBCASE("[SplitContainer] right to left") {
			split_container->set_layout_direction(Control::LAYOUT_DIRECTION_RTL);
			split_container->set_position(Point2(0, 0));
			int def_pos = 0;
			// Positive.
			split_container->set_split_offset(10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 10);
			check_position_rtl(split_container, def_pos + 10, separator_size.x);

			// Negative.
			split_container->set_split_offset(-10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -10);
			check_position_rtl(split_container, def_pos, separator_size.x);
		}

		SUBCASE("[SplitContainer] no expand flags") {
			int def_pos = 0;

			// Positive.
			split_container->set_split_offset(10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 10);
			check_position(split_container, def_pos + 10, separator_size.x);

			// Negative.
			split_container->set_split_offset(-10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -10);
			check_position(split_container, def_pos, separator_size.x);

			// Clamped.
			split_container->set_split_offset(1000);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 1000);
			check_position(split_container, split_container->get_size().x - separator_size.x, separator_size.x);
		}

		SUBCASE("[SplitContainer] first child expanded") {
			int def_pos = split_container->get_size().x - separator_size.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			// Positive.
			split_container->set_split_offset(10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 10);
			check_position(split_container, def_pos, separator_size.x);

			// Negative.
			split_container->set_split_offset(-10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -10);
			check_position(split_container, def_pos - 10, separator_size.x);

			// Clamped.
			split_container->set_split_offset(-1000);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -1000);
			check_position(split_container, 0, separator_size.x);
		}

		SUBCASE("[SplitContainer] second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			// Positive.
			split_container->set_split_offset(10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 10);
			check_position(split_container, def_pos + 10, separator_size.x);

			// Negative.
			split_container->set_split_offset(-10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -10);
			check_position(split_container, def_pos, separator_size.x);

			// Clamped.
			split_container->set_split_offset(1000);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 1000);
			check_position(split_container, split_container->get_size().x - separator_size.x, separator_size.x);
		}

		SUBCASE("[SplitContainer] both children expanded") {
			int def_pos = (split_container->get_size().x - separator_size.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			// Positive.
			split_container->set_split_offset(10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 10);
			check_position(split_container, def_pos + 10, separator_size.x);

			// Negative.
			split_container->set_split_offset(-10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -10);
			check_position(split_container, def_pos - 10, separator_size.x);

			// Clamped positive.
			split_container->set_split_offset(1000);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 1000);
			check_position(split_container, split_container->get_size().x - separator_size.x, separator_size.x);

			// Clamped negative.
			split_container->set_split_offset(-1000);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -1000);
			check_position(split_container, 0, separator_size.x);
		}

		SUBCASE("[SplitContainer] unequal stretch ratios") {
			int def_pos = (split_container->get_size().x * 2 / 3) - separator_size.x / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_a->set_stretch_ratio(2.0);
			MessageQueue::get_singleton()->flush();

			// Positive.
			split_container->set_split_offset(10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 10);
			check_position(split_container, def_pos + 10, separator_size.x);

			// Negative.
			split_container->set_split_offset(-10);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -10);
			check_position(split_container, def_pos - 10, separator_size.x);

			// Clamped positive.
			split_container->set_split_offset(1000);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == 1000);
			check_position(split_container, split_container->get_size().x - separator_size.x, separator_size.x);

			// Clamped negative.
			split_container->set_split_offset(-1000);
			MessageQueue::get_singleton()->flush();
			CHECK(split_container->get_split_offset() == -1000);
			check_position(split_container, 0, separator_size.x);
		}
	}

	SUBCASE("[SplitContainer] resize split container") {
		SUBCASE("[SplitContainer] no expand flags") {
			int def_pos = 0;
			// Increase the size.
			split_container->set_size(Size2(600, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Change size with a split offset.
			split_container->set_split_offset(100);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);

			split_container->set_size(Size2(500, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);

			// Change size so that the first child changes size.
			split_container->set_size(Size2(80, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, 80 - separator_size.x, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);
		}

		SUBCASE("[SplitContainer] first child expanded") {
			int def_pos = split_container->get_size().x - separator_size.x;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			// Increase the size.
			split_container->set_size(Size2(600, 500));
			def_pos = split_container->get_size().x - separator_size.x;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			def_pos = split_container->get_size().x - separator_size.x;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Change size with a split offset.
			split_container->set_split_offset(-100);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos - 100, separator_size.x);

			split_container->set_size(Size2(500, 500));
			def_pos = split_container->get_size().x - separator_size.x;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos - 100, separator_size.x);
			CHECK(split_container->get_split_offset() == -100);

			// Change size so that the second child changes size.
			split_container->set_size(Size2(80, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, 0, separator_size.x);
			CHECK(split_container->get_split_offset() == -100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos - 100, separator_size.x);
			CHECK(split_container->get_split_offset() == -100);
		}

		SUBCASE("[SplitContainer] second child expanded") {
			int def_pos = 0;
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			// Increase the size.
			split_container->set_size(Size2(600, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Change size with a split offset.
			split_container->set_split_offset(100);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);

			split_container->set_size(Size2(500, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);

			// Change size so that the first child changes size.
			split_container->set_size(Size2(80, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, 80 - separator_size.x, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);
		}

		SUBCASE("[SplitContainer] both children expanded") {
			int def_pos = (split_container->get_size().x - separator_size.x) / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();

			// Increase the size.
			split_container->set_size(Size2(600, 500));
			def_pos = (split_container->get_size().x - separator_size.x) / 2;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			def_pos = (split_container->get_size().x - separator_size.x) / 2;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Change size with a split offset.
			split_container->set_split_offset(100);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);

			split_container->set_size(Size2(500, 500));
			def_pos = (split_container->get_size().x - separator_size.x) / 2;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);

			// Change size so that the second child is minimized.
			split_container->set_size(Size2(80, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, 80 - separator_size.x, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			def_pos = (split_container->get_size().x - separator_size.x) / 2;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);
		}

		SUBCASE("[SplitContainer] unequal stretch ratios") {
			int def_pos = (split_container->get_size().x * 2 / 3) - separator_size.x / 2;
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_a->set_stretch_ratio(2.0);
			MessageQueue::get_singleton()->flush();

			// Increase the size.
			split_container->set_size(Size2(600, 500));
			def_pos = (split_container->get_size().x * 2 / 3) - separator_size.x / 2;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Decrease the size.
			split_container->set_size(Size2(400, 500));
			def_pos = (split_container->get_size().x * 2 / 3) - separator_size.x / 2;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos, separator_size.x);

			// Change size with a split offset.
			split_container->set_split_offset(100);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);

			split_container->set_size(Size2(500, 500));
			def_pos = (split_container->get_size().x * 2 / 3) - separator_size.x / 2;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);

			// Change size so that the second child is minimized.
			split_container->set_size(Size2(80, 500));
			MessageQueue::get_singleton()->flush();
			check_position(split_container, 80 - separator_size.x, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);

			// Increase size again.
			split_container->set_size(Size2(500, 500));
			def_pos = (split_container->get_size().x * 2 / 3) - separator_size.x / 2;
			MessageQueue::get_singleton()->flush();
			check_position(split_container, def_pos + 100, separator_size.x);
			CHECK(split_container->get_split_offset() == 100);
		}
	}

	SUBCASE("[SplitContainer] drag") {
		SUBCASE("[SplitContainer] vertical, no expand flags") {
			SIGNAL_WATCH(split_container, "dragged");
			Array signal_args;
			signal_args.push_back(Array());
			((Array)signal_args[0]).push_back(0);

			split_container->set_vertical(true);
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = 0;
			int split_dragger_ofs = 0;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.y, false);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			SIGNAL_CHECK_FALSE("dragged");

			// Move the dragger.
			dragger_pos = 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.y, false);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			((Array)signal_args[0])[0] = split_container->get_split_offset();
			SIGNAL_CHECK("dragged", signal_args);

			// Move down.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.y, false);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			((Array)signal_args[0])[0] = split_container->get_split_offset();
			SIGNAL_CHECK("dragged", signal_args);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().y - separator_size.y;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, 1000), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.y, false);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			((Array)signal_args[0])[0] = split_container->get_split_offset();
			SIGNAL_CHECK("dragged", signal_args);

			// Move up.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.y, false);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			((Array)signal_args[0])[0] = split_container->get_split_offset();
			SIGNAL_CHECK("dragged", signal_args);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(0, dragger_pos), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.y, false);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			SIGNAL_CHECK_FALSE("dragged");

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(0, 200), MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.y, false);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			SIGNAL_CHECK_FALSE("dragged");

			SIGNAL_UNWATCH(split_container, "dragged");
		}

		SUBCASE("[SplitContainer] no expand flags") {
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = 0;
			int split_dragger_ofs = 0;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move the dragger.
			dragger_pos = 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Continue moving.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().x - separator_size.x;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(1000, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move back in.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(200, 0), MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
		}

		SUBCASE("[SplitContainer] first child expanded") {
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = split_container->get_size().x - separator_size.x;
			int split_dragger_ofs = -dragger_pos;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move the dragger.
			dragger_pos -= 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Continue moving.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().x - separator_size.x;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(1000, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move back in.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(200, 0), MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
		}

		SUBCASE("[SplitContainer] second child expanded") {
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = 0;
			int split_dragger_ofs = 0;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move the dragger.
			dragger_pos = 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Continue moving.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().x - separator_size.x;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(1000, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move back in.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(200, 0), MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
		}

		SUBCASE("[SplitContainer] both children expanded") {
			child_a->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			child_b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
			MessageQueue::get_singleton()->flush();
			Point2 mouse_offset = Point2(1, 1);
			int dragger_pos = (split_container->get_size().x - separator_size.x) / 2;
			int split_dragger_ofs = -dragger_pos;

			// Grab the dragger.
			SEND_GUI_MOUSE_BUTTON_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move the dragger.
			dragger_pos += 10;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
			// It is clamped.
			split_container->clamp_split_offset();
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Continue moving.
			dragger_pos = 400;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Moves even when mouse is outside.
			dragger_pos = split_container->get_size().x - separator_size.x;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(1000, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Move back in.
			dragger_pos = 100;
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButtonMask::LEFT, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// Release.
			SEND_GUI_MOUSE_BUTTON_RELEASED_EVENT(mouse_offset + Point2(dragger_pos, 0), MouseButton::LEFT, MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);

			// No longer moves with the mouse.
			SEND_GUI_MOUSE_MOTION_EVENT(mouse_offset + Point2(200, 0), MouseButtonMask::NONE, Key::NONE);
			MessageQueue::get_singleton()->flush();
			check_position(split_container, dragger_pos, separator_size.x);
			CHECK(split_container->get_split_offset() == dragger_pos + split_dragger_ofs);
		}
	}

	memdelete(child_b);
	memdelete(child_a);
	memdelete(split_container);
}

} // namespace TestSplitContainer

#endif // TEST_SPLIT_CONTAINER_H
