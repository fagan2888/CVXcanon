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
#include <ecos.h>
#include "BuildMatrix.hpp"


Solution solve(Sense sense, LinOp* objective, std::vector< LinOp* > constraints,
							 std::map<std::string, double> arguments){
	/* Pre-process constraints */
	// filter by type
	// compute dimensions
	// build variable_id, constraint_id maps


	/* Instiantiate problem data (convert appropriate linOp trees to sparse matrix form) */

	/* Call ECOS and solve the problem */

	/* post-process ECOS call and build solution object */
	
	return Solution();
}