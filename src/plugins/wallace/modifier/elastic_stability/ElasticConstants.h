#pragma once

#include <eigen3/Eigen/Eigen>
#include <eigen3/unsupported/Eigen/CXX11/Tensor>

class ElasticConstants {

public:
    //constructor
    ElasticConstants(int type, std::vector<float> soec, std::vector<float> toec) :
    _structure(type),
    _vectorSoec(soec),
    _vectorToec(toec)
    {
        generateSoec(_structure, _vectorSoec);
        generateToec(_structure, _vectorToec);
    }


    //returns the voigted second order elastic constants
    const Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> voigtSoec() const { return _voigtSoec; }

    //returns the voigted third order elastic constants
    const Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6, 6>> voigtToec() const { return _voigtToec; }

    //returns the full soec
    const Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> fullSoec() const { return _fullSoec; }

    //returns the full toec
    const Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> fullToec() const { return _fullToec; }

    enum LaueGroups {
        TRIGONAL = 1,
        MONOCLINIC = 2,
        ORTHORHOMBIC = 3,
        TETRAGONAL_LOW = 4,
        TETRAGONAL_HIGH = 5,
        RHOMBOHEDRAL_LOW = 6,
        RHOMBOHEDRAL_HIGH = 7,
        HEXAGONAL_LOW = 8,
        HEXAGONAL_HIGH = 9,
        CUBIC_LOW = 10,
        CUBIC_HIGH = 11,
        TRANSVERSELEY_ISOTROPIC = 12,
        ISOTROPIC = 13
    };

    static std::vector<std::string> uniqueSoec(int structure) {
        if (structure == 9) {
            std::vector<std::string> str = {"C11", "C12", "C13"};
            return str;
        }
        else {
            std::vector<std::string> str = {"Errpr"};
            return str;
        }
    }

    static int numberSoec(int structure) {
        if (structure == 9) { return 5; }
        else if (structure == 11) {return 3; }
        else return -1;
    }

    static int numberToec(int structure) {
        if (structure == 9) { return 10; }
        if (structure == 11) { return 6; }
    }

    //Gets voigt notation index from full notation indices
    static int vID(int i, int j) {
        if (i==j && i==0) { return 0; }
        else if (i==j && i==1) { return 1; }
        else if (i==j && i==2) { return 2; }
        else if (i != j && i+j==3) { return 3;}
        else if (i != j && i+j==2) { return 4;}
        else if (i != j && i+j==1) { return 5; }
    }



private:
    const int _structure;
    const std::vector<float> _vectorSoec;
    const std::vector<float> _vectorToec;
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> _voigtSoec;
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6, 6>> _voigtToec;
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> _fullSoec;
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> _fullToec;

    //Gets voigt notation index from full notation indices
    //int vID(int i, int j) {
      //  if (i==j && i==0) { return 0; }
        //else if (i==j && i==1) { return 1; }
        //else if (i==j && i==2) { return 2; }
        //else if (i != j && i+j==3) { return 3;}
        //else if (i != j && i+j==2) { return 4;}
        //else if (i != j && i+j==1) { return 5; }
    //}


    /**********************************************************************
    *Generate second order elastic constants in taco form from input type and vector
    ***********************************************************************/

