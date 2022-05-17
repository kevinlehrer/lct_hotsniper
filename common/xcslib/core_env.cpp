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
// Filename      : core_env.cpp
//
// Purpose       : implementation of the multiplexer class 
//                 
// Special Notes : 
//                 
// Creator       : Pier Luca Lanzi
//
// Creation Date : 2002/05/31
//
// Current Owner : Pier Luca Lanzi
//
//-------------------------------------------------------------------------

/*!
 * \file core_env.cpp
 *
 * \brief implements the Boolean multiplexer
 *
 * \author Pier Luca Lanzi
 *
 * \version 0.01
 *
 * \date 2002/05/14
 *
 */

#include <cmath>
#include "xcs_utility.hpp"
#include "xcs_random.hpp"

#include "core_env.hpp"

using namespace std;

bool	core_env::init=false;	//!< set the init flag to false so that the use of the config manager becomes mandatory
//#define __DEBUG__
double			core_env::min_input;
double			core_env::max_input;
unsigned long	core_env::no_inputs;

core_env::core_env(xcs_config_mgr2& xcs_config)
{
	if (!core_env::init)
	{
		// check if configuration file exists
		if (!xcs_config.exist(tag_name()))
		{
			xcs_utility::error(class_name(), "constructor", "section <" + tag_name() + "> not found", 1);	
		}
		
		// read configurations for environment
		try {
			//! input range
			min_input = xcs_config.Value(tag_name(), "min input", 0.0);
			max_input = xcs_config.Value(tag_name(), "max input", 1.0);
			no_inputs = xcs_config.Value(tag_name(), "input size", 1);
			cout << "min_input = " << min_input << " / max_input = " << max_input << " / input_size = " << no_inputs << endl;
		} catch (const char *attribute) {
			string msg = "attribute \'" + string(attribute) + "\' not found in <" + tag_name() + ">";
		}

		// create initial input values
		current_inputs.clear();
		for(int i=0;i<no_inputs;i++)
		{
			current_inputs.push_back(min_input);
		}

	}

	core_env::init = true;
}

core_env::core_env(xcs_config_mgr2& xcs_config, const PerformanceCounters* counters)
{
	new (this) core_env(xcs_config);
	measurements = counters;

	//for(int i=0; i<n; i++)
	cout << "I am a stupid test " << endl;

	// DEBUG: print initial performance counters to check if access works
	# if 1
	int coreCounter = 0;
	/* get performance counters for current core    */
	float power = measurements->getPowerOfCore(coreCounter);     // not needed
	float temperature = measurements->getTemperatureOfCore(
		coreCounter);
	//int frequency = oldFrequencies.at(coreCounter);
	float utilization = measurements->getUtilizationOfCore(
		coreCounter);
	float ips = measurements->getIPSOfCore(coreCounter);

	cout << "[Scheduler][xcs]: Core " << setw(2) << coreCounter
			<< ":";
	cout << " P=" << fixed << setprecision(3) << power << " W";
	//cout << " f=" << frequency << " MHz";
	cout << " T=" << fixed << setprecision(1) << temperature << " C";
	cout << " IPS=" << fixed << setprecision(3) << ips;
	// avoid the little circle symbol, it is not ASCII
	cout << " utilization=" << fixed << setprecision(3) << utilization
			<< endl;
	# endif
}

/*!
 * \fn void core_env::begin_problem(const bool explore)
 * \param explore true if the problem is solved in exploration
 *
 * \brief generates a new input configuration for the Boolean multiplexer
 */
void	
core_env::begin_problem(const bool explore)
{
	current_reward = 0;

	update_inputs();

	// DEBUG: print initial performance counters to check if access works
	# if 0
	int coreCounter = 0;
	/* get performance counters for current core    */
	float power = measurements->getPowerOfCore(coreCounter);     // not needed
	float temperature = measurements->getTemperatureOfCore(
		coreCounter);
	//int frequency = oldFrequencies.at(coreCounter);
	float utilization = measurements->getUtilizationOfCore(
		coreCounter);
	float ips = measurements->getIPSOfCore(coreCounter);

	cout << "[Scheduler][xcs][Environment]: Core " << setw(2) << coreCounter
			<< ":";
	cout << " P=" << fixed << setprecision(3) << power << " W";
	//cout << " f=" << frequency << " MHz";
	cout << " T=" << fixed << setprecision(1) << temperature << " C";
	cout << " IPS=" << fixed << setprecision(3) << ips;
	// avoid the little circle symbol, it is not ASCII
	cout << " utilization=" << fixed << setprecision(3) << utilization
			<< endl;
	# endif
}

