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
				<< "./main (string)'path_name_file' (bool)debug" << endl
				<< "or (to generate random matrix)" << endl
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

			master->solve(env, lp);

			printf("\nGMI cuts: ");
			master->save(env, lp);

			status = 0;
			int jj;
			double f0, fj, coeff;
			int r, j;
			double a[Num_original_variables][N];
			double Aprime[Num_original_variables][num_constraint];
			double gamma[Num_original_variables][N];

			int var = N;
			int con = num_constraint;

			for (jj = 0; jj < Num_original_variables; jj++) {

				double fraction = fabs(
						(master->varVals[jj]) - (round(master->varVals[jj])));

				if (fraction < 10e-6)
					continue;

				//basic integer non feasible variables
				//determine the row of the current GMI cut
				status = CPXbinvacol(env, lp, jj, Aprime[jj]);

				//find the row of basic jj

				for (r = 0; r < num_constraint; r++) {
					if (fabs(Aprime[jj][r] - 1.0) < 1e-6L) {
						break;
					}
				}
//
				printf(": row %d ", r);
				f0 = master->varVals[jj] - floor(master->varVals[jj]);
				status = CPXbinvarow(env, lp, r, a[jj]);

				for (j = 0; j < N; j++) {

					if (j >= Num_original_variables) {
						if (a[jj][j] >= 0.0) {
							coeff = a[jj][j] / f0;
						} else {
							coeff = -a[jj][j] / (1 - f0);
						}
					} else {
						fj = a[jj][j] - floor(a[jj][j]);
						if (fj <= f0) {
							coeff = fj / f0;
						} else {
							coeff = (1 - fj) / (1 - f0);
						}
					}

					gamma[jj][j] = coeff;
				}

			}

			printf("\n ******* ADDING CUTS (GMI): *******\n");

			for (jj = 0; jj < Num_original_variables; jj++) {

				double fraction = fabs(
						(master->varVals[jj]) - (round(master->varVals[jj])));

				if (fraction < 10e-6)
					continue;

				if (verbose) {
					cout << endl;
					cout << "Aprime: ";
					for (int i = 0; i < con; i++)
						cout << " " << Aprime[jj][i];
					cout << endl;

					cout << "a: ";
					for (int i = 0; i < var; i++) {
						cout << " " << a[jj][i];
					}
					cout << endl;

					cout << "gamma: ";
					for (int i = 0; i < var; i++) {
						cout << " " << gamma[jj][i];
					}
					cout << endl;
				}

				master->add_constraint_R(env, lp, gamma[jj], var);

			}

			master->save(env, lp);

			cout << endl << endl << endl << endl << endl;

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
