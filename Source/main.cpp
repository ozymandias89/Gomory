/*
 @file    main.cc
 @author  Riccardo Zanella, riccardozanella89@gmail.com
 @version 2.0
 */

/* --*- C++ -*------x-----------------------------------------------------------
 *
 *
 * Description: This main resolve a linear relaxation of the problem in the form
 * 				min cT x
 * 				s.t Ax=b
 * 				x€R+
 *
 * 				then print the higher fractional variable and create a branch
 * 				with P1 and P2 new problem to resolve.
 * 				In the end create problem step 7.
 *
 * -----------------x-----------------------------------------------------------
 */

#include "generator.h"
#include <sstream>
#include <time.h>
#include "ProblemMaster.cpp"

int main(int argc, char const *argv[]) {

	// --------------------------------------------------
	// 1. Parameters (generate problem)
	// --------------------------------------------------
	string file;
	bool verbose;

	if (argc == 3) {
		file = argv[1];
		std::stringstream ss(argv[2]);
		if (!(ss >> std::boolalpha >> verbose)) {
			cerr << "Parameters error!!!!!" << endl;
			exit(1);
		}
	} else if (argc == 4) {
		int num_rows = strtol(argv[1], NULL, 10);
		int limit = strtol(argv[2], NULL, 10);
		file = generate_canonical_matrix(num_rows, limit);
		std::stringstream ss(argv[3]);
		if (!(ss >> std::boolalpha >> verbose)) {
			cerr << "Parameters error!!!!!" << endl;
			exit(1);
		}
	} else {
		cerr << "Parameters error!!!!!" << endl;
		cout << "The correct syntax is:" << endl
				<< "./main (string)'path_name_file' (bool)debug"
				<< endl << "or (to generate random matrix)" << endl
				<< "./main (int)number_constraints (int)limit_number_coefficients generate (bool)debug"
				<< endl;
		exit(1);
	}

	ifstream myfile(file.c_str(), std::ios::in);
	load_problem(myfile);

	myfile.close();

	iter = 0;

	//clock start
	clock_t t1;
	t1 = clock();

	// --------------------------------------------------
	// 2. Initialization problem
	// --------------------------------------------------

	DECL_ENV(env);
	DECL_PROB(env, lp, "resolve problem RL");

	try {

		ProblemMaster* master = new ProblemMaster(verbose);
		master->setupLP(env, lp);

		// tollerance cplex on problem master
		CPXsetdblparam(env, CPXPARAM_Simplex_Tolerances_Feasibility, 1e-5);
		CPXsetdblparam(env, CPXPARAM_Simplex_Tolerances_Optimality, 1e-5);

		do {

			clock_t t2;
			t2 = clock();
			double elapsed_secs = double(t2 - t1) / CLOCKS_PER_SEC;

			if (elapsed_secs > 120.0) {
				throw std::runtime_error("Timeout!");
			}

			int index=master->solve(env, lp);

			master->save(env, lp);

			exit(0);
//			master->add_constraint_R(env, lp);
//			master->save(env, lp);




//			int add_GMI_cuts ( void ) {
//				printf("\nGMI cuts: ");
//				status = 0;
//				//coefficenti della x
//				double a[cutinfo.numcols];
//				int jj;
//				double f0, fj, coeff;
//				int r,j;
//
//				//ciclo su tutte le varibili
//				for ( jj = 0; jj < cutinfo.numcols; jj++ ) {
//					//se la viariabile è continua o intera non faccio nulla e passo alla successiva
//					//per capire se una variabile è di base o meno il test sarebbe
//					if ( cutinfo.type[jj] == CPX_CONTINUOUS ) continue;
//					if ( cutinfo.feas[jj] == CPX_INTEGER_FEASIBLE ) continue; //note: non basic integer vars are int_feasible
//																				  //printf("GMI for var %s",cutinfo.colname[jj]);Getchar();
//					//basic integer non feasible variables
//					//determine the row of the current GMI cut
//					status = CPXbinvacol(env, nodelp, jj, cutinfo.Aprime[jj]);
//					//find the row of basic jj
//					for ( r = 0; r < cutinfo.numrows; r++ ) {
//						if ( fabs(cutinfo.Aprime[jj][r] - 1.0) < L_EPS_PREC ) {
//							break;
//						}
//					}
//					printf(": row %d (%s) ",r,cutinfo.colname[jj]);
//					f0 = cutinfo.x[jj] - floor(cutinfo.x[jj]);
//					status = CPXbinvarow(env, nodelp, r, a);
//					for ( j = 0; j < cutinfo.numcols; j++ ) {
//						//if ( cutinfo.basic[j] ) {
//						//  coeff = 0.0;
//						//} else {
//						if ( cutinfo.type[j] == CPX_CONTINUOUS ) {
//							if ( a[j] >= 0.0 ) {
//								coeff = a[j] / f0;
//							} else {
//								coeff = - a[j] / (1-f0);
//							}
//						} else { //integer variable
//							//printf("a: %lf; a_: %lf; f: %lf;\n",a[j],floor(a[j]),a[j] - floor(a[j]));fflush(stdout);
//							fj = a[j] - floor(a[j]);
//							if ( fj <= f0 ) {
//								coeff = fj / f0;
//							} else {
//								coeff = (1-fj) / (1-f0);
//							}
//						}
//						//}
//						cutinfo.ind[j] = j;
//						cutinfo.coeff[j] = coeff;
//					}
//
//					printf("\n ******* ADDING CUT %d (GMI): *******\n",++numCutsAdded);
//					for (j=0; j<cutinfo.numcols; j++) {
//						printf("%9s",cutinfo.colname[cutinfo.ind[j]]);
//					}
//					printf("\n");
//					for (j=0; j<cutinfo.numcols; j++) {
//						printf("%9.4lf",cutinfo.coeff[j]);
//					}
//					printf("\n");
//					// add cut for jj's row
//					/*if (  false
//					 //||  !strcmp(cutinfo.colname[jj],"x00")
//					 //||  !strcmp(cutinfo.colname[jj],"x01")
//					 ||  !strcmp(cutinfo.colname[jj],"x02")
//					 ||  !strcmp(cutinfo.colname[jj],"x37")
//					 )  //luino*/
//					status = CPXcutcallbackadd (env, gbl_cbdata, gbl_wherefrom, cutinfo.numcols, 1 - L_EPS_TOL, 'G',
//							cutinfo.ind, cutinfo.coeff, 0);
//					if ( status ) {
//						fprintf (stderr, "Failed to add cut.\n");
//						status = L_ERROR_SEVERE;
//						goto TERMINATE;
//					}
//				}
//
//				TERMINATE:
//				return status;
//
//			}















			iter++;
		} while (1);

	} catch (std::exception& e) {
		cout << "Number of constraints added: "
				<< CPXgetnumrows(env, lp) - Num_original_constraints << endl;
		std::cout << ">>>EXCEPTION: " << e.what() << std::endl;
		cout << "Number of constraints added: "
				<< CPXgetnumrows(env, lp) - Num_original_constraints << endl;
		cout << "Number of iteration: " << iter << endl;
		CHECKED_CPX_CALL(CPXwriteprob, env, lp, "../data/problem.lp", 0);

		CPXfreeprob(env, &lp);
		CPXcloseCPLEX(&env);

		clock_t t2;
		t2 = clock();
		double elapsed_secs = double(t2 - t1) / CLOCKS_PER_SEC;
		cout << "Elapsed time: " << elapsed_secs << endl;

	}

	return 0;
}
