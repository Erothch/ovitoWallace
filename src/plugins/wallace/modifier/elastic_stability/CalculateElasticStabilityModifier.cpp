////////
// Written 2018 by Eric for use with Ovito Wallace Plugin
////////

#include <plugins/particles/Particles.h>
#include <core/dataset/pipeline/ModifierApplication.h>
#include <plugins/stdobj/simcell/SimulationCellObject.h>
#include <core/utilities/concurrent/ParallelFor.h>
#include "CalculateElasticStabilityModifier.h"
#include <eigen3/Eigen/Eigenvalues>
#include <eigen3/Eigen/LU>


namespace Ovito { namespace Particles { OVITO_BEGIN_INLINE_NAMESPACE(Modifiers) OVITO_BEGIN_INLINE_NAMESPACE(Analysis)

IMPLEMENT_OVITO_CLASS(CalculateElasticStabilityModifier);
DEFINE_PROPERTY_FIELD(CalculateElasticStabilityModifier, structure);
DEFINE_PROPERTY_FIELD(CalculateElasticStabilityModifier, soec);
DEFINE_PROPERTY_FIELD(CalculateElasticStabilityModifier, toec);
DEFINE_PROPERTY_FIELD(CalculateElasticStabilityModifier, CTransformation);
SET_PROPERTY_FIELD_LABEL(CalculateElasticStabilityModifier, CTransformation, "Transformation");

/****************************************************
 * Construct Modifier Object
 * **************************************************/
CalculateElasticStabilityModifier::CalculateElasticStabilityModifier(DataSet *dataset) : AsynchronousModifier(dataset),
    _structure(11),
    _soec(std::vector<float>({0, 0, 0})),
    _toec(std::vector<float>({0, 0, 0, 0, 0, 0})),
    _CTransformation(AffineTransformation::Identity())
{}

/******************************************************************************
* Creates and initializes a computation engine that will compute the modifier's results.
******************************************************************************/
Future<AsynchronousModifier::ComputeEnginePtr> CalculateElasticStabilityModifier::createEngine(TimePoint time, ModifierApplication* modApp, const PipelineFlowState& input)
{
// Get modifier input.
const ParticlesObject* particles = input.expectObject<ParticlesObject>();
const PropertyObject* posProperty = particles->expectProperty(ParticlesObject::PositionProperty);
const SimulationCellObject* simCell = input.expectObject<SimulationCellObject>();

//Get input from other modifiers
const PropertyObject* deformationGradientProperty = particles->expectProperty(ParticlesObject::DeformationGradientProperty);
if(!deformationGradientProperty)
    throwException(tr("Requires calculated deformation gradients"));

const PropertyObject* atomicStrainProperty = particles->expectProperty(ParticlesObject::StrainTensorProperty);
if(!atomicStrainProperty)
    throwException(tr("Requires atomic strain tensors to be calculated"));



// Create engine object. Pass all relevant modifier parameters to the engine as well as the input data.
return std::make_shared<ElasticStabilityEngine>(particles, posProperty->storage(), deformationGradientProperty->storage(),
            atomicStrainProperty->storage(), simCell->data(), structure(), soec(), toec(), CTransformation());
}

/******************************************************************************
* Performs the actual computation. This method is executed in a worker thread.
******************************************************************************/
void CalculateElasticStabilityModifier::ElasticStabilityEngine::perform()
{

    task()->setProgressText(tr("Setting up calculations"));

    //Get transformation matrix
    _ETransformation.setZero();
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            _ETransformation(i, j) = static_cast<float>(_CTransformation[i][j]);
        }
    }

    //Get approp elastic constants
    ElasticConstants elasticConstants(_structure, _soec, _toec);

    Eigen::array<Eigen::IndexPair<int>, 1> pdT1 = { Eigen::IndexPair<int>(1, 0) };
    Eigen::array<Eigen::IndexPair<int>, 1> pdT2 = { Eigen::IndexPair<int>(1, 1) };
    Eigen::array<Eigen::IndexPair<int>, 1> pdT3 = { Eigen::IndexPair<int>(1, 2) };
    Eigen::array<Eigen::IndexPair<int>, 1> pdT4 = { Eigen::IndexPair<int>(1, 3) };
    Eigen::array<Eigen::IndexPair<int>, 1> pdT5 = { Eigen::IndexPair<int>(1, 4) };
    Eigen::array<Eigen::IndexPair<int>, 1> pdT6 = { Eigen::IndexPair<int>(1, 5) };


    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> fC2 = elasticConstants.fullSoec();
    //fC2 =  (((fC2.contract(_ETransformation, pdT1)).contract(_ETransformation, pdT2)).contract(_ETransformation, pdT3)).contract(_ETransformation, pdT4);
    //fC2 = _ETransformation.contract((_ETransformation.contract(_ETransformation.contract(_ETransformation.contract(fC2, pdT4).eval(), pdT4).eval(), pdT4)),pdT4).eval();
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> fC3 = elasticConstants.fullToec();
    //fC3 = _ETransformation.contract(_ETransformation.contract(_ETransformation.contract(_ETransformation.contract(_ETransformation.contract(_ETransformation.contract(fC3, pdT6).eval(), pdT6).eval(), pdT6).eval(), pdT6).eval(), pdT6).eval(), pdT6).eval();
    for (int i=0; i<4; i++) {
        fC2 = _ETransformation.contract(fC2, pdT4).eval();
    }
    for (int i=0; i<6; i++) {
        fC3 = _ETransformation.contract(fC3, pdT6).eval();
    }
    //fC3 = fC3.contract(_ETransformation, pdT1).eval();

