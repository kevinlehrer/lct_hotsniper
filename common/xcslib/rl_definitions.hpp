/*
 * The XCS Library 
 * A C++ framework to apply and develop learning classifier systems
 * Copyright (C) 2002-2009 Pier Luca Lanzi
 * 
 * Pier Luca Lanzi 
 * Dipartimento di Elettronica e Informazione
 * Politecnico di Milano
 * Piazza Leonardo da Vinci 32
 * I-20133 MILANO - ITALY
 * pierluca.lanzi@polimi.it/lanzi@elet.polimi.it
 *
 * This file is part of the XCSLIB library.
 *
 * xcslib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * xcslib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the license is available at http://www.gnu.org/licenses/gpl.html
 * 
 * If you use this code, please cite the following technical report:
 *
 * P.L. Lanzi and D. Loiacono (2009), "XCSLib: The XCS Classifier System Library", 
 * Technical Report No. 2009005, Illinois Genetic Algorithms Laboratory
 * University of Illinois at Urbana-Champaign, 117 Transportation Building
 * 104 S. Mathews Avenue Urbana, IL 61801
 * 
 * Available at http://www.illigal.uiuc.edu/pub/papers/IlliGALs/2009005.pdf
 *
 * For updates please visit: http://xcslib.sf.net 
 *                           http://www.pierlucalanzi.net
 */



//-------------------------------------------------------------------------
// Filename      : rl_definitions.hh
//
// Purpose       : mapping between the high level class names and the 
//                 specific classes used for implementing RL elements
//                 defining types, classes, and constants for all the files.
//                 
// Special Notes : 
//                 
//
// Creator       : Pier Luca Lanzi
//
// Creation Date : 2004/07/23
//
// Current Owner : Pier Luca Lanzi
//-------------------------------------------------------------------------

/*!
 * \file rl_definitions.hh
 *
 * \brief maps high level class names (e.g., t_condition) to the actual class name (e.g., xcs_bitstring_condition)
 *
 */

#ifndef __RL_DEFINITIONS__
#define __RL_DEFINITIONS__

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string.h>
#include <iomanip>

//! STL libraries
#include <algorithm>
#include <vector>

//! maximum number of char available for error messages
const unsigned long MSGSTR = 256;

//! maps the actual class used for detectors, specified with the __DETECTORS__ variable in 
//! the make file to the high level name t_sensors
class   __INPUTS__;
typedef __INPUTS__ t_state;

//! maps the actual class used for classifier actions, specified with the __ACTION__ variable in 
//! the make file to the high level name t_action
class   __ACTION__;
typedef __ACTION__ t_action;

//! maps the actual class used for the environment, specified with the __ENVIRONMENT__ variable in 
//! the make file to the high level name t_environment
class   __ENVIRONMENT__;
typedef __ENVIRONMENT__ t_environment;

#include __DET_INCLUDE__
#include __ENV_INCLUDE__
#include __ACT_INCLUDE__
	
extern t_environment*	Environment;

extern int global_core_id;
extern float global_frequency;
extern int global_delta_frequency;
#endif
