#ifndef VTU2TEC
#define VTU2TEC

#include <vector>
#include <array>

namespace vtu2Tec{

class doubleScalarField{
    public:
        std::vector<double> values;
        std::string name;
};

class doubleVectorField{
    public:
        std::vector<std::array<double, 3>> values;
        std::string name;
};

class doubleTensorField{
    public:
        std::vector<std::array<double, 9>> values;
        std::string name;
};

class vtu2Tec{
    public:
        std::string vtuFile, tecFile;
        std::vector<std::array<double,3>> nodeCoordinates;
        std::vector<bool> nodePartOfTriangular;
        std::vector<bool> nodePartOfQuad;
        std::vector<bool> nodePartOfHexa;
        std::vector<bool> nodePartOfTetra;
        std::vector<std::vector<int>> connectivity;
        std::vector<int> cellTypes;
        std::vector<int> offsets;
        std::vector<doubleScalarField> scalarFields;
        std::vector<doubleVectorField> vectorFields;
        std::vector<doubleTensorField> tensorFields;
        bool hasVector, hasTensor, hasScalar;

        int numNodes, numCells;

        vtu2Tec(std::string);
        void findGeneralInfo();
        void parseNodes();
        void parseCells();
        void parseFields();
        void writeTecFile();
        void writePointToFile2D(std::ofstream&, int);
        void writePointToFile3D(std::ofstream&, int);
};
}
#endif