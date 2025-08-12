## Usage
User only needs to set the init, draw and cleanup functions. Other DRM-GBM-EGL functions are hidden from the user. init function initializes the needed GL program variables and it is called only once before the main loop. The draw function calls the required OpenGL calls to render and it is called for each frame. The cleanup function is used for cleaning up the initialized GL variables.

User can set up keyboard and mouse callback functions for handling inputs.

## Build
```
mkdir -p build
cd build
cmake ..
make
```
