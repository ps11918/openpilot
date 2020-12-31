#include <acado_code_generation.hpp>

#define PI 3.1415926536
#define deg2rad(d) (d/180.0*PI)

const int N_steps = 16;
double T_IDXS[N_steps] = {0.00976562, 0.0390625 , 0.08789062, 0.15625,
                     0.24414062, 0.3515625 , 0.47851562, 0.625     , 0.79101562,
                     0.9765625 , 1.18164062, 1.40625   , 1.65039062, 1.9140625 ,
                     2.19726562, 2.5};


using namespace std;
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;
int main( )
{
  USING_NAMESPACE_ACADO


  DifferentialEquation f;

  DifferentialState xx; // x position
  DifferentialState yy; // y position
  DifferentialState psi; // vehicle heading
  DifferentialState dpsi;

  OnlineData v_poly_r0, v_poly_r1, v_poly_r2, v_poly_r3;
  Control ddpsi;
  auto v_poly = v_poly_r0*(xx*xx*xx) + v_poly_r1*(xx*xx) + v_poly_r2*xx + v_poly_r3;

  // Equations of motion
  f << dot(xx) == v_poly * cos(psi);
  f << dot(yy) == v_poly * sin(psi);
  f << dot(psi) == dpsi;
  f << dot(dpsi) == ddpsi;


  // Running cost
  Function h;

  // Distance errors
  h << yy;

  // Yaw rate trajectory error
  h << dpsi;
  
  // Angular rate error
  h << ddpsi;

  BMatrix Q(3,3); Q.setAll(true);
  // Q(0,0) = 1.0;
  // Q(1,1) = 1.0;
  // Q(2,2) = 1.0;

  // Terminal cost
  Function hN;

  // Distance errors
  hN << yy;

  BMatrix QN(1,1); QN.setAll(true);
  // QN(0,0) = 1.0;


  Grid times(N_steps, T_IDXS);
  OCP ocp(times);
  ocp.subjectTo(f);

  ocp.minimizeLSQ(Q, h);
  ocp.minimizeLSQEndTerm(QN, hN);

  // car can't go backward to avoid "circles"
  ocp.subjectTo( deg2rad(-90) <= psi <= deg2rad(90));
  // more than absolute max steer angle
  ocp.subjectTo( deg2rad(-50) <= ddpsi <= deg2rad(50));
  ocp.setNOD(9);

  OCPexport mpc(ocp);
  mpc.set( HESSIAN_APPROXIMATION, GAUSS_NEWTON );
  mpc.set( DISCRETIZATION_TYPE, MULTIPLE_SHOOTING );
  mpc.set( INTEGRATOR_TYPE, INT_RK4 );
  mpc.set( NUM_INTEGRATOR_STEPS, 50);
  mpc.set( MAX_NUM_QP_ITERATIONS, 500);
  mpc.set( CG_USE_VARIABLE_WEIGHTING_MATRIX, YES);

  mpc.set( SPARSE_QP_SOLUTION, CONDENSING );
  mpc.set( QP_SOLVER, QP_QPOASES );
  mpc.set( HOTSTART_QP, YES );
  mpc.set( GENERATE_TEST_FILE, NO);
  mpc.set( GENERATE_MAKE_FILE, NO );
  mpc.set( GENERATE_MATLAB_INTERFACE, NO );
  mpc.set( GENERATE_SIMULINK_INTERFACE, NO );

  if (mpc.exportCode("selfdrive/controls/lib/lateral_mpc/lib_mpc_export") != SUCCESSFUL_RETURN)
    exit( EXIT_FAILURE );

  mpc.printDimensionsQP( );

  return EXIT_SUCCESS;
}
