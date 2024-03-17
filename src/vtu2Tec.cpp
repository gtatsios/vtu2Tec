#include "vtu2Tec.h"
#include <iostream>
#include <fstream>
#include <regex>

namespace vtu2Tec{

vtu2Tec::vtu2Tec(std::string inputFile){
    vtuFile = inputFile;
    
    std::ifstream file(vtuFile);
    if(!file.good()) throw std::runtime_error("File '" + vtuFile + "' does not exist");
    
    findGeneralInfo();
    parseNodes();
    parseCells();
    parseFields();
    writeTecFile();
}

void vtu2Tec::findGeneralInfo(){
    std::ifstream infile(vtuFile);
    std::string line;
    std::regex pattern(R"(<Piece\s+NumberOfPoints="(\d+)\"\s+NumberOfCells="(\d+)\">)");
    
    while(std::getline(infile,line)){
        std::smatch match;
        if(std::regex_search(line, match, pattern)){
            numNodes = std::stoi(match[1]);
            numCells = std::stoi(match[2]);
            break;
        }
    }
    infile.close();
}

void vtu2Tec::parseNodes(){
    std::ifstream infile(vtuFile);
    std::string line;
    bool foundCoords = false;
    bool foundNodes = false;
    std::array<double, 3> coordinates;
    int nodeCounter = 0;

    while(std::getline(infile,line)){
        if(foundNodes){
            foundCoords = true;
            foundNodes = false;
            continue;
        }
        if(line=="<Points>"){
            foundNodes = true;
            continue;
        }
        if(foundCoords){
            std::istringstream iss(line);
            for(int i = 0; i< 3; i++){
                if (!(iss >> coordinates[i])) {
                    std::cerr << "Error reading values from line. " << line << std::endl;
                    break;
                }
            }
            nodeCoordinates.emplace_back(coordinates);
            nodeCounter += 1;
        }
        if(nodeCounter == numNodes) break;
    }
    infile.close();
}

void vtu2Tec::parseCells(){
    std::ifstream infile(vtuFile);
    std::string line;
    bool foundConnectivity = false;
    bool foundCellTypes = false;
    bool foundOffsets = false;
    bool insideCells = false;
    int cellCounter = 0;

    while(std::getline(infile,line)){

        if(line=="<Cells>"){
            insideCells = true;
        }
        if(line=="</Cells>") break;

        if(!insideCells) continue;
        if(line.find("connectivity") != std::string::npos) { 
            foundConnectivity = true;
            cellCounter = 0;
            continue;
        }
        if(foundConnectivity){
            std::istringstream iss(line);
            std::vector<int> con;
            int nod;
            while (iss >> nod) {
                con.emplace_back(nod);
            }
            connectivity.emplace_back(con);
            cellCounter += 1;
            if(cellCounter == numCells) foundConnectivity = false;
        }

        if(line.find("types") != std::string::npos) {
            foundCellTypes = true;
            cellCounter = 0;
            continue;
        }
        if(foundCellTypes){
            cellTypes.emplace_back(std::stoi(line));
            cellCounter += 1;
            if(cellCounter == numCells) foundCellTypes = false;
        }

        if(line.find("offsets") != std::string::npos){
            foundOffsets = true;
            cellCounter = 0;
            continue;
        }
        if(foundOffsets){
            offsets.emplace_back(std::stoi(line));
            cellCounter += 1;
            if(cellCounter == numCells) foundOffsets = false;
        }


    }
    infile.close();
}

void vtu2Tec::parseFields(){
    std::ifstream infile(vtuFile);
    std::string line;
    int nodeCounter = 0;
    bool foundField = false;
    bool inPointData = false;
    std::string fieldName;
    std::string fieldType;
    doubleScalarField tempScalarField;
    doubleVectorField tempVectorField;
    doubleTensorField tempTensorField;
    double scalarValue;
    std::array<double, 3> vectorValue;
    std::array<double, 9> tensorValue;
    double tensorElement;
    int tensorPositionCounter = 0;

    while(std::getline(infile,line)){

        if(line.find("<PointData") != std::string::npos) {
            inPointData = true;
        }

        if(line.find("</PointData>") != std::string::npos) {
            inPointData = false;
        }

        if(!inPointData) continue;
        if(!foundField){
            if(line.find("<DataArray type=\"Float64\" Name") != std::string::npos){
                foundField = true;
                size_t pos = line.find("Name=") + 6;
                size_t endPos = line.find('"',pos);
                fieldName = line.substr(pos, endPos-pos);
                pos = line.find("NumberOfComponents=") + 20;
                std::istringstream iss(line.substr(pos));
                int numComps;
                if(!(iss >> numComps)){
                    std::cout << "failed to parse integer \n";
                }
                if (numComps == 1) {
                    fieldType = "scalar";
                    tempScalarField.values.clear();
                    tempScalarField.name = fieldName;
                } else if (numComps == 3) {
                    fieldType = "vector";
                    tempVectorField.values.clear();
                    tempVectorField.name = fieldName;
                } else if (numComps == 9) {
                    fieldType = "tensor";
                    tempTensorField.values.clear();
                    tempTensorField.name = fieldName;
                }
                continue;
            }
        }

        if(foundField){
            if(fieldType=="scalar"){
                std::istringstream iss(line);
                if (!(iss >> scalarValue)) {
                    std::cerr << "Failed to parse double from line: " << line << std::endl;
                }
                tempScalarField.values.emplace_back(scalarValue);
                nodeCounter += 1;
            } else if (fieldType == "vector"){
                std::istringstream iss(line);
                for(int i = 0; i<3; i++){
                    if (!(iss >> vectorValue[i])) {
                        std::cerr << "Failed to parse triplet of doubles from line: " << line << std::endl;
                    }
                }
                tempVectorField.values.emplace_back(vectorValue);
                nodeCounter += 1;
            } else if (fieldType == "tensor"){
                std::istringstream iss(line);
                while(iss >> tensorElement){
                    tensorValue[tensorPositionCounter++] = tensorElement;
                }

                if(tensorPositionCounter==9) {
                    tensorPositionCounter = 0;
                    nodeCounter += 1;
                    tempTensorField.values.emplace_back(tensorValue);
                }
            }

            if(nodeCounter==numNodes){
                nodeCounter = 0;
                foundField = false;
                if(fieldType=="scalar") {
                    scalarFields.emplace_back(tempScalarField);
                } else if(fieldType=="vector") {
                    vectorFields.emplace_back(tempVectorField);
                }else if(fieldType=="tensor") {
                    tensorFields.emplace_back(tempTensorField);
                }
            }
        }

    }

    infile.close();
};

void vtu2Tec::writeTecFile(){
    tecFile = vtuFile;
    tecFile.replace(tecFile.length()-3,3, "dat");

    hasScalar = scalarFields.size()>0; 
    hasVector = vectorFields.size()>0; 
    hasTensor = tensorFields.size()>0; 
    
    bool hasTriangle = false, hasQuad = false, hasTetra = false, hasHexa = false;
    int nTriangularCells = 0, nQuadCells = 0, nTetraCells = 0, nHexaCells = 0;
    for (int i = 0; i<cellTypes.size();i++){
        if(cellTypes[i]==5) {
            nTriangularCells += 1;
            hasTriangle=true;
        } else if(cellTypes[i]==9) {
            nQuadCells += 1;
            hasQuad=true;
        } else if(cellTypes[i]==10) {
            nTetraCells += 1;
            hasTetra=true;
        } else if(cellTypes[i]==12) {
            nHexaCells += 1;
            hasHexa=true;
        }
    }
    nodePartOfTriangular.resize(numNodes, false);
    nodePartOfQuad.resize(numNodes, false);
    nodePartOfTetra.resize(numNodes, false);
    nodePartOfHexa.resize(numNodes, false);
    for(int i = 0; i<numCells; i++){
        for(auto& n : connectivity[i]){
            if(cellTypes[i]==5){
                nodePartOfTriangular[n] = true;
            } else if(cellTypes[i]==9){
                nodePartOfQuad[n] = true;
            } else if(cellTypes[i]==10){
                nodePartOfTetra[n] = true;
            } else if(cellTypes[i]==12){
                nodePartOfHexa[n] = true;
            }
        }
    }

    int nTriangularNodes = 0, nQuadNodes = 0, nTetraNodes = 0, nHexaNodes = 0;
    //this information is only useful when the mesh has more than one kind of element
    std::vector<int> triangularNodesCrossRefArray(numNodes);
    std::vector<int> quadNodesCrossRefArray(numNodes);
    std::vector<int> tetraNodesCrossRefArray(numNodes);
    std::vector<int> hexaNodesCrossRefArray(numNodes);

    std::vector<int> inverseTriangularNodesCrossRefArray(numNodes);
    std::vector<int> inverseQuadNodesCrossRefArray(numNodes);
    std::vector<int> inverseTetraNodesCrossRefArray(numNodes);
    std::vector<int> inverseHexaNodesCrossRefArray(numNodes);

    for (int i=0; i<numNodes; i++){
        if(nodePartOfTriangular[i]) {
            triangularNodesCrossRefArray[nTriangularNodes] = i;
            inverseTriangularNodesCrossRefArray[i] = nTriangularNodes;
            nTriangularNodes += 1;
        } else if(nodePartOfQuad[i]) {
            quadNodesCrossRefArray[nQuadNodes] = i;
            inverseQuadNodesCrossRefArray[i] = nQuadNodes;
            nQuadNodes += 1;
        } else if(nodePartOfTetra[i]) {
            tetraNodesCrossRefArray[nTetraNodes] = i;
            inverseTetraNodesCrossRefArray[i] = nTetraNodes;
            nTetraNodes += 1;
        } else if(nodePartOfHexa[i]) {
            hexaNodesCrossRefArray[nHexaNodes] = i;
            inverseHexaNodesCrossRefArray[i] = nHexaNodes;
            nHexaNodes += 1;
        } 
    }

    bool is2D = nTriangularCells>0 || nQuadCells >0;
    bool is3D = nTetraCells>0 || nHexaCells>0;

    if( is2D && is3D ){
        std::cerr << "Found both 2D and 3D cells \n";
    }

    std::ofstream outfile(tecFile);
    outfile << std::scientific;

    if(is2D) outfile << "Variables = x, y, ";
    if(is3D) outfile << "Variables = x, y, z, ";
    //write the variable list. yes it takes this much
    if(hasScalar){
        for(int i = 0; i<scalarFields.size()-1; i++){
            outfile << scalarFields[i].name << ", ";
        }
        if(hasVector || hasTensor){
            outfile << scalarFields[scalarFields.size()-1].name << ", ";
        } else {
            outfile << scalarFields[scalarFields.size()-1].name << "\n";
        }
    }

    if(hasVector){
        for(int i = 0; i<vectorFields.size()-1;i++){
            outfile << vectorFields[i].name << "_x, " << vectorFields[i].name << "_y, " << vectorFields[i].name << "_z, ";
        }
        if(hasTensor){
            int i = vectorFields.size()-1;
            outfile << vectorFields[i].name << "_x, " << vectorFields[i].name << "_y, " << vectorFields[i].name << "_z, ";
        } else {
            int i = vectorFields.size()-1;
            outfile << vectorFields[i].name << "_x, " << vectorFields[i].name << "_y, " << vectorFields[i].name << "_z \n";
        }
    }

    if(hasTensor){
        for(int i = 0; i<tensorFields.size()-1;i++){
            outfile << tensorFields[i].name << "_xx, " << tensorFields[i].name << "_xy, " << tensorFields[i].name << "_xz, " 
            << tensorFields[i].name << "_yx, " << tensorFields[i].name << "_yy, " << tensorFields[i].name << "_yz, "
            << tensorFields[i].name << "_zx, " << tensorFields[i].name << "_zy, " << tensorFields[i].name << "_zz, ";
        }
        {
            int i = tensorFields.size()-1;
            outfile << tensorFields[i].name << "_xx, " << tensorFields[i].name << "_xy, " << tensorFields[i].name << "_xz, " 
            << tensorFields[i].name << "_yx, " << tensorFields[i].name << "_yy, " << tensorFields[i].name << "_yz, "
            << tensorFields[i].name << "_zx, " << tensorFields[i].name << "_zy, " << tensorFields[i].name << "_zz \n";
        }
    }


    //write triangular elements
    if(hasTriangle){
        outfile << "ZONE T=\"Triangular Elements\" F=FEPOINT, N = " << nTriangularNodes << ", E = " << nTriangularCells << ", ET = TRIANGLE \n";
        for(int i = 0; i<nTriangularNodes; i++){
            writePointToFile2D(outfile, triangularNodesCrossRefArray[i]);
        }
        for(int i = 0; i<numCells; i++){
            if(cellTypes[i] == 5){
                for(int j = 0; j<3; j++){
                    outfile << inverseTriangularNodesCrossRefArray[connectivity[i][j]] +1 << " ";
                }
                outfile << " \n";
            }
        }
    }

    //write quad elements
    if(hasQuad){
        outfile << "ZONE T=\"Quadrilateral Elements\" F=FEPOINT, N = " << nQuadNodes << ", E = " << nQuadCells << ", ET = QUADRILATERAL \n";
        for(int i = 0; i<nQuadNodes; i++){
            writePointToFile2D(outfile, quadNodesCrossRefArray[i]);
        }
        for(int i = 0; i<numCells; i++){
            if(cellTypes[i] == 9){
                for(int j = 0; j<4; j++){
                    outfile << inverseQuadNodesCrossRefArray[connectivity[i][j]] +1 << " ";
                }
                outfile << " \n";
            }
        }
    }

    //write tetra elements
    if(hasTetra){
        outfile << "ZONE T=\"Tetrahedral Elements\" F=FEPOINT, N = " << nTetraNodes << ", E = " << nTetraCells << ", ET = TETRAHEDRON \n";
        for(int i = 0; i<nTetraNodes; i++){
            writePointToFile3D(outfile, tetraNodesCrossRefArray[i]);
        }
        for(int i = 0; i<numCells; i++){
            if(cellTypes[i] == 10){
                for(int j = 0; j<4; j++){
                    outfile << inverseTetraNodesCrossRefArray[connectivity[i][j]] +1 << " ";
                }
                outfile << " \n";
            }
        }       
    }

    //write hexa elements
    if(hasHexa){
        outfile << "ZONE T=\"Hexahedral Elements\" F=FEPOINT, N = " << nHexaNodes << ", E = " << nHexaCells << ", ET = BRICK \n";
        for(int i = 0; i<nHexaNodes; i++){
            writePointToFile3D(outfile, hexaNodesCrossRefArray[i]);
        }
        for(int i = 0; i<numCells; i++){
            if(cellTypes[i] == 12){
                for(int j = 0; j<8; j++){
                    outfile << inverseHexaNodesCrossRefArray[connectivity[i][j]] +1 << " ";
                }
                outfile << " \n";
            }
        }       
    }

    outfile.close();
    std::cout << "done writting \n";
};

void vtu2Tec::writePointToFile2D(std::ofstream& outfile, int j){
        
    outfile << nodeCoordinates[j][0] << ", " << nodeCoordinates[j][1] << ", " ;

        if(hasScalar){
        for(int i = 0; i<scalarFields.size()-1; i++){
            outfile << scalarFields[i].values[j] << ", ";
        }
        if(hasVector || hasTensor){
            outfile << scalarFields[scalarFields.size()-1].values[j] << ", ";
        } else {
            outfile << scalarFields[scalarFields.size()-1].values[j] << "\n";
        }
    }
    
    if(hasVector){
        for(int i = 0; i<vectorFields.size()-1;i++){
            for(int k = 0; k<3; k++){
                outfile << vectorFields[i].values[j][k] << ", ";
            }
        }
        if(hasTensor){
            int i = vectorFields.size()-1;
            for(int k = 0; k<3; k++){
                outfile << vectorFields[i].values[j][k] << ", ";
            }
        }else {
            int i = vectorFields.size()-1;
            for(int k = 0; k<2; k++){
                outfile << vectorFields[i].values[j][k] << ", ";
            }
            outfile << vectorFields[i].values[j][2] << "\n";
        }
    }

    if(hasTensor){
        for(int i = 0; i<tensorFields.size()-1;i++){
            for(int k = 0; k<9; k++){
                outfile << tensorFields[i].values[j][k] << ", ";
            }
        }
        {
            int i = tensorFields.size()-1;
            for(int k = 0; k<8; k++){
                outfile << tensorFields[i].values[j][k] << ", ";
            }
            outfile << tensorFields[i].values[j][8] << "\n";
        }
    }
}

void vtu2Tec::writePointToFile3D(std::ofstream& outfile, int j){
        
    outfile << nodeCoordinates[j][0] << ", " << nodeCoordinates[j][1] << ", " << nodeCoordinates[j][2] << ", ";

    if(hasScalar){
        for(int i = 0; i<scalarFields.size()-1; i++){
            outfile << scalarFields[i].values[j] << ", ";
        }
        if(hasVector || hasTensor){
            outfile << scalarFields[scalarFields.size()-1].values[j] << ", ";
        } else {
            outfile << scalarFields[scalarFields.size()-1].values[j] << "\n";
        }
    }
    
    if(hasVector){
        for(int i = 0; i<vectorFields.size()-1;i++){
            for(int k = 0; k<3; k++){
                outfile << vectorFields[i].values[j][k] << ", ";
            }
        }
        if(hasTensor){
            int i = vectorFields.size()-1;
            for(int k = 0; k<3; k++){
                outfile << vectorFields[i].values[j][k] << ", ";
            }
        }else {
            int i = vectorFields.size()-1;
            for(int k = 0; k<2; k++){
                outfile << vectorFields[i].values[j][k] << ", ";
            }
            outfile << vectorFields[i].values[j][2] << "\n";
        }
    }

    if(hasTensor){
        for(int i = 0; i<tensorFields.size()-1;i++){
            for(int k = 0; k<9; k++){
                outfile << tensorFields[i].values[j][k] << ", ";
            }
        }
        {
            int i = tensorFields.size()-1;
            for(int k = 0; k<8; k++){
                outfile << tensorFields[i].values[j][k] << ", ";
            }
            outfile << tensorFields[i].values[j][8] << "\n";
        }
    }
}

}
