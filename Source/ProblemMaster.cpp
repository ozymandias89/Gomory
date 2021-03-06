/*
 * ProblemMaster.cpp
 *
 *  Created on: 26 gen 2016
 *      Author: riccardo
 */

#include "ProblemMaster.h"

ProblemMaster::ProblemMaster(bool verbose) {
	this->verbose = verbose;

}

ProblemMaster::~ProblemMaster() {
	// TODO Auto-generated destructor stub
}

void ProblemMaster::setupLP(CEnv env, Prob lp) {

	{	// variables
		static const char* varType = NULL;
		double obj = 0.0;
		double lb = 0.0;
		double ub = CPX_INFBOUND;

		for (int i = 0; i < N; i++) {
			obj = c[i];
			snprintf(name, NAME_SIZE, "x_%i", i);
			char* varName = (char*) (&name[0]);
			CHECKED_CPX_CALL(CPXnewcols, env, lp, 1, &obj, &lb, &ub, varType,
					&varName);

		}

	}

	// constraints

	{
		std::vector<int> idx;
		std::vector<double> coef;

		for (int i = 0; i < num_constraint; i++) {
			char sense = 'E';
			int matbeg = 0;
			double rhs = b[i];
			int nzcnt = 0;

			for (int iter = 0; iter < N; iter++) {

				if (A[i][iter] != 0) {
					idx.push_back(iter);
					coef.push_back(A[i][iter]);
					nzcnt++;
				}

			}

			CHECKED_CPX_CALL(CPXaddrows, env, lp, 0, 1, nzcnt, &rhs, &sense,
					&matbeg, &idx[0], &coef[0], 0, 0);

			idx.clear();
			coef.clear();
		}
	}

}

void ProblemMaster::print_objval(CEnv env, Prob lp) {

	double objval;
	CHECKED_CPX_CALL(CPXgetobjval, env, lp, &objval);
	std::cout << endl << "Obj val: " << objval << std::endl;
}

void ProblemMaster::set_var_P(CEnv env, Prob lp) {

	int cur_numcols = CPXgetnumcols(env, lp);

	varVals.clear();
	varVals.resize(cur_numcols);
	CHECKED_CPX_CALL(CPXgetx, env, lp, &varVals[0], 0, cur_numcols - 1);

}

void ProblemMaster::print_var_P(CEnv env, Prob lp) {

	cout << "PRIMAL VARIABLES: " << endl;
	int cur_numcols = CPXgetnumcols(env, lp);
	int surplus;
	status = CPXgetcolname(env, lp, NULL, NULL, 0, &surplus, 0,
			cur_numcols - 1);
	int cur_colnamespace = -surplus; // the space needed to save the names

	// allocate memory
	char** cur_colname = (char **) malloc(sizeof(char *) * cur_numcols);
	char* cur_colnamestore = (char *) malloc(cur_colnamespace);

	// get the names
	CPXgetcolname(env, lp, cur_colname, cur_colnamestore, cur_colnamespace,
			&surplus, 0, cur_numcols - 1);

	//  print index, name and value of each column
	for (int i = 0; i < cur_numcols; i++)
		cout << cur_colname[i] << " = " << varVals[i] << endl;

	// free
	free(cur_colname);
	free(cur_colnamestore);

}

int ProblemMaster::select_fractionar_var() {

	// Selects variable with maximal fractionary value
	int index = -1;
	double max_fractionary = 10e-6;

	for (int i = 0; i < Num_original_variables; i++) {
		const double value = varVals[i], fractionary = fabs(
				value - round(value));

		if (fractionary > max_fractionary) {
			max_fractionary = fractionary;
			index = i;
		}
	}

	return index;
}

void ProblemMaster::solve(CEnv env, Prob lp) {

	// --------------------------------------------------
	// 2. solve linear problem
	// --------------------------------------------------
	CHECKED_CPX_CALL(CPXlpopt, env, lp);

	int stat = CPXgetstat(env, lp);

	// --------------------------------------------------
	// 3. STOP CONDITION
	// --------------------------------------------------
	if (stat == CPX_STAT_UNBOUNDED || stat == CPX_STAT_INFEASIBLE) {
		cout << endl << " STOP PROBLEM MASTER IS UNBOUNDED OR INFEASIBLE " << endl;
		cout << " Iteration number: " << iter << endl;
		throw std::runtime_error("  STOP CONDITION STEP 1 ");
	}

	cout << endl << "PROBLEM MASTER:" << endl;

	// --------------------------------------------------
	// 4. print solution
	// --------------------------------------------------
	print_objval(env, lp);

	// --------------------------------------------------
	// 5. set number and value of variable
	//    (cur_numcols,varVals) and print these
	// --------------------------------------------------
	set_var_P(env, lp);
	print_var_P(env, lp);

	// --------------------------------------------------
	// 6. chose the best fractional variable
	// --------------------------------------------------
	int index = select_fractionar_var();

	// --------------------------------------------------------
	// 7. if x solution aren't integer create P1 and P2 problem
	// --------------------------------------------------------
	if (index == -1) {
		cout << " Iteration number: " << iter << endl;
		throw std::runtime_error(
				" The last solution is the best integer solution. STOP CONDITION STEP 4 ");

	}


}

void ProblemMaster::add_constraint_R(CEnv env, Prob lp, double* gamma, int var) {

	static const char* varType = NULL;
	double obj = 0.0;
	double lb = 0.0;
	double ub = CPX_INFBOUND;
	char sense = 'E';
	int matbeg = 0;
	int nzcnt = 0;

	std::vector<int> idx;
	std::vector<double> coef;

	// add Slack variables
	snprintf(name, NAME_SIZE, "S_%i", slack);
	slack++;
	char* varName = (char*) (&name[0]);
	CHECKED_CPX_CALL(CPXnewcols, env, lp, 1, &obj, &lb, &ub, varType, &varName);

	//-S
	coef.push_back(-1);
	idx.push_back(N);
	nzcnt++;

	//a_T * x
	for (int i = 0; i < var; i++) {

		if (gamma[i] != 0) {
			coef.push_back(gamma[i]);
			idx.push_back(i);
			nzcnt++;
		}
	}

	double rhs = 1;

	num_constraint++;
	N++;

	//add 0 to c
	c.push_back(0);



	//extend A matrix
	A.resize(num_constraint);
	for (int i = 0; i < num_constraint; i++)
		A[i].resize(N);

	for (int i = 0; i < N; i++)
		A[(num_constraint - 1)][i]=0;

	for (int i = 0; i < var; i++)
		A[(num_constraint - 1)][i] = gamma[i];


	A[(num_constraint - 1)][N - 1] = -1;


	CHECKED_CPX_CALL(CPXaddrows, env, lp, 0, 1, nzcnt, &rhs, &sense, &matbeg,
			&idx[0], &coef[0], 0, 0);

	b.push_back(rhs);




}

void ProblemMaster::save(CEnv env, Prob lp) {

	CHECKED_CPX_CALL(CPXwriteprob, env, lp, "../data/problem.lp", 0);

}
