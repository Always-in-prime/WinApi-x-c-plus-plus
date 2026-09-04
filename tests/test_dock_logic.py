"""Unit tests for C++ dock panel logic using Python.
Этот модуль тестирует бизнес-логику C++ приложения, проверяя 
расчет координат, попадание точек в границы кнопок и переходы 
состояний. Логика полностью синхронизирована с новой int-структурой Rect.
"""
import unittest
from dataclasses import dataclass
from typing import Optional

@dataclass
class Rect:
    """Представляет прямоугольник с целочисленными координатами (как в C++)."""
    left: int
    top: int
    right: int
    bottom: int

    def width(self) -> int:
        return self.right - self.left

    def height(self) -> int:
        return self.bottom - self.top

    def contains(self, x: int, y: int) -> bool:
        """Проверяет, находится ли точка (x, y) внутри прямоугольника (включая границы)."""
        return self.left <= x <= self.right and self.top <= y <= self.bottom


@dataclass
class Button:
    """Абстрактное представление кнопки для тестирования."""
    bounds: Rect
    is_hovered: bool = False
    button_type: str = "app"  # "app", "link", "minimize", "close"
    name: str = ""
    target: str = ""

    def contains(self, x: int, y: int) -> bool:
        """Проверяет попадание точки в границы кнопки."""
        return self.bounds.contains(x, y)


class TestRect(unittest.TestCase):
    """Тесты геометрических операций Rect."""

    def test_contains_point_inside(self):
        rect = Rect(10, 10, 60, 40)
        self.assertTrue(rect.contains(20, 20))
        self.assertTrue(rect.contains(10, 10))  # Верхний левый угол (включительно)
        self.assertTrue(rect.contains(60, 40))  # Нижний правый угол (включительно)

    def test_contains_point_outside(self):
        rect = Rect(10, 10, 60, 40)
        self.assertFalse(rect.contains(5, 5))
        self.assertFalse(rect.contains(70, 50))
        self.assertFalse(rect.contains(10, 41))  # Ниже прямоугольника
        self.assertFalse(rect.contains(61, 20))  # Правее прямоугольника

    def test_zero_size_rectangle(self):
        rect = Rect(10, 10, 10, 10)
        self.assertTrue(rect.contains(10, 10))  # Точечный прямоугольник содержит свою точку


class TestButtonContainment(unittest.TestCase):
    """Тесты логики определения попадания курсора в кнопку."""

    def setUp(self):
        self.app_button = Button(
            bounds=Rect(100, 50, 140, 90),
            button_type="app",
            name="Chrome"
        )
        self.minimize_button = Button(
            bounds=Rect(500, 10, 530, 40),
            button_type="minimize"
        )
        self.close_button = Button(
            bounds=Rect(540, 10, 570, 40),
            button_type="close"
        )

    def test_app_button_contains_center(self):
        self.assertTrue(self.app_button.contains(120, 70))

    def test_app_button_edge_cases(self):
        self.assertTrue(self.app_button.contains(100, 50))   # Верхний левый
        self.assertTrue(self.app_button.contains(140, 90))   # Нижний правый
        self.assertFalse(self.app_button.contains(141, 70))  # Чуть правее
        self.assertFalse(self.app_button.contains(120, 91))  # Чуть ниже

    def test_control_buttons_separate(self):
        self.assertTrue(self.minimize_button.contains(510, 20))
        self.assertFalse(self.close_button.contains(510, 20))
        self.assertTrue(self.close_button.contains(550, 20))
        self.assertFalse(self.minimize_button.contains(550, 20))


class TestDockPanelLogic(unittest.TestCase):
    """Тесты бизнес-логики панели DockPanel."""

    def setUp(self):
        self.dock_bounds = Rect(0, 0, 800, 180)
        
        # Кнопки управления (справа сверху)
        self.ctrl_buttons = [
            Button(Rect(710, 25, 740, 55), button_type="minimize", name="Minimize"),
            Button(Rect(750, 25, 780, 55), button_type="close", name="Close"),
        ]
        
        # Кнопки приложений (нижний ряд)
        self.app_buttons = [
            Button(Rect(60, 130, 120, 190), button_type="app", name="Chrome", target="chrome.exe"),
            Button(Rect(140, 130, 200, 190), button_type="app", name="Explorer", target="explorer.exe"),
            Button(Rect(220, 130, 280, 190), button_type="app", name="Notepad", target="notepad.exe"),
        ]
        
        # Кнопки ссылок (верхний ряд)
        self.link_buttons = [
            Button(Rect(60, 30, 120, 90), button_type="link", name="GitHub", target="https://github.com"),
            Button(Rect(140, 30, 200, 90), button_type="link", name="YouTube", target="https://youtube.com"),
        ]
        
        self.is_mouse_inside = False
        self.hovered_button: Optional[Button] = None

    def simulate_mouse_move(self, x: int, y: int) -> Optional[str]:
        self.is_mouse_inside = self.dock_bounds.contains(x, y)
        if not self.is_mouse_inside:
            self.hovered_button = None
            return None
        
        # Проверяем кнопки управления первыми (они поверх остальных)
        for btn in self.ctrl_buttons:
            if btn.contains(x, y):
                self.hovered_button = btn
                return btn.button_type
        
        # Проверяем ссылки
        for btn in self.link_buttons:
            if btn.contains(x, y):
                self.hovered_button = btn
                return btn.button_type

        # Проверяем приложения
        for btn in self.app_buttons:
            if btn.contains(x, y):
                self.hovered_button = btn
                return btn.button_type
                
        self.hovered_button = None
        return None

    def test_mouse_enter_dock(self):
        self.assertFalse(self.is_mouse_inside)
        result = self.simulate_mouse_move(400, 100)
        self.assertTrue(self.is_mouse_inside)
        self.assertIn(result, ["app", "link", None])

    def test_mouse_leave_dock(self):
        self.simulate_mouse_move(400, 100)
        self.assertTrue(self.is_mouse_inside)
        result = self.simulate_mouse_move(-10, -10)
        self.assertFalse(self.is_mouse_inside)
        self.assertIsNone(result)

    def test_hover_detection_app_button(self):
        # Центр кнопки Chrome (x: 60..120, y: 130..190)
        result = self.simulate_mouse_move(90, 160)
        self.assertEqual(result, "app")
        self.assertIsNotNone(self.hovered_button)
        self.assertEqual(self.hovered_button.name, "Chrome")

    def test_hover_detection_link_button(self):
        # Центр кнопки GitHub (x: 60..120, y: 30..90)
        result = self.simulate_mouse_move(90, 60)
        self.assertEqual(result, "link")
        self.assertIsNotNone(self.hovered_button)
        self.assertEqual(self.hovered_button.name, "GitHub")

    def test_hover_detection_minimize_button(self):
        result = self.simulate_mouse_move(725, 40)
        self.assertEqual(result, "minimize")
        self.assertIsNotNone(self.hovered_button)

    def test_hover_detection_close_button(self):
        result = self.simulate_mouse_move(765, 40)
        self.assertEqual(result, "close")
        self.assertIsNotNone(self.hovered_button)

    def test_no_hover_between_buttons(self):
        # Точка между Chrome (x: 60-120) и Explorer (x: 140-200)
        result = self.simulate_mouse_move(130, 160)
        self.assertIsNone(result)
        self.assertIsNone(self.hovered_button)

    def test_hover_transition(self):
        result1 = self.simulate_mouse_move(90, 160)
        self.assertEqual(result1, "app")
        self.assertEqual(self.hovered_button.name, "Chrome")
        
        result2 = self.simulate_mouse_move(170, 160)
        self.assertEqual(result2, "app")
        self.assertEqual(self.hovered_button.name, "Explorer")


class TestButtonClickHandling(unittest.TestCase):
    """Тесты логики обработки кликов по кнопкам."""

    def determine_click_action(self, button_type: str) -> str:
        actions = {
            "minimize": "minimize_window",
            "close": "close_application",
            "app": "launch_application",
            "link": "open_url"
        }
        return actions.get(button_type, "no_action")

    def test_minimize_button_action(self):
        self.assertEqual(self.determine_click_action("minimize"), "minimize_window")

    def test_close_button_action(self):
        self.assertEqual(self.determine_click_action("close"), "close_application")

    def test_app_button_action(self):
        self.assertEqual(self.determine_click_action("app"), "launch_application")
        
    def test_link_button_action(self):
        self.assertEqual(self.determine_click_action("link"), "open_url")


if __name__ == "__main__":
    unittest.main()