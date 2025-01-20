# Two Cars - C++ Clone

A C++ implementation of the popular mobile game **Two Cars**, where players control two cars simultaneously to collect circles and avoid squares. This project utilizes the SDL2 library for rendering and input handling.

---

## Features
- **Dual car controls**: Manage two cars simultaneously.
- **Dynamic gameplay**: Dodge obstacles and collect items to earn points.
- **Custom assets**: Includes custom sprites and audio.
- **Cross-platform compatibility**: Works on Windows (and other platforms with proper SDL2 setup).

---

## Prerequisites
Before running the game, ensure you have the following installed on your system:
- **C++ compiler** (e.g., `g++`, `clang`, or MSVC)
- **SDL2 library**  
  Install SDL2, SDL2_image, SDL2_mixer, and SDL2_ttf using:
  - Windows: Download from [SDL2 Downloads](https://www.libsdl.org/download-2.0.php)
  - Linux: Install via package manager:
    ```bash
    sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
    ```

---

## Setup and Usage

1. **Clone the repository**:
   ```bash
   git clone https://github.com/your-username/two-cars-cpp-clone.git
   cd two-cars-cpp-clone
   ```

2. **Running the game**:
   A Makefile is already in place for compiling the program. On Windows (with MinGW-64), you can run:
   ```bash
   mingw32-make
   ./main
   ```

   On Linux:
   ```bash
   make
   ./main
   ```

3. **Dependencies**:
   - Ensure `.dll` files for SDL2 (e.g., `SDL2.dll`, `SDL2_image.dll`) are in the same directory as the executable. If not included in the repository, download them from [SDL2 Downloads](https://www.libsdl.org/download-2.0.php).

---

## Controls
- **A**: Move the left car.
- **D**: Move the right car.

---

## Roadmap
- [x] Add increasing difficulty over time.
- [x] Implement score and high score tracking.
- [ ] Add a two-player mode.
- [x] Enhance graphics and animations.

---

## Contributing
Contributions are welcome! Feel free to submit pull requests or open issues to improve the game.

---

## License
This project is licensed under the [MIT License](LICENSE).

---

## Acknowledgments
- Original game inspiration: [Two Cars by Ketchapp](https://play.google.com/store/apps/details?id=com.ketchapp.twocars)
- **SDL2** for enabling this game's development.