bool	
core_env::stop()
const
{
	return(true); 
}


void	
core_env::perform(const t_action& action)
{
	// TODO: perform action via HotSniper funcitons
	// Actions: increase frequency, decrease frequency, keep frequency constant
	// Reward handling???

	#if 0
	unsigned long		address;
	unsigned long		index;		//! index of the output bit
	string			str_inputs;

	str_inputs = inputs.string_value();

	index = xcs_utility::binary2long(str_inputs.substr(0,address_size));
	address = address_size + index;

	if (!flag_layered_reward)
	{
		if ((str_inputs[address]-'0 
		')==action.value())
		{
			current_reward = 1000;
			solved = true;
		} else {
			current_reward = 0;
			solved = false;
		}
	} else {
		if ((str_inputs[address]-'0')==action.value())
		{
			current_reward = 300 + index*200 + double(100*(unsigned long)(str_inputs[address]-'0'));
			solved = true;
		} else {
			current_reward = index*200 + double(100*(unsigned long)(str_inputs[address]-'0'));
			solved = false;
		}
	}
	#endif
}

//! only the current reward is traced
void
core_env::trace(ostream& output) const
{
	int old_precision = output.precision();
	output.setf(ios::scientific);
	output.precision(5);
	output << current_reward;
	output.unsetf(ios::scientific);
	output.precision(old_precision);
}

void 
core_env::reset_input()
{
	current_state = 0;
	inputs.set_string_value(xcs_utility::long2binary(current_state,state_size));
}

bool 
core_env::next_input()
{
	return true;
}

void
core_env::save_state(ostream& output) const
{

}

void
core_env::restore_state(istream& input)
{

}

core_env::core_env()
{
	if (!core_env::init)
	{
		xcs_utility::error(class_name(),"class constructor", "not inited", 1);
	} else {
		// nothing to init
	}
}

#if 0
t_state
core_env::state()
{
	current_inputs[0] = measurements->getFreqOfCore(global_core_id);
	current_inputs[1] = measurements->getUtilizationOfCore(global_core_id);
	current_inputs[2] = measurements->getIPSOfCore(global_core_id);

	real_inputs tmp(no_inputs);

	for(int i=0;i<no_inputs;i++)
		tmp.set_input(i,current_inputs[i]);

	inputs = tmp;

	return inputs;
}
#endif

void
core_env::update_inputs()
{
	//current_inputs[0] = measurements->getFreqOfCore(global_core_id);
	current_inputs[0] = global_frequency;
	current_inputs[1] = measurements->getUtilizationOfCore(global_core_id);
	current_inputs[2] = measurements->getIPSOfCore(global_core_id);

	real_inputs tmp(no_inputs);

	for(int i=0;i<no_inputs;i++)
		tmp.set_input(i,current_inputs[i]);

	inputs = tmp;

	# if 1
	int coreCounter = global_core_id;
	/* get performance counters for current core    */
	float power = measurements->getPowerOfCore(coreCounter);     // not needed
	float temperature = measurements->getTemperatureOfCore(
		coreCounter);
	//int frequency = oldFrequencies.at(coreCounter);
	float utilization = measurements->getUtilizationOfCore(
		coreCounter);
	float ips = measurements->getIPSOfCore(coreCounter);
	int frequency = global_frequency;

	cout << "[Scheduler][xcs][Environment]: Core " << setw(2) << coreCounter
			<< ":";
	cout << " P=" << fixed << setprecision(3) << power << " W";
	cout << " f=" << frequency << " MHz";
	cout << " T=" << fixed << setprecision(1) << temperature << " C";
	cout << " IPS=" << fixed << setprecision(3) << ips;
	// avoid the little circle symbol, it is not ASCII
	cout << " utilization=" << fixed << setprecision(3) << utilization
			<< endl;
	# endif
}
