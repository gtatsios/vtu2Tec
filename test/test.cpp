
#include <vtu2Tec.h>
#include <iostream>

int main() {

    vtu2Tec::vtu2Tec("mesh_tri_2D.vtu");
    std::cout << "Converted 2D Triangular mesh \n";
    vtu2Tec::vtu2Tec("mesh_quad_2D.vtu");
    std::cout << "Converted 2D Quadrilateral mesh \n";
    vtu2Tec::vtu2Tec("mesh_tet.vtu");
    std::cout << "Converted 3D Tetrahedral mesh \n";
    vtu2Tec::vtu2Tec("mesh_hex.vtu");
    std::cout << "Converted 3D Hexahedral mesh \n";

}
