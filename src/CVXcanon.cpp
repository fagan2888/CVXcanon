//    This file is part of CVXcanon.
//
//    CVXcanon is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    CVXcanon is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with CVXcanon.  If not, see <http://www.gnu.org/licenses/>.

#include "CVXcanon.hpp"
#include <map>
#include <set>
#include <ecos.h>
#include "BuildMatrix.hpp"
#include "LinOp.hpp"
#include "LinOpOperations.hpp"
#include "EcosProblem.hpp"

/**
 * Takes as list of contraints as input. Returns a map from constraint
 * type to list of linOp trees for each type.
 * 
 * Constraint Types: 	EQ, 		// equality constraint
 *										LEQ, 		// non-negative orthant
 *										SOC, 		// second-order cone
 *										EXP, 		// exponential cone *** TODO: Needs special formatting (elemwise) for ECOS! **
 *										SDP, 		// semi-definite cone **** NOT CURRENTLY SUPPORTED
 */
std::map<OperatorType, std::vector<LinOp *> > filter_constraints(std::vector<LinOp *> constraints){
	std::map<OperatorType, std::vector<LinOp *> > constr_map;
	constr_map.insert(std::pair<OperatorType, std::vector<LinOp *> >(EQ, std::vector<LinOp *>()));
	constr_map.insert(std::pair<OperatorType, std::vector<LinOp *> >(LEQ, std::vector<LinOp *>()));
	constr_map.insert(std::pair<OperatorType, std::vector<LinOp *> >(SOC, std::vector<LinOp *>()));
	constr_map.insert(std::pair<OperatorType, std::vector<LinOp *> >(EXP, std::vector<LinOp *>()));

	for (int i = 0; i < constraints.size(); i++){
		LinOp constraint = *constraints[i];
		// assuming node has exactly one argument, the root of the linOp Tree
		LinOp *tree = constraint.args[0];
		constr_map[constraint.type].push_back(tree);
	}

	return constr_map;
}

int accumulate_size(std::vector<LinOp *> constraints){
	int size = 0;
	for (int i = 0; i < constraints.size(); i++){
		LinOp constr = *constraints[i];
		size += constr.size[0] * constr.size[1];
	}
	return size;
}

std::map<OperatorType, std::vector<int> > compute_dimensions(std::map<OperatorType, std::vector<LinOp *> > constr_map){
	std::map<OperatorType, std::vector<int> > dims;
	dims.insert(std::pair<OperatorType, std::vector<int> >(EQ, std::vector<int>()));
	dims.insert(std::pair<OperatorType, std::vector<int> >(LEQ, std::vector<int>()));
	dims.insert(std::pair<OperatorType, std::vector<int> >(SOC, std::vector<int>()));
	dims.insert(std::pair<OperatorType, std::vector<int> >(EXP, std::vector<int>()));

	// equality
	int EQ_dim = accumulate_size(constr_map[EQ]);
	dims[EQ].push_back(EQ_dim);

	// positive orthant
	int LEQ_dim = accumulate_size(constr_map[LEQ]);
	dims[LEQ].push_back(LEQ_dim);

	// dims[SOC] defines the dimension of the i-th SOC 
	std::vector<LinOp *> soc_constr = constr_map[SOC];
	for (int i = 0; i < soc_constr.size(); i++){
		LinOp constr = *soc_constr[i];
		dims[SOC].push_back(constr.size[0]);
	}

	// EXP
	int num_exp_cones = accumulate_size(constr_map[EXP]);
	dims[EXP].push_back(num_exp_cones);

	return dims;
}

struct Variable {
	int id;
	std::vector<int> size;
	bool operator < (const Variable &other) const { return id < other.id; }
};

// returns a vector of all variable 
std::vector<Variable> get_expr_vars(LinOp &expr){
	std::vector<Variable> vars; 
	if(expr.type == VARIABLE){
		vars.push_back(Variable());
		vars[0].id = get_id_data(expr);
		vars[0].size = expr.size;
	} else {
		for(int i = 0; i < expr.args.size(); i++){
			std::vector<Variable> new_vars = get_expr_vars(*expr.args[i]);
			vars.insert(vars.end(), new_vars.begin(), new_vars.end());
		}
	}
	return vars;
}

int get_var_offsets(LinOp *objective, std::vector<LinOp*> constraints,
										std::map<int, int> &var_offsets){
	std::vector<Variable> vars = get_expr_vars(*objective);
	for(int i = 0; i < constraints.size(); i++){
		std::vector<Variable> constr_vars = get_expr_vars(*constraints[i]);
		vars.insert(vars.end(), constr_vars.begin(), constr_vars.end());
	}

	// sort by ID and remove duplicates to ensure variables always have same order
	std::set<Variable> s(vars.begin(), vars.end());

	// TODO: Is there a cleaner way to this this (can't get vars.assign(s.begin(), s.end()) to compile)
	vars.clear();
	std::set<Variable>::iterator it;
	for (it = s.begin(); it != s.end(); it++) {
      vars.push_back(*it);
  }

	std::sort(vars.begin(), vars.end());

	int vert_offset = 0; // number of variables in the problem
	for(int i = 0; i < vars.size(); i++){
		Variable var = vars[i];
		var_offsets[var.id] = vert_offset;
		vert_offset += var.size[0] * var.size[1];
	}

	return vert_offset;
}

Solution solve(Sense sense, LinOp* objective, std::vector<LinOp *> constraints,
							 std::map<std::string, double> solver_options){
	/* Pre-process constraints */
	std::map<OperatorType, std::vector<LinOp *> > constr_map = filter_constraints(constraints);
	std::map<OperatorType, std::vector<int> > dims_map = compute_dimensions(constr_map);

	std::map<int, int> var_offsets;
	int num_variables = get_var_offsets(objective, constraints, var_offsets);

	/* Instantiate problem data */
	EcosProblem problem = EcosProblem(sense, objective, constr_map,
																		dims_map, var_offsets, num_variables);
	
	Solution solution = problem.solve(solver_options);

	/* Temporary for debugging */
	std::cout << "SOLVER STATUS: " << solution.status << std::endl;
	std::cout << "OPTIMAL VALUE: " << solution.optimal_value << std::endl;

	return solution;
}