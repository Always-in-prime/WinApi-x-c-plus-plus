# Dock Panel C++ Application

A Windows dock panel application built with C++ and GDI+.

## Project Structure

```
/workspace
├── src/                    # C++ source files
│   ├── main.cpp           # Application entry point and message loop
│   ├── button.h           # Button class declarations
│   ├── button.cpp         # Button implementations
│   ├── dock_panel.h       # DockPanel class declaration
│   └── dock_panel.cpp     # DockPanel implementation
├── tests/                  # Python unit tests for C++ logic
│   ├── __init__.py
│   └── test_dock_logic.py # Logic tests (geometry, hover, alpha)
└── .github/workflows/      # CI/CD configuration
    └── cpp_tests.yml      # GitHub Actions workflow
```

## Testing

### Running Python Tests Locally

The project includes Python unit tests that validate the business logic of the C++ application:

```bash
# Run all tests with verbose output
python -m unittest discover -s tests -v

# Run specific test file
python -m unittest tests.test_dock_logic -v

# Run with flake8 linting
flake8 tests/ --max-line-length=100 --ignore=E501,W503
```

### Test Coverage

The Python tests cover:
- **Geometry operations**: Rectangle containment logic
- **Button hit detection**: Mouse coordinate to button mapping
- **Hover state management**: State transitions on mouse movement
- **Alpha transparency**: Opacity calculations based on mouse position
- **Click action handling**: Button type to action mapping

### CI/CD Pipeline

GitHub Actions automatically runs on every push and pull request:

1. **test-python-logic**: Runs Python unit tests on Ubuntu
2. **build-cpp-windows**: Builds C++ application on Windows
3. **code-quality**: Lints Python test files with flake8

## Building the C++ Application (Windows)

Requires Visual Studio with C++ workload and Windows SDK:

```bash
# Using MSBuild (if .sln file exists)
msbuild src/dock_panel.sln /p:Configuration=Release

# Or compile manually
g++ -o dock_panel.exe src/*.cpp -lgdiplus -mwindows
```

## Architecture

The codebase follows Google C++ Style Guide with clear separation of concerns:

- **Button classes** (`button.h/cpp`): Abstract base class and concrete implementations
- **DockPanel** (`dock_panel.h/cpp`): UI composition and event handling
- **Main** (`main.cpp`): Windows message loop and initialization

## Development Workflow

1. Make changes to C++ code in `src/`
2. Update Python tests in `tests/` to reflect logic changes
3. Run tests locally before committing
4. Push to trigger CI/CD pipeline

## Requirements

- **Runtime**: Windows 10+, GDI+
- **Development**: 
  - C++: Visual Studio 2019+ or MinGW
  - Testing: Python 3.8+, unittest (stdlib), flake8 (optional)
