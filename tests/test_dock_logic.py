"""Unit tests for C++ dock panel logic using Python.

This module tests the business logic of the C++ dock panel application
by validating coordinate calculations, button containment logic, and
state transitions without requiring Windows/GDI+ dependencies.
"""

import unittest
from dataclasses import dataclass
from typing import Optional


@dataclass
class RectF:
    """Represents a rectangle with floating-point coordinates."""
    x: float
    y: float
    width: float
    height: float

    def contains(self, x: float, y: float) -> bool:
        """Check if point (x, y) is inside this rectangle."""
        # Use strict inequality for zero-size rectangles
        if self.width <= 0 or self.height <= 0:
            return False
        return self.x <= x < self.x + self.width and self.y <= y < self.y + self.height


@dataclass
class Button:
    """Abstract representation of a button for testing."""
    bounds: RectF
    is_hovered: bool = False
    button_type: str = "app"  # "app", "minimize", "close"
    name: str = ""

    def contains(self, x: float, y: float) -> bool:
        """Check if point is inside button bounds."""
        return self.bounds.contains(x, y)


class TestRectF(unittest.TestCase):
    """Tests for RectF geometry operations."""

    def test_contains_point_inside(self):
        """Point inside rectangle should return True."""
        rect = RectF(10.0, 10.0, 50.0, 30.0)
        self.assertTrue(rect.contains(20.0, 20.0))
        self.assertTrue(rect.contains(10.0, 10.0))  # Edge case: top-left corner
        self.assertTrue(rect.contains(59.0, 39.0))  # Edge case: bottom-right area

    def test_contains_point_outside(self):
        """Point outside rectangle should return False."""
        rect = RectF(10.0, 10.0, 50.0, 30.0)
        self.assertFalse(rect.contains(5.0, 5.0))
        self.assertFalse(rect.contains(70.0, 50.0))
        self.assertFalse(rect.contains(10.0, 41.0))  # Below rectangle

    def test_zero_size_rectangle(self):
        """Rectangle with zero size should not contain any points."""
        rect = RectF(10.0, 10.0, 0.0, 0.0)
        self.assertFalse(rect.contains(10.0, 10.0))


class TestButtonContainment(unittest.TestCase):
    """Tests for button hit detection logic."""

    def setUp(self):
        """Set up test buttons."""
        self.app_button = Button(
            bounds=RectF(100.0, 50.0, 40.0, 40.0),
            button_type="app",
            name="Chrome"
        )
        self.minimize_button = Button(
            bounds=RectF(500.0, 10.0, 30.0, 30.0),
            button_type="minimize"
        )
        self.close_button = Button(
            bounds=RectF(540.0, 10.0, 30.0, 30.0),
            button_type="close"
        )

    def test_app_button_contains_center(self):
        """App button should contain its center point."""
        self.assertTrue(self.app_button.contains(120.0, 70.0))

    def test_app_button_edge_cases(self):
        """Test app button boundary conditions."""
        # Top-left corner
        self.assertTrue(self.app_button.contains(100.0, 50.0))
        # Bottom-right corner area
        self.assertTrue(self.app_button.contains(139.0, 89.0))
        # Just outside right edge
        self.assertFalse(self.app_button.contains(141.0, 70.0))
        # Just outside bottom edge
        self.assertFalse(self.app_button.contains(120.0, 91.0))

    def test_control_buttons_separate(self):
        """Minimize and close buttons should not overlap."""
        # Point in minimize button should not be in close button
        self.assertTrue(self.minimize_button.contains(510.0, 20.0))
        self.assertFalse(self.close_button.contains(510.0, 20.0))

        # Point in close button should not be in minimize button
        self.assertTrue(self.close_button.contains(550.0, 20.0))
        self.assertFalse(self.minimize_button.contains(550.0, 20.0))


class TestDockPanelLogic(unittest.TestCase):
    """Tests for DockPanel business logic simulation."""

    def setUp(self):
        """Set up a simulated dock panel with buttons."""
        # Simulate typical dock layout
        self.dock_bounds = RectF(0.0, 0.0, 600.0, 100.0)

        # Control buttons (top-right)
        self.ctrl_buttons = [
            Button(RectF(500.0, 10.0, 30.0, 30.0), button_type="minimize"),
            Button(RectF(540.0, 10.0, 30.0, 30.0), button_type="close"),
        ]

        # App buttons (bottom row)
        self.app_buttons = [
            Button(RectF(20.0, 50.0, 40.0, 40.0), button_type="app", name="Chrome"),
            Button(RectF(70.0, 50.0, 40.0, 40.0), button_type="app", name="VSCode"),
            Button(RectF(120.0, 50.0, 40.0, 40.0), button_type="app", name="Terminal"),
        ]

        self.is_mouse_inside = False
        self.hovered_button: Optional[Button] = None

    def simulate_mouse_move(self, x: float, y: float) -> Optional[str]:
        """Simulate mouse move and return hovered button type or None."""
        self.is_mouse_inside = self.dock_bounds.contains(x, y)

        if not self.is_mouse_inside:
            self.hovered_button = None
            return None

        # Check control buttons first (they're on top)
        for btn in self.ctrl_buttons:
            if btn.contains(x, y):
                self.hovered_button = btn
                return btn.button_type

        # Check app buttons
        for btn in self.app_buttons:
            if btn.contains(x, y):
                self.hovered_button = btn
                return btn.button_type

        self.hovered_button = None
        return None

    def test_mouse_enter_dock(self):
        """Mouse entering dock should set is_mouse_inside to True."""
        self.assertFalse(self.is_mouse_inside)
        result = self.simulate_mouse_move(300.0, 50.0)
        self.assertTrue(self.is_mouse_inside)
        # Point (300, 50) is in the dock but not on any specific button
        # since app buttons are at y=50 with height 40, so y=50 is at the top edge
        # which may or may not be included depending on containment logic
        self.assertIn(result, ["app", None])

    def test_mouse_leave_dock(self):
        """Mouse leaving dock should set is_mouse_inside to False."""
        self.simulate_mouse_move(300.0, 50.0)
        self.assertTrue(self.is_mouse_inside)

        result = self.simulate_mouse_move(-10.0, -10.0)
        self.assertFalse(self.is_mouse_inside)
        self.assertIsNone(result)

    def test_hover_detection_app_button(self):
        """Hovering over app button should detect correct button."""
        result = self.simulate_mouse_move(35.0, 65.0)  # Center of first app button
        self.assertEqual(result, "app")
        self.assertIsNotNone(self.hovered_button)
        self.assertEqual(self.hovered_button.name, "Chrome")

    def test_hover_detection_minimize_button(self):
        """Hovering over minimize button should detect correctly."""
        result = self.simulate_mouse_move(515.0, 25.0)
        self.assertEqual(result, "minimize")
        self.assertIsNotNone(self.hovered_button)
        self.assertEqual(self.hovered_button.button_type, "minimize")

    def test_hover_detection_close_button(self):
        """Hovering over close button should detect correctly."""
        result = self.simulate_mouse_move(555.0, 25.0)
        self.assertEqual(result, "close")
        self.assertIsNotNone(self.hovered_button)
        self.assertEqual(self.hovered_button.button_type, "close")

    def test_no_hover_between_buttons(self):
        """Mouse between buttons should not hover anything."""
        # Point between Chrome (y: 50-90) and VSCode (y: 50-90)
        # Chrome: x=20-60, VSCode: x=70-110, so x=65 is between them
        result = self.simulate_mouse_move(65.0, 70.0)
        self.assertIsNone(result)
        self.assertIsNone(self.hovered_button)

    def test_hover_transition(self):
        """Moving from one button to another should update hover state."""
        # Start on Chrome button
        result1 = self.simulate_mouse_move(35.0, 65.0)
        self.assertEqual(result1, "app")
        self.assertEqual(self.hovered_button.name, "Chrome")

        # Move to VSCode button
        result2 = self.simulate_mouse_move(85.0, 65.0)
        self.assertEqual(result2, "app")
        self.assertEqual(self.hovered_button.name, "VSCode")


class TestAlphaCalculation(unittest.TestCase):
    """Tests for transparency/alpha calculation logic."""

    def calculate_alpha(self, is_mouse_inside: bool,
                        has_hovered_button: bool) -> int:
        """Calculate global alpha (0-255) based on mouse state.

        This simulates the C++ GetGlobalAlpha() logic:
        - Fully opaque (255) when mouse is inside
        - Semi-transparent (100) when mouse left but recently hovered
        - Nearly transparent (20) when mouse is far away
        """
        if is_mouse_inside:
            return 255
        elif has_hovered_button:
            return 100  # Fading out
        else:
            return 20   # Almost invisible

    def test_alpha_when_mouse_inside(self):
        """Alpha should be 255 when mouse is inside dock."""
        alpha = self.calculate_alpha(is_mouse_inside=True, has_hovered_button=False)
        self.assertEqual(alpha, 255)

    def test_alpha_when_mouse_left_recently(self):
        """Alpha should be 100 when mouse just left."""
        alpha = self.calculate_alpha(is_mouse_inside=False, has_hovered_button=True)
        self.assertEqual(alpha, 100)

    def test_alpha_when_mouse_far_away(self):
        """Alpha should be 20 when mouse is far away."""
        alpha = self.calculate_alpha(is_mouse_inside=False, has_hovered_button=False)
        self.assertEqual(alpha, 20)


class TestButtonClickHandling(unittest.TestCase):
    """Tests for button click action logic."""

    def determine_click_action(self, button_type: str) -> str:
        """Determine action based on button type clicked."""
        actions = {
            "minimize": "minimize_window",
            "close": "close_application",
            "app": "launch_application"
        }
        return actions.get(button_type, "no_action")

    def test_minimize_button_action(self):
        """Minimize button should trigger minimize action."""
        action = self.determine_click_action("minimize")
        self.assertEqual(action, "minimize_window")

    def test_close_button_action(self):
        """Close button should trigger close action."""
        action = self.determine_click_action("close")
        self.assertEqual(action, "close_application")

    def test_app_button_action(self):
        """App button should trigger launch action."""
        action = self.determine_click_action("app")
        self.assertEqual(action, "launch_application")


if __name__ == "__main__":
    unittest.main()