    //put into voigt forms
    _vC2.setZero();
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            for (int k=0; k<3; k++){
                for (int l=0; l<3; l++)
                {   int w = ElasticConstants::vID(i, j);
                    int v = ElasticConstants::vID(k, l);
                    //WTensor(w, v)= L(i, j, k, l);
                    _vC2(w, v) = fC2(i, j, k, l);
                }
            }
        }
    }


    _vC3.setZero();
    for (int i=0; i<3; i++) {
        for (int j=0; j< 3; j++) {
            for (int k=0; k < 3; k++){
                for (int l=0; l < 3; l++) {
                    for (int m = 0; m < 3; m++) {
                        for (int n = 0; n < 3; n++) {
                            int v = ElasticConstants::vID(i, j);
                            int w = ElasticConstants::vID(k, l);
                            int x = ElasticConstants::vID(m, n);
                            _vC3(v, w, x) = fC3(i, j, k, l, m, n);
                        }
                    }
                }
            }
        }
    }


    Eigen::MatrixXf soecMatrix = Eigen::MatrixXf::Zero(6, 6);
    for (int i = 0; i < 6; i ++) {
        for (int j = 0; j < 6; j++) {
            soecMatrix(i, j) = _vC2(i, j);
        }
    }
    Eigen::EigenSolver<Eigen::MatrixXf>  evs(soecMatrix, false);
    //Eigen::VectorXf solved_evs = evs.eigenvalues().real.cast<float>();
    _refStabPar = evs.eigenvalues().real().minCoeff();

    //Create rank 4 symm identity tensor
    //Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> I4;
    for (int i = 0; i < 3; i ++) { for (int j = 0; j < 3; j++) { for (int k = 0; k < 3; k++) { for (int l = 0; l < 3; l++) {
                    float add = 0;
                    if (i==l&&j==k) { add=add+0.5; }
                    if (i==k&&j==l) { add=add+0.5; }
                    _I4(i, j, k, l)=add;
                }}}}


    // Compute deformed constants per particle
    task()->setProgressText(tr("Getting Elastic Constants at Finite Deformation"));
    parallelFor(positions()->size(), *task(), [this](size_t index){
        computeDeformedElasticConstants(index);
    });


}
/******************************************************************************c
* Elastic constants at finite deformation *make this exportable?
* ***************************************************************************/
void CalculateElasticStabilityModifier::ElasticStabilityEngine::computeDeformedElasticConstants(size_t particleIndex)
{

    //Get def grad storage
    const Matrix3* Fm = deformationGradient()->constDataMatrix3()+particleIndex;
    //Det|f|
    float J = Fm->determinant();
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3>> Ft;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            Ft(i, j) = Fm->row(i)[j]; // I switched these to test if trasp is issue
        }
    }
    //float J = Fmatrix.determinant();

    //get strain tensor storage
    const SymmetricTensor2* nm = strainTensors()->constDataSymmetricTensor2()+particleIndex;
    //voigt form strain This is Correct
    Eigen::TensorFixedSize<float, Eigen::Sizes<6>> n;
    n(0) = nm->xx();
    n(1) = nm->yy();
    n(2) = nm->zz();
    n(3) = nm->yz()*2;
    n(4) = nm->xz()*2;
    n(5) = nm->xy()*2;


    //C3.n //CONFIRMED CORRECT!
    Eigen::array<Eigen::IndexPair<int>, 1> pd0 = { Eigen::IndexPair<int>(2,0) };
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> C3n = _vC3.contract(n, pd0);


    //Get D1 term //This term is correct :)
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> D1f;
    for (int i = 0; i < 3; i++) { for (int j = 0; j < 3; j ++) { for (int k  = 0; k < 3; k ++) { for (int l = 0; l < 3; l++) {
      D1f(i, j, k, l) = 0.5*(Ft(k, i)*Ft(l, j)+Ft(l, i)*Ft(k, j));
    }}}}
    //Voigt the D1 term
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> D1v;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            for (int k=0; k<3; k++){
                for (int l=0; l<3; l++)
                {   int w = ElasticConstants::vID(i, j);
                    int v = ElasticConstants::vID(k, l);
                    D1v(w, v)= D1f(i, j, k, l);
                }
            }
        }
    }//Voigted D1 All set :)


    //Get the D2 term
    Eigen::array<Eigen::IndexPair<long>,0> empty_index_list = {};
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> D2A1;
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> D2A2;

    //Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> D2f1;
    //Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> D2f2;

    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> Ft4 = Ft.contract(Ft, empty_index_list).eval(); //Good

    Eigen::array<Eigen::IndexPair<int>, 1> pI0 = { Eigen::IndexPair<int>(0, 1) }; //Seems wrong
    Eigen::array<Eigen::IndexPair<int>, 2> pI1 = { Eigen::IndexPair<int>(1, 1), Eigen::IndexPair<int>(3, 0) }; //Seems correct now
    //Eigen::array<Eigen::IndexPair<int>, 1> pI2 = { Eigen::IndexPair<int>(2, 0) }; //Seems wrong
    //Eigen::array<Eigen::IndexPair<int>, 2> pI3 = { Eigen::IndexPair<int>(0, 0), Eigen::IndexPair<int>(3, 1) }; //Seems correct now

    Eigen::array<Eigen::ptrdiff_t, 6> SP1 = {{0, 1, 4, 5, 2, 3}}; // Need to shuffle back to the correct positions
    //Eigen::array<Eigen::ptrdiff_t, 6> SP2 = {{4, 5, 3, 1, 0, 2}}; // Testing this again now

    D2A1 = Ft4.contract(_I4, pI0).eval(); //TensorContract[TensorProduct[Ft4, I4], {1, 6}]
    D2A1 = D2A1.contract(_I4, pI1).eval(); //THIS IS CORRECT
    D2A2.shuffle(SP1) = D2A1.eval(); //Check this, seems to be a sine issue??

    //D2A2 = Ft4.contract(_I4, pI2).eval(); //Also getting wrong answer {3, 6} 5<->1 4<->3 6<->2 instead of as above (test with SP1 as {{2. 1. 0. 4. 5. 3}}
    //D2A2.shuffle(SP1) = D2A2.eval(); //Now compares correctly to the mathematica

   // D2f1 = D2A1.contract(_I4, pI1).eval(); //THIS IS CORRECT BUT ORDERED 6, 5, 4, 3, 2, 1
   // D2f2 = D2A2.contract(_I4, pI3).eval(); //SAME AS ABOVE!! THIS SEEMS WRONG :0 :(

    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3, 3, 3>> D2f;
    //D2f = 0.5*(D2f1+D2f2).eval();
    D2f = 0.5*(D2A1+D2A2).eval();

    //Voigt the D2 term
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6, 6>> D2v;
    for (int i=0; i<3; i++) { for (int j=0; j<3; j++) { for (int k=0; k<3; k++) { for (int l=0; l<3; l++) { for (int m = 0; m < 3; m++) { for (int n = 0; n < 3; n++) {
        int w = ElasticConstants::vID(i, j);
        int v = ElasticConstants::vID(k, l);
        int y = ElasticConstants::vID(m, n);
        D2v(w, v, y)= D2f(i, j, k, l, m, n); }}}}}} //THIS NOW WORKS!


    //Calc cdef //Need to confirm
    Eigen::array<Eigen::IndexPair<int>, 1> pd1 = { Eigen::IndexPair<int>(0,1) };
    Eigen::array<Eigen::IndexPair<int>, 1> pd2 = { Eigen::IndexPair<int>(0,1) };
    Eigen::array<Eigen::IndexPair<int>, 1> pd3 = { Eigen::IndexPair<int>(0,0) };
    Eigen::array<Eigen::IndexPair<int>, 1> pd4 = { Eigen::IndexPair<int>(0,0) };


    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> voigtCorrect1;
    voigtCorrect1.setValues({{1, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1}, {2, 2, 2, 2, 2, 2}, {2, 2, 2, 2, 2, 2}, {2, 2, 2, 2, 2, 2}});
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> voigtCorrect2;
    voigtCorrect2.setValues({{1, 1, 1, 2, 2, 2}, {1, 1, 1, 2, 2, 2}, {1, 1, 1, 2, 2, 2}, {1, 1, 1, 2, 2, 2}, {1, 1, 1, 2, 2, 2}, {1, 1, 1, 2, 2, 2}});


    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> Cdef;
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> ct1 = (_vC2+C3n).eval()*voigtCorrect1*voigtCorrect2; // Correct
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> ct2 = (_vC2+0.5*C3n).eval()*voigtCorrect1; //Check !! DEFINITION ISSUE ON THESE CONVERT TO VOIGT CORRECTLY!!

    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> Cdef1;
    Cdef1 = D1v.contract(ct1, pd1).eval(); // This is now good
    Cdef1 = D1v.contract(Cdef1, pd2).eval(); // This should be good.

    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> Cdef2; // Just the second term Check index order after contractions here
    Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6, 6>> Cdef22;

    //Make some ct2.contract term
    Cdef22 = ct2.contract(D2v, pd4);
    Cdef2 = Cdef22.contract(n, pd4); // This is good now

    Cdef = (1/J)*(Cdef1+Cdef2).eval(); //This seems correct now

    //send out soecDeformed - stored as C11 C12 C13 C14 ... C22 C23 C24 ....C66
    int count = 0;
    for (int i = 0; i < 6; i++) {
        int num = 6 - i;
        for (int j = 0; j < num; j++) {
        soecDeformed()->setFloatComponent(particleIndex, count, Cdef(i, j));
        count++;
        }
    }



    //Compute stress per atom
    Eigen::TensorFixedSize<float, Eigen::Sizes<6>> P2;
    Eigen::TensorFixedSize<float, Eigen::Sizes<6>> P3;
    Eigen::TensorFixedSize<float, Eigen::Sizes<6>> Pkst;
    P2 = _vC2.contract(n, pd3); // Correct
    P3 = C3n.contract(n, pd3); //Correct
    Pkst = P2+0.5*P3; // Correct

    //get unvoigted version of C' and Pkst
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> Cdef4;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            for (int k=0; k<3; k++){
                for (int l=0; l<3; l++)
                {   int w = ElasticConstants::vID(i, j);
                    int v = ElasticConstants::vID(k, l);
                    Cdef4(i, j, k, l)= Cdef(w, v);
                }
            }
        }
    }
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3>> Pkst2;
    for (int i=0; i<3; i++){
        for (int j=0; j<3; j++){
            int w = ElasticConstants::vID(i, j);
            Pkst2(i, j) = Pkst(w);
        }
    }

    //Delta matrix
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3>> del;
    for (int i=0; i<3; i++){
        for (int j=0; j<3; j++){
            if(i==j) {del(i, j) = 1; }
            else {del(i, j) = 0; }
        }
    }
    //Del prod pkst
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> DP = Pkst2.contract(del, empty_index_list ); // Good

    //Compute symm wallace tensor (*Something wrong before this point*)
    Eigen::TensorFixedSize<float, Eigen::Sizes<3, 3, 3, 3>> B;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j ++) {
            for (int k = 0; k <3; k++) {
                for (int l = 0; l < 3; l++) {
                    B(i, j, k, l) = DP(i, l, j, k)+DP(j, l, i, k)+DP(i, k, j, l)+DP(j, k, i, l)-DP(i, j, k, l)- DP(k, l, i, j);//+Cdef4(i,j,k,l);
                }
            }
        }
    } //Good
    B = 0.5*B+Cdef4; //Good

    //Do this a prettier way later...
    Eigen::MatrixXf WMatrix = Eigen::MatrixXf::Zero(6, 6);
    //Eigen::TensorFixedSize<float, Eigen::Sizes<6, 6>> WTensor;
    for (int i=0; i<3; i++) {
        for (int j=0; j<3; j++) {
            for (int k=0; k<3; k++){
                for (int l=0; l<3; l++)
                {   int w = ElasticConstants::vID(i, j);
                    int v = ElasticConstants::vID(k, l);
                    //WTensor(w, v)= L(i, j, k, l);
                    WMatrix(w, v) = B(i, j, k, l);
                }
            }
        }
    }

    //send out symm wallace tensor - stored as B11 B12 B13 B14 ... B22 B23 B24 ....B66
    count = 0;
    for (int i = 0; i < 6; i++) {
        int num = 6 - i;
        for (int j = 0; j < num; j++) {
     //   wallaceTensor()->setFloatComponent(particleIndex, count, WTensor(i, j));
        wallaceTensor()->setFloatComponent(particleIndex, count, WMatrix(i, j));
        count++;
        }
    }

   //Get eigenvalues and stab parameter
    Eigen::EigenSolver<Eigen::MatrixXf>  wvs(WMatrix, false);
    //Eigen::VectorXf solved_evs = evs.eigenvalues().real.cast<float>();
    float curMinEig = wvs.eigenvalues().real().minCoeff();
    float stabPar = (_refStabPar-curMinEig)/_refStabPar; //This appears to be correct, next up is cleaning this up, printing results, and python bindings
    stabilityParameter()->setFloat(particleIndex, stabPar);


}

/******************************************************************************c
* Compute stress/atom *make this an exportable property?
* ***************************************************************************/
void CalculateElasticStabilityModifier::ElasticStabilityEngine::computeStress(size_t particleIndex)
{

}


/******************************************************************************c
 * Computes the Wallace tensor of a single particle
 * ***************************************************************************/
void CalculateElasticStabilityModifier::ElasticStabilityEngine::computeElasticStability(size_t particleIndex)
{
    //Compute wallace tensor
    task()->setProgressText(tr("Calculating per particle wallace tensor"));

    //eigenvalue?? //Switch from taco over to Eigen in this setup, then can calc eigenvalues, has tensor module
}

/*************************************************************************
 * Emit results
 * ************************************************************************/
 void CalculateElasticStabilityModifier::ElasticStabilityEngine::emitResults(TimePoint time, ModifierApplication *modApp, PipelineFlowState &state)
 {
     ParticlesObject* particles = state.expectMutableObject<ParticlesObject>();

     if(_inputFingerprint.hasChanged(particles))
         modApp->throwException(tr("Cached modifier results are obsolete, because the number or the storage order of input particles has changed."));

     //particles->createProperty(wallaceTensor());
     particles->createProperty(stabilityParameter());

 }

}
}