    void generateSoec(int type, std::vector<float> soec) {
        Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> genVoigtSoec;
        genVoigtSoec.setZero();
        Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> genFullSoec;
        genFullSoec.setZero();
        float arraySoec [6][6] = { }; //Aggregate initialize to 0
        //0 -> C11, 1-> C12, etc..
        switch(type)
        {
        case 9: //High symmetry hexagonal - input vector is 5 indices
            {
            float A = (soec[0]-soec[1])/2;
            arraySoec[0][0] = soec[0];
            arraySoec[0][1] = soec[1];
            arraySoec[0][2] = soec[2];
            arraySoec[1][1] = soec[0];
            arraySoec[1][2] = soec[2];
            arraySoec[2][2] = soec[3];
            arraySoec[3][3] = soec[4];
            arraySoec[4][4] = soec[4];
            arraySoec[5][5] = A;
            break;
            }
        case 11: //High symmetry cubic - input vector is 3 indices
            {
            arraySoec[0][0] = soec[0];
            arraySoec[0][1] = soec[1];
            arraySoec[0][2] = soec[1];
            arraySoec[1][1] = soec[0];
            arraySoec[1][2] = soec[1];
            arraySoec[2][2] = soec[0];
            arraySoec[3][3] = soec[2];
            arraySoec[4][4] = soec[2];
            arraySoec[5][5] = soec[2];
            break;
            }
        }
        //Apply general symm
        for (int i = 0; i < 6; i++) {
            for (int j = i; j < 6; j++ ) {
                if (i!=j) { arraySoec[j][i] = arraySoec[i][j]; }
            }
        }
        //Insert Array into tensor type variable for voigt notation
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < 6; j++)
            {
                genVoigtSoec(i, j) = arraySoec[i][j];
            }
        }
         _voigtSoec = genVoigtSoec;
         //_voigtSoec.pack();

        //Insert Array into tensor type variable for full notation
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 3; k++)
                {
                    for (int l = 0; l < 3; l++)
                    {
                       int x = vID(i, j);
                       int y = vID(k, l);
                        genFullSoec(i, j, k, l) = arraySoec[x][y];
                    }
                }
            }
        }
        _fullSoec = genFullSoec;
        //_fullSoec.pack();
    }



    /**********************************************************************
    *Generate third order elastic constants
    ***********************************************************************/
    void generateToec(int type, std::vector<float> toec) {
        Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6, 6>> genVoigtToec;
        genVoigtToec.setZero();
        Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> genFullToec;
        genFullToec.setZero();
        float arrayToec [6][6][6] = { }; //Aggregate initialize to 0
        //0 -> C111, 1-> C112, etc...
        switch(type)
        {
        case 9: //High symmetry hexagonal - input vector is 10 indices
            {
            float A = toec[0]+ toec[1]-toec[7]; //C111+C112-C222
            float D = -(2*toec[0]+toec[1]-3*toec[7])/4;
            float I = (2*toec[0]-toec[1]-toec[7])/4;
            float J = (toec[2]-toec[3])/2;
            float K = -(toec[5]-toec[6])/2;
            arrayToec[0][0][0] = toec[0];
            arrayToec[0][0][1] = toec[1];
            arrayToec[0][0][2] = toec[2];
            arrayToec[0][1][1] = A;
            arrayToec[0][1][2] = toec[3];
            arrayToec[0][2][2] = toec[4];
            arrayToec[0][3][3] = toec[5];
            arrayToec[0][4][4] = toec[6];
            arrayToec[0][5][5] = D;
            arrayToec[1][1][1] = toec[7];
            arrayToec[1][1][2] = toec[2];
            arrayToec[1][2][2] = toec[4];
            arrayToec[1][3][3] = toec[6];
            arrayToec[1][4][4] = toec[5];
            arrayToec[1][5][5] = I;
            arrayToec[2][2][2] = toec[8];
            arrayToec[2][3][3] = toec[9];
            arrayToec[2][4][4] = toec[9];
            arrayToec[2][5][5] = J;
            arrayToec[3][4][5] = K;
            break;


            }
        case 11: //High symmetry cubic input toec vector is 6 indices
            {
            arrayToec[0][0][0] = toec[0];
            arrayToec[0][0][1] = toec[1];
            arrayToec[0][0][2] = toec[1];
            arrayToec[0][1][1] = toec[1];
            arrayToec[0][1][2] = toec[2];
            arrayToec[0][2][2] = toec[1];
            arrayToec[0][3][3] = toec[3];
            arrayToec[0][4][4] = toec[4];
            arrayToec[0][5][5] = toec[4];
            arrayToec[1][1][1] = toec[0];
            arrayToec[1][1][2] = toec[1];
            arrayToec[1][2][2] = toec[1];
            arrayToec[1][3][3] = toec[4];
            arrayToec[1][4][4] = toec[3];
            arrayToec[1][5][5] = toec[4];
            arrayToec[2][2][2] = toec[0];
            arrayToec[2][3][3] = toec[4];
            arrayToec[2][4][4] = toec[4];
            arrayToec[2][5][5] = toec[3];
            arrayToec[3][4][5] = toec[5];
            break;

            }

        }
        //Apply general symm
        for (int i = 0; i < 6; i++) {
            for (int j = i; j < 6; j++){
                for (int k = j; k < 6; k++ ) {
                    if (j!=k || i!=j) { arrayToec[i][k][j] = arrayToec[i][j][k];
                                arrayToec[k][i][j] = arrayToec[i][j][k];
                                arrayToec[j][i][k] = arrayToec[i][j][k];
                                arrayToec[k][j][i] = arrayToec[i][j][k];
                                arrayToec[j][k][i] = arrayToec[i][j][k];
                    }
                }
            }
        }


        //Insert Array into tensor type variable for voigt notation
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 6; j++) {
                for (int k = 0; k < 6; k++) {
                genVoigtToec(i, j, k) = arrayToec[i][j][k];
                }
            }
        }
         _voigtToec = genVoigtToec;
         //_voigtToec.pack();

        //Insert Array into tensor type variable for full notation
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                for (int k = 0; k < 3; k++) {
                    for (int l = 0; l < 3; l++) {
                        for (int m = 0; m < 3; m++) {
                            for (int n = 0; n < 3; n++) {
                                int x = vID(i, j);
                                int y = vID(k, l);
                                int z = vID(m, n);
                                genFullToec(i, j, k, l, m, n) = arrayToec[x][y][z];
                            }
                        }
                    }
                }
            }
        }
        _fullToec = genFullToec;
        //_fullToec.pack();
    }


};
