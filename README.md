# vtu2Tec

vtu2Tec is a simple way to convert .vtu files to Tecplot format. It supports:
* triangular and quadrilateral 2D elements
* tetrahedral and hexahedral 3D elements
* scalar, vector and tensor fields
* only point data, cell data is not yet supported
* different types of cells in the mesh

It has been tested with the output files of [lean-vtk (original)](https://github.com/mmorse1217/lean-vtk), and [lean-vtk (with tensor support)](https://github.com/gtatsios/lean-vtk).

### Compiling and running

The easiest way to use vtu2Tec is to simply copy and paste include/vtu2Tec.h and src/vtu2tec.cpp into your project source code.

To compile a static library, run `make` in the project root. This will generate the `build` folder with the `libvtu2tec.a` library. 

To run the tests compile with `make tests` and run the `test` executable in the `build/test` folder. 

Tested with g++ 9.4.0.

### Example usage

The following code will convert the `example.vtu` to `example.dat` file for Tecplot.

```cpp

#include "vtu2Tec.h"

int main(){
    
    vtu2Tec::vtu2Tec convert("example.vtu");

    return 0;
}
```

### Contributing
Please fork the repo and make a pull request for any future changes.
