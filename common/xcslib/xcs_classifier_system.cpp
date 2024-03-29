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
// Filename      : xcs_classifier_system.cc
//
// Purpose       : definition of the class that implements XCS 
//                 
// Special Notes : 
//                 
// Creator       : Pier Luca Lanzi
//
// Creation Date : 2002/05/28
//
//-------------------------------------------------------------------------
// Updates
//         2006 12 15 added specificity, average_gradient, and "update during test" flag.
//         2006 10 31 added the gradient descent
//         2006 02 01 modified the configuration reading
//         2005 07 06 corrected a small bug in the code added on the May 04th 
//                    for the new deletion
//         2005 05 04 new deletion added: random+fitness score (ala Tim)
//         2005 04 29 init_population_random changed back to the
//                    usual "slow" random init.
//         2005 04 29 changed the random init of [P] due to a bug dependent
//                    on the version of STL
//         2004 07 29 corrected a bug in the AS sub
//         2003 11 21 init of offspring is complaint with 
//                    algorithmic description
//         2003 11 20 fitness is updated without MAM
//         2003 11 20 added the random deletion and changed 
//                    the code to allow the addition of other
//                    deletion strategies.
//         2003 11 20 modified the tournament selection process
//                    according to Martin's code.
//         2003 08 20 added sort after population is loaded
//         2003 08 16 added tournament_size parameter reading from 
//                    configuration file
//         2003 08 08 added population.clear in the
//                    init_population_load function
//         2003 08 06 removed the id setting from the classifier
//                    constructor. Added two methods to explain
//                    fields produced/read by stream operators.
//-------------------------------------------------------------------------

/*! \file	xcs_classifier_system.cc
 *  \brief	Implementation of the XCS classifier system as described in Butz & Wilson paper.
 *
 */

#include <string>
#include <fstream>
#include <algorithm>
#include "xcs_classifier_system.hpp"

#include "rl_definitions.hpp"

using namespace std;

//#define __DEBUG__ 1

//! reset all the collected statistics
void
xcs_statistics::reset()
{
	average_prediction = 0;
	average_fitness = 0;
	average_error = 0;
	average_actionset_size = 0;
	average_experience = 0;
	average_numerosity = 0;
	average_time_stamp = 0;
	average_no_updates = 0;	
	system_error = 0;

	no_macroclassifiers = 0;
	no_ga = 0;
	no_cover = 0;
	no_subsumption = 0;
}

//! class constructor; it invokes the reset method \sa reset
xcs_statistics::xcs_statistics()
{
	reset();
}

ostream& 
operator<<(ostream& output, const xcs_statistics& stats)
{
	output << stats.average_prediction << "\t";
	output << stats.average_fitness << "\t";
	output << stats.average_error << "\t";
	output << stats.average_actionset_size << "\t";
	output << stats.average_experience << "\t";
	output << stats.average_numerosity << "\t";
	output << stats.average_time_stamp << "\t";
	output << stats.average_no_updates << "\t";
	output << stats.system_error << "\t";

	output << stats.no_macroclassifiers << "\t";
	output << stats.no_ga << "\t";
	output << stats.no_cover << "\t";
	output << stats.no_subsumption << "\t";
	return (output);
}

istream& 
operator>>(istream& input, xcs_statistics& stats)
{
	input >> stats.average_prediction;
	input >> stats.average_fitness;
	input >> stats.average_error;
	input >> stats.average_actionset_size;
	input >> stats.average_experience;
	input >> stats.average_numerosity;
	input >> stats.average_time_stamp;
	input >> stats.average_no_updates;
	input >> stats.system_error;

	input >> stats.no_macroclassifiers;
	input >> stats.no_ga;
	input >> stats.no_cover;
	input >> stats.no_subsumption;
	return (input);
}

xcs_classifier_system::xcs_classifier_system(xcs_config_mgr2& xcs_config)
{
	delta_del = .1;
	use_exponential_fitness = false;			//! deprecated

	string		str_covering_setting;
	string		str_ga_sub;				//! string to read the setting for GA subsumption
	string		str_gaa_sub;				//! string for reading Butz subsubmption setting
	string		str_as_sub;				//! string to read the setting for AS subsumption
	string		str_pop_init;				//! string to read the population init strategy
	string		str_exploration;			//! string to set exploration strategy
	string		DeleteWithAccuracyString;		//! string to set deletion strategy
	double		covering_threshold;			//! generic threshold for setting the covering strategy
	string		str_covering;				//! char array to read the covering strategy
	string 		str_error_first;			//! char array to read the update order for prediction error
	string		str_use_mam;				//! string for reading MAM settings
	string		str_ga_ts;				//! string for reading GA tournament selection settings
	string		str_discovery_component;		//! string for reading discovery component flag
	string		str_use_gd;				//! string for reading gradient descent flag
	string		str_update_test;			//! string for reading the update setting

	//! look for the init section in the configuration file
	if (!xcs_config.exist(tag_name()))
	{
		xcs_utility::error(class_name(), "constructor", "section <" + tag_name() + "> not found", 1);	
	}

	try {
		max_population_size = xcs_config.Value(tag_name(), "population size");
		learning_rate = xcs_config.Value(tag_name(), "learning rate");
		discount_factor = xcs_config.Value(tag_name(), "discount factor");
		str_covering_setting = (string) xcs_config.Value(tag_name(), "covering strategy");
		str_covering = str_covering_setting.substr(0, str_covering_setting.find(" "));
		//cout <<"STR <" << str_covering << ">" << endl;
		covering_threshold = atol(xcs_utility::trim(str_covering_setting.substr(0, str_covering_setting.find(" "))).c_str());
		//cout <<"STR " << covering_threshold << endl;
		str_discovery_component = (string) xcs_config.Value(tag_name(), "discovery component");
		theta_ga = xcs_config.Value(tag_name(), "theta GA");
		prob_crossover = xcs_config.Value(tag_name(), "crossover probability");
		prob_mutation = xcs_config.Value(tag_name(), "mutation probability");
		epsilon_zero = xcs_config.Value(tag_name(), "epsilon zero");
		vi = xcs_config.Value(tag_name(), "vi");
		alpha = xcs_config.Value(tag_name(), "alpha");
		init_prediction = xcs_config.Value(tag_name(), "prediction init");
		init_error = xcs_config.Value(tag_name(), "error init");
		init_fitness = xcs_config.Value(tag_name(), "fitness init");
		init_set_size = xcs_config.Value(tag_name(), "set size init");
		str_pop_init = (string) xcs_config.Value(tag_name(), "population init");

		str_exploration = (string) xcs_config.Value(tag_name(), "exploration strategy");
		DeleteWithAccuracyString = (string) xcs_config.Value(tag_name(), "deletion strategy");
		theta_del = xcs_config.Value(tag_name(), "theta delete");
		theta_sub = xcs_config.Value(tag_name(), "theta GA sub");
		theta_as_sub = xcs_config.Value(tag_name(), "theta AS sub");
		str_ga_sub = (string) xcs_config.Value(tag_name(), "GA subsumption");
		str_gaa_sub = (string) xcs_config.Value(tag_name(), "GAA subsumption", "off");
		str_as_sub = (string) xcs_config.Value(tag_name(), "AS subsumption");
		str_error_first = (string) xcs_config.Value(tag_name(), "update error first");
		str_use_mam = (string) xcs_config.Value(tag_name(), "use MAM");
		str_ga_ts = (string) xcs_config.Value(tag_name(), "GA tournament selection");
		tournament_size = xcs_config.Value(tag_name(), "tournament size");

		str_use_gd = (string) xcs_config.Value(tag_name(), "gradient descent", "off");
		str_update_test = (string) xcs_config.Value(tag_name(), "update during test", "on");
 
	} catch (const char *attribute) {
		string msg = "attribute \'" + string(attribute) + "\' not found in <" + tag_name() + ">";
		xcs_utility::error(class_name(), "constructor", msg, 1);
	}

	//!
	set_covering_strategy(string(str_covering),covering_threshold);
	set_exploration_strategy(str_exploration.c_str());
	set_deletion_strategy(DeleteWithAccuracyString.c_str());
	set_init_strategy(string(str_pop_init));

	//! reserve memory for [P], [M], [A], [A]-1
	match_set.reserve(max_population_size);
	action_set.reserve(max_population_size);
	previous_action_set.reserve(max_population_size);
	select.reserve(max_population_size);

	//! set subsuption methods
	xcs_utility::set_flag(string(str_ga_sub),flag_ga_subsumption);
	xcs_utility::set_flag(string(str_gaa_sub),flag_gaa_subsumption);
	xcs_utility::set_flag(string(str_as_sub),flag_as_subsumption);
	xcs_utility::set_flag(string(str_update_test), flag_update_test);
	//! create the prediction array
	create_prediction_array();
	
	//! check subsumption settings
	t_condition	cond;

	if (flag_as_subsumption && !cond.allow_as_subsumption())
	{
		xcs_utility::error( class_name(), "constructor", "AS subsumption requested but condition class does not allow", 1);
	}

	if (flag_ga_subsumption && !cond.allow_ga_subsumption())
	{
		xcs_utility::error( class_name(), "constructor", "GA subsumption requested but condition class does not allow", 1);
	}

	flag_cover_average_init = false;
	flag_ga_average_init = false;
	xcs_utility::set_flag(string(str_error_first), flag_error_update_first);
	xcs_utility::set_flag(string(str_use_mam), flag_use_mam);
	
	//! print the system configuration
	//print_options(clog);

	//! set tournament selection
	xcs_utility::set_flag(string(str_ga_ts), flag_ga_tournament_selection);
	xcs_utility::set_flag(string(str_discovery_component), flag_discovery_component);

	//! set gradient descent
	xcs_utility::set_flag(string(str_use_gd), flag_use_gradient_descent);
}

void	
xcs_classifier_system::set_exploration_strategy(const char* explorationType)
{
	if (!strcmp(explorationType, "PROPORTIONAL"))
	{	
		//!	action selection proportional to prediction array value
		action_selection_strategy = ACTSEL_PROPORTIONAL;
	} else if (!strcmp(explorationType, "RANDOM"))
	{	//! 	random exploration
		action_selection_strategy = ACTSEL_SEMIUNIFORM;
		prob_random_action = 1.0;
	} else if (!strncmp(explorationType, "SEMIUNIFORM",11))
	{
		action_selection_strategy = ACTSEL_SEMIUNIFORM;	
		prob_random_action = atof(explorationType+12);
		if ((prob_random_action<=0.) || (prob_random_action>1.))
		{	
			//! probability out of range
			char errMsg[MSGSTR] = "";
			sprintf(errMsg,
			"'Biased' parameter (%f) out of range (0.0,1.0]",
				prob_random_action);
			xcs_utility::error(class_name(),"SetExploration",string(errMsg),1);
		}
	}
	else
	{	//! unknown exploration strategy 
		char errMsg[MSGSTR] = "";
		sprintf(errMsg, "Unrecognized Exploration Policy '%s'", explorationType);
		xcs_utility::error(class_name(),"SetExploration",string(errMsg),1);
	}
}


void
xcs_classifier_system::set_deletion_strategy(const char* deleteType)
{
	if (!strcmp(deleteType, "STANDARD"))
	{
		flag_delete_with_accuracy = false;
 		delete_strategy = XCS_DELETE_RWS_SETBASED;	//! delete with RWS according to actionset size

	} else if (!strcmp(deleteType, "ACCURACY-BASED"))
	{
		flag_delete_with_accuracy = true;
 		delete_strategy = XCS_DELETE_RWS_FITNESS;	//! delete with RWS according to actionset size and fitness
	} else if (!strcmp(deleteType, "RANDOM-WITH-ACCURACY"))
	{
		delete_strategy = XCS_DELETE_RANDOM;		//! random delete with accuracy score
		flag_delete_with_accuracy = true;
	} else if (!strcmp(deleteType, "RANDOM"))
	{
		delete_strategy = XCS_DELETE_RANDOM;		//! random delete
	} else {	
		//! unrecognized deletion strategy
		xcs_utility::error(class_name(),"set_deletion_strategy", "unrecognized deletion strategy '" + string(deleteType) + "'", 1);
	}
}

void	
xcs_classifier_system::print_options(ostream& output) const
{
	output << "\nXCS OPTIONS\n" << endl;

	if (flag_ga_subsumption)
	{
		output << "\tGA subsumption:\t\tyes ";
		output << "\ttheta_ga       \t\t";
		output << theta_sub << "\n";
	}
	else
	{
		output << "\tGA subsumption:\t\tno ";
	}
	output << endl;

	if (flag_as_subsumption)
	{
		output << "\tAS subsumption:\t\tyes ";
	} else {
		output << "\tAS subsumption:\t\tno ";
	}
	output << endl;

	output << "\tpopulation initialization:\t";
	switch (population_init)
	{
		case INIT_EMPTY:
			output << "\t[P] is initially empty\n";
			break;
		case INIT_RANDOM:
			output << "\t[P] is initially random\n";
			break;
	}

	output << "\texploration strategy\t";
	output << endl;
	switch (action_selection_strategy)
	{
		case ACTSEL_SEMIUNIFORM:
			output << " biased ";
			output << "with probability " << prob_random_action << endl;
			break;
		case ACTSEL_UNIFORM:
			output << " uniform\n";
			break;
		case ACTSEL_PROPORTIONAL:
			output << " proportional\n";
			break;
	}

	output << "\tdeletion strategy\t";
	output << endl;
	output << (flag_delete_with_accuracy ? "\taccuracy-based\n" : "standard\n");
	output << endl;

	output << "\terror update:\t";
	output << (flag_error_update_first)?"error is updated first":"prediction is updated first";
	output << endl;
}

void
xcs_classifier_system::init_classifier_set()
{
	t_classifier classifier;
	switch (population_init)
	{
		//! [P] = {}
		case INIT_EMPTY:
			erase_population();
			break;

		//! fill [P] with random classifiers
		case INIT_RANDOM:
			init_population_random();
			break;

		//! fill [P] with classifiers save in a file
		case INIT_LOAD:
			init_population_load(population_init_file);
			break;

		default:
			xcs_utility::error(class_name(),"init_classifier_set", "init strategy unknown" , 1);
	}
};
				
				
bool	compare_cl(t_classifier *clp1, t_classifier *clp2) {return *clp1<*clp2;};
void
xcs_classifier_system::insert_classifier(t_classifier& new_cl)
{
	///CHECK
	assert(new_cl.actionset_size>=0);
	assert(new_cl.numerosity==1);

	//new_cl.condition.check_limits();
	///END CHECK

	/// keep a sorted index of classifiers
	t_classifier *clp = new t_classifier;
	*clp = new_cl;

	clp->time_stamp = total_steps;
	clp->experience = 0;

	t_set_iterator	pp;

	pp = lower_bound(population.begin(),population.end(),clp,compare_cl);
	if ((pp!=population.end()))
	{
		if ( (**pp)!=(*clp) )
		{
			clp->generate_id();
			population.insert(pp,clp);
			macro_size++;
		}
		else {
			(**pp).numerosity++;
			delete clp;
		}
	} else {
		population.insert(pp,clp);
		macro_size++;
	}
	population_size++;
}

//! build [M]
unsigned long	
xcs_classifier_system::match(const t_state& detectors)
{
	t_set_iterator			pp;		/// iterator for visiting [P]
	unsigned long			sz = 0;		/// number of micro classifiers in [M]

	match_set.clear();				/// [M] = {}
	//cout << "population = " << population << endl;
	//cout << "population.begin() : " << population.begin() << endl;
	//cout << "popluation.end()   : " << population.end() << endl;
	int count_population = 0;
	for(pp=population.begin();pp!=population.end();pp++)
	{
		//cout << "iterator = " << pp << endl;
 		//cout << "classifier = " << **pp << endl;
 		//cout << "input      = " << detectors << endl;
		#if 1
		if ((**pp).match(detectors))
		{
			match_set.push_back(*pp);
			sz += (**pp).numerosity;
			//cout << "[xcs]: match " << sz << endl;
       		}
		#endif
		count_population++;
   	}
	//cout << "[xcs]: population size = " << count_population << endl;
	//cout << "[XCS]: sz = " << sz << endl;
	
	return sz;
}

//! perform covering on [M], only if needed
bool
xcs_classifier_system::perform_covering(t_classifier_set &match_set, const t_state& detectors)
{
	//cout << "[xcs]: covering (2) " << endl;
	switch (covering_strategy)
	{
		//! perform covering according to Wilson 1995
		case COVERING_STANDARD:
			return perform_standard_covering(match_set, detectors);
			break;

		//! covering strategy as in Butz and Wilson 2001
		case COVERING_ACTION_BASED:
			return perform_nma_covering(match_set, detectors);
			break;
		default:
			xcs_utility::error(class_name(),"perform_covering", "covering strategy not allowed", 1);
			exit(-1);
	}
}

//! perform covering based on the number of actions in [M]
bool
xcs_classifier_system::perform_standard_covering(t_classifier_set &match_set, const t_state& detectors)
{
	if ((match_set.size()==0) || need_standard_covering(match_set, detectors))
	{
		t_classifier	classifier;

		//! create a covering classifier
		classifier.cover(detectors);

		//! init classifier parameters
		init_classifier(classifier);


		//! insert the new classifier in [P]
		insert_classifier(classifier);

		//! delete another classifier from [P] if necessary
		delete_classifier();
		
		//! signal that a covering operation took place
		return true;
	}
	return false;
}

bool
xcs_classifier_system::need_standard_covering(t_classifier_set &match_set, const t_state& detectors)
{
	//unsigned long	i;
	t_set_iterator	pp;					//! iterator for visiting [P]
	//unsigned long	sz = 0;					//! number of micro classifiers in [M]
	double		average_prediction;			//!	average prediction in [P]
	double		total_match_set_prediction;		//!	total prediction in [M]

	if (match_set.size()==0)
		return true;

	average_prediction = 0;
	total_match_set_prediction = 0.;

	for(pp=population.begin();pp!=population.end();pp++)
	{
		average_prediction += (*pp)->prediction * (*pp)->numerosity;
		//cout << (*pp)->prediction << "*" << (*pp)->numerosity << "\t";
	}
	//cout << endl << endl;

	average_prediction = average_prediction/population_size;

	for(pp=match_set.begin(); pp!=match_set.end();pp++)
	{
		total_match_set_prediction += (*pp)->prediction * (*pp)->numerosity;
	}

	//cerr << "==> " << total_match_set_prediction << "<=" << fraction_for_covering << " x " << average_prediction << endl;;

	return (total_match_set_prediction<=fraction_for_covering*average_prediction);
}

void	
xcs_classifier_system::build_prediction_array()
{
	t_set_iterator					mp;
	vector<t_system_prediction>::iterator		pr;	
	t_system_prediction				prediction;

	//! clear P(.)
	init_prediction_array();

	//! scan [M] and build the prediction array
	for(mp=match_set.begin(); mp!=match_set.end(); mp++ )
	{
		//!	look whether the action was already found in the prediction array
		pr = find(prediction_array.begin(), prediction_array.end(), ((**mp).action));

		if (pr==prediction_array.end())
		{	
			xcs_utility::error(class_name(),"build_prediction_array", "action not found in prediction array", 1);
			exit(-1);
			/*!	the action was not previously found
			 *	thus prediction array is initialized with the
			 *	classifier values
			 */
			prediction.payoff = (**mp).prediction * (**mp).fitness;
			prediction.sum = (**mp).fitness;
			prediction.n = 1;
			//!	add the element to the prediction array
			prediction_array.push_back(prediction);
		} else {  
			/*!	the action was already in the array
			 *	thus the corresponding value is updated 
			 *	with the classifier values
			 */
			pr->payoff += (**mp).prediction * (**mp).fitness;
			pr->sum += (**mp).fitness;
			pr->n++;
		}
	}

	available_actions.clear();
	for(pr=prediction_array.begin(); pr!=prediction_array.end(); pr++ )
	{
		if (pr->n!=0)
		{
			available_actions.push_back((pr - prediction_array.begin()));
			pr->payoff /= pr->sum;
		}
	}
};

void	xcs_classifier_system::select_action(const t_action_selection policy, t_action& act)
{
	static unsigned long		stat_rnd = 0;
	vector<unsigned long>::iterator	best;
	vector<unsigned long>::iterator	ap;

	switch(policy)
	{
		//! select the action with the highest payoff
		case ACTSEL_DETERMINISTIC:
			assert(available_actions.size()>0);
			random_shuffle(available_actions.begin(),available_actions.end());

			best = available_actions.begin();
			for(ap=available_actions.begin(); ap!=available_actions.end(); ap++)
			{
				if ((prediction_array[*best].payoff) < (prediction_array[*ap].payoff))
					best = ap;
			}
			act = prediction_array[*best].action;
			break;
			
		//! biased action selection
		case ACTSEL_SEMIUNIFORM:
			assert(available_actions.size()>0);
			if (xcs_random::random()<prob_random_action)
			{	//	random action
				act = prediction_array[available_actions[xcs_random::dice(available_actions.size())]].action;
				stat_rnd++;
			} else {
				best = available_actions.begin();
				for(ap=available_actions.begin(); ap!=available_actions.end(); ap++)
				{
					if ((prediction_array[*best].payoff) < (prediction_array[*ap].payoff))
						best = ap;
				}
				act = prediction_array[*best].action;
			}
			break;
		default: 
			xcs_utility::error(class_name(),"select_action", "action selection strategy not allowed", 1);
	};
};

void
xcs_classifier_system::update_set(const double P, t_classifier_set &action_set)
{
	t_set_iterator	clp;
	double		set_size = 0;
	double		fitness_sum;	//! sum of classifier fitness in [A]

	//! update the experience of classifiers in [A]
	//! estimate the action set size
	for(clp=action_set.begin(); clp != action_set.end(); clp++)
	{
		(**clp).experience++;
		set_size += (**clp).numerosity;
		fitness_sum += (**clp).fitness;	//! sums up classifier fitness for gradient descent
	}

	for(clp=action_set.begin(); clp!= action_set.end(); clp++)
	{
		//! prediction error is updated first if required (i.e., flag_error_update is true)
		if (flag_error_update_first)
		{
			//! update the classifier prediction error
			if (!flag_use_mam || ((**clp).experience>(1/learning_rate)))
			{
				(**clp).error += learning_rate*(fabs(P-(**clp).prediction)-(**clp).error);
			} else {
				(**clp).error += (fabs(P-(**clp).prediction)-(**clp).error)/(**clp).experience;
			}
		}

		//! update the classifier prediction

		if (flag_use_gradient_descent)
		{
			//! update the classifier prediction with gradient descent
			(**clp).prediction +=
				learning_rate*(P - (**clp).prediction) * ((**clp).fitness/fitness_sum);
		} else {
			//! usual update of classifier prediction
			if (!flag_use_mam || ((**clp).experience>(1/learning_rate)))
			{
				(**clp).prediction += learning_rate*(P - (**clp).prediction);
			} else {
				(**clp).prediction += (P - (**clp).prediction)/(**clp).experience;
			}
		}

		if (!flag_error_update_first)
		{
			//! update the classifier prediction error
			if (!flag_use_mam || ((**clp).experience>(1/learning_rate)))
			{
				(**clp).error += learning_rate*(fabs(P-(**clp).prediction)-(**clp).error);
			} else {
				(**clp).error += (fabs(P-(**clp).prediction)-(**clp).error)/(**clp).experience;
			}
		}


		//! update the classifier action set size estimate
		if (!flag_use_mam || ((**clp).experience>(1/learning_rate)))
		{
			(**clp).actionset_size += learning_rate*(set_size - (**clp).actionset_size);
		} else {
			(**clp).actionset_size += (set_size - (**clp).actionset_size)/(**clp).experience;
		}
	}

	//! update fitness
	update_fitness(action_set);

	//! do AS subsumption
	if (flag_as_subsumption)
	{	
		do_as_subsumption(action_set);
	}
}

void
xcs_classifier_system::update_fitness(t_classifier_set &action_set)
{
	t_set_iterator 			as;
	double				ra;
	vector<double>			raw_accuracy;
	vector<double>::iterator	rp;
	double				accuracy_sum = 0;

	raw_accuracy.clear();

	for(as = action_set.begin(); as !=action_set.end(); as++)
	{
		if ((**as).error<epsilon_zero)
			ra = (**as).numerosity;
		else 
			ra = alpha*(pow(((**as).error/epsilon_zero),-vi)) * (**as).numerosity;

		raw_accuracy.push_back(ra);
		accuracy_sum += ra;
	}

//	//! use mam for fitness
//
// 	for(as = action_set.begin(), rp=raw_accuracy.begin(); as!=action_set.end(); as++,rp++)
// 	{
// 		if (!flag_use_mam || ((**as).experience>(1/learning_rate)))
// 		{
// 			(**as).fitness += learning_rate*((*rp)/accuracy_sum - (**as).fitness);
// 		} else {
// 			(**as).fitness += ((*rp)/accuracy_sum - (**as).fitness)/(**as).experience;
// 		}
// 	}

	for(as = action_set.begin(), rp=raw_accuracy.begin(); as!=action_set.end(); as++,rp++)
	{
		(**as).fitness += learning_rate*((*rp)/accuracy_sum - (**as).fitness);
	}

}

bool
xcs_classifier_system::subsume(const t_classifier &first, const t_classifier &second)
{
	bool	result;
	
	result = (classifier_could_subsume(first, epsilon_zero, theta_sub)) && (first.subsume(second));

	if (result)
		stats.no_subsumption++;
	
	return result;
}

bool
xcs_classifier_system::need_ga(t_classifier_set &action_set, const bool flag_explore)
{
	double		average_set_stamp = 0;
	unsigned long	size = 0;

	if (!flag_explore) return false;

	t_set_iterator 	as;	

	for(as=action_set.begin(); as!=action_set.end(); as++)
	{
		average_set_stamp += (**as).time_stamp * (**as).numerosity;
		size += (**as).numerosity;
	}

	average_set_stamp = average_set_stamp / size;

	if (total_steps<average_set_stamp)
	{
		cout << "TOTSTEPS = " << total_steps << endl;
		cout << "AVGTS = " << average_set_stamp << endl;
	}

	if (total_steps<average_set_stamp)
		cerr << "NEEDGA " << total_steps << " " << average_set_stamp << endl; 
	assert(total_steps>=average_set_stamp);
	return ((total_steps - average_set_stamp)>=theta_ga);
}

void
xcs_classifier_system::genetic_algorithm(t_classifier_set &action_set, const t_state& detectors, const bool flag_condensation)
{
	t_set_iterator 	parent1;
	t_set_iterator	parent2;

	t_classifier	offspring1;
	t_classifier	offspring2;


	t_set_iterator	as;

	//! set the time stamp of classifiers in [A]
	for(as=action_set.begin(); as!=action_set.end(); as++)
	{
		(**as).time_stamp = total_steps;
	}

	//! select offspring classifiers
	if (!flag_ga_tournament_selection)
	{
		select_offspring(action_set, parent1, parent2);
	} else {
		select_offspring_ts(action_set, parent1);
		select_offspring_ts(action_set, parent2);
	}

	//! the GA is activated only if condensation is off
	if (!flag_condensation)
	{	
		offspring1 = (**parent1);
		offspring2 = (**parent2);

		offspring1.numerosity = offspring2.numerosity = 1;
		offspring1.experience = offspring2.experience = 1;

		if (xcs_random::random()<prob_crossover)
		{
			offspring1.recombine(offspring2);

			//! classifier parameters are inited from parents' averages
			if (flag_ga_average_init)
			{
				init_classifier(offspring1,true);
				init_classifier(offspring2,true);
				offspring1.prediction = offspring2.prediction = ((**parent1).prediction+(**parent2).prediction)/2;
			} else {
				offspring1.prediction = offspring2.prediction = ((**parent1).prediction+(**parent2).prediction)/2;
				offspring1.error = offspring2.error = ((**parent1).error+(**parent2).error)/2;
				offspring1.fitness = offspring2.fitness = ((**parent1).fitness+(**parent2).fitness)/2;
				offspring1.time_stamp = offspring2.time_stamp = total_steps;
				offspring1.actionset_size = offspring2.actionset_size = ((**parent2).actionset_size + (**parent2).actionset_size)/2;
				//offspring1.actionset_size = offspring2.actionset_size = init_set_size;
			}
		}

		offspring1.mutate(prob_mutation,detectors);
		offspring2.mutate(prob_mutation,detectors);

		//! offsprings are penalized through the reduction of fitness
		offspring1.fitness = offspring1.fitness * 0.1;
		offspring2.fitness = offspring2.fitness * 0.1;

		t_condition	cond;
	
		if (cond.allow_ga_subsumption() && flag_ga_subsumption)
		{
			if (subsume(**parent1, offspring1))
			{	//! parent1 subsumes offspring1
				(**parent1).numerosity++;
				population_size++;
			} else if (subsume(**parent2, offspring1))
			{	//! parent2 subsumes offspring1
				(**parent2).numerosity++;
				population_size++;
			} else {
				//! neither of the parent subsumes offspring1
				if (!flag_gaa_subsumption)
				{
					//! if the usual GA subsumption is used, offspring classifier is inserted
					insert_classifier(offspring1);
				} else {
					//! if Martin's GA subsumption is used, offspring classifier is compared to the classifiers in [A]
					t_set_iterator 	par;

					ga_a_subsume(action_set,offspring1,par);
					if (par!=action_set.end())
					{				
						(**par).numerosity++;
						population_size++;
					} else {
						insert_classifier(offspring1);
					}
				}
			}
	
			if (subsume(**parent1, offspring2))
			{	//! parent1 subsumes offspring2
				(**parent1).numerosity++;
				population_size++;
			}
			else if (subsume(**parent2, offspring2))
			{	//! parent2 subsumes offspring2
				(**parent2).numerosity++;
				population_size++;
			} else {
				//! neither of the parent subsumes offspring1
				if (!flag_gaa_subsumption)
				{
					//! if the usual GA subsumption is used, offspring classifier is inserted
					insert_classifier(offspring2);
				} else {
					//! if Martin's GA subsumption is used, offspring classifier is compared to the classifiers in [A]
					t_set_iterator 	par;

					ga_a_subsume(action_set,offspring2,par);
					if (par!=action_set.end())
					{				
						(**par).numerosity++;
						population_size++;
					} else {
						insert_classifier(offspring2);
					}
				}
			}
			delete_classifier();
			delete_classifier();
		} else {	
			// insert offspring classifiers without subsumption
			insert_classifier(offspring1);
			insert_classifier(offspring2);

			delete_classifier();
			delete_classifier();
		}

	} else {
		// when in condensation
		(**parent1).numerosity++;
		population_size++;
		delete_classifier();

		(**parent2).numerosity++;
		population_size++;
		delete_classifier();
	}
}

void	
xcs_classifier_system::step(const bool exploration_mode, const bool condensationMode)
{
	t_action	action;					//! selected action
	unsigned long	match_set_size;		//! number of microclassifiers in [M]
	//unsigned long	action_set_size;	//! number of microclassifiers in [A]
	double		P;						//! value for prediction update, computed as r + gamma * max P(.) 
	double		max_prediction;

	cout << "[xcs] step" << endl;
	
	//! reads the current input
	current_input = Environment->state(); 
	cout << "[xcs]: got current input" << endl;

	vector<double> meas;
	current_input.numeric_representation(meas);

	#if 0
	for(auto i=0; i<3; i++)
		cout << "[xcs]: measurement " << i << " = " << meas[i] << endl;
	#endif

	#if 1
	//! update the number of learning steps performed so far
	if (exploration_mode)
	{
		total_steps++;
	}

	/*! 
	 * check if [M] needs covering,
	 * if it does, it apply the selected covering strategy, i.e., standard as defined in Wilson 1995,
	 * or action_based as defined in Butz and Wilson 2001
	 */
	int n=0;
	do {
		n++;
		cout << "[xcs]: match(1)" << endl;
		match_set_size = match(current_input);
		cout << "[XCS] matchset size " << match_set_size << endl;
		//cout << "[XCS] ich finde kein match" << endl;
	}
   	while (perform_covering(match_set, current_input));
	//cout << "[xcs]: got covering" << endl;
	//cout << "[xcs]: match set size = " << match_set_size << endl;
	//! build the prediction array P(.)
	build_prediction_array();

#ifdef __DEBUG__
	cout << "BUILT THE PREDICTION ARRAY" << endl;
	print_prediction_array(cout);
	cout << endl;
#endif
	
	//! select the action to be performed
	if (exploration_mode)
		select_action(action_selection_strategy, action);
	else 
		select_action(ACTSEL_DETERMINISTIC, action);

	//! build [A]
	build_action_set(action);

#ifdef __DEBUG__
	cout << "ACTION " << action << endl;
#endif
	//! store the current input before performing the selected action
	/*!
	 * used by the genetic algorithm
	 */
	previous_input = Environment->state();

	Environment->perform(action);

	//! if the environment is single step, the system error is collected
	if (Environment->single_step())
	{
		double payoff = prediction_array[action.value()].payoff;
		system_error = fabs(payoff-Environment->reward());
	}

#ifdef __DEBUG__
	cout << "ACTION " << action << "\t";
	cout << "REWARD " << Environment->reward() << endl;

	if (flag_update_test)
		cerr << "Update During Test" << endl;
	else 
		cerr << "No Update During Test" << endl;
#endif

	total_reward = total_reward + Environment->reward();

	//! reinforcement component
	
	//! if [A]-1 is not empty it computes P
	if ((exploration_mode || flag_update_test) && previous_action_set.size())
	{
		// TODO: check why we get here and what happens here!!!
		//cout << "[xcs]: in reinforcment stuff that i donÄt want" << endl;
#ifdef __DEBUG__
		cerr << "Fa l'update" << endl;
#endif

		vector<t_system_prediction>::iterator	pr = prediction_array.begin();
		max_prediction = pr->payoff;

		for(pr = prediction_array.begin(); pr!=prediction_array.end(); pr++)
		{
			if (max_prediction<pr->payoff)
			{
				max_prediction = pr->payoff;
			}
		}

		P = previous_reward + discount_factor * max_prediction;

		//! use P to update the classifiers parameters
		update_set(P, previous_action_set);
	}

	if (Environment->stop())
	{
		P = Environment->reward();
		if (exploration_mode || flag_update_test)
		{
#ifdef __DEBUG__
			cerr << "Fa l'update" << endl;
#endif
			update_set(P, action_set);
		}
	}

	//! apply the genetic algorithm to [A] if needed
	if (flag_discovery_component && need_ga(action_set, exploration_mode))
	{
		genetic_algorithm(action_set, previous_input, condensationMode);
		stats.no_ga++;
	}
	
	//!	[A]-1 <= [A]
	//!	r-1 <= r
	previous_action_set = action_set;
	action_set.clear();
	previous_reward = Environment->reward();
	#endif
}


void	
xcs_classifier_system::step(const bool exploration_mode, const bool condensationMode, const bool performAction)
{
	t_action	action;					//! selected action
	unsigned long	match_set_size;		//! number of microclassifiers in [M]
	//unsigned long	action_set_size;	//! number of microclassifiers in [A]
	double		P;						//! value for prediction update, computed as r + gamma * max P(.) 
	double		max_prediction;

	//cout << "[xcs] step" << endl;
	
	if(performAction)
	{
		//! reads the current input
		current_input = Environment->state(); 
		//cout << "[xcs]: got current input" << endl;

		vector<double> meas;
		current_input.numeric_representation(meas);

		#if 0
		for(auto i=0; i<3; i++)
			cout << "[xcs]: measurement " << i << " = " << meas[i] << endl;
		#endif

		//! update the number of learning steps performed so far
		if (exploration_mode)
		{
			total_steps++;
		}

		/*! 
		* check if [M] needs covering,
		* if it does, it apply the selected covering strategy, i.e., standard as defined in Wilson 1995,
		* or action_based as defined in Butz and Wilson 2001
		*/
		int n=0;
		do {
			n++;
			//cout << "[xcs]: match(1)" << endl;
			match_set_size = match(current_input);
			//cout << "[XCS] matchset size " << match_set_size << endl;
			//cout << "[XCS] ich finde kein match" << endl;
		}
		while (perform_covering(match_set, current_input));
		//cout << "[xcs]: got covering" << endl;
		//cout << "[xcs]: match set size = " << match_set_size << endl;
		//! build the prediction array P(.)
		build_prediction_array();

	#ifdef __DEBUG__
		cout << "BUILT THE PREDICTION ARRAY" << endl;
		print_prediction_array(cout);
		cout << endl;
	#endif
		
		//! select the action to be performed
		if (exploration_mode)
			select_action(action_selection_strategy, action);
		else 
			select_action(ACTSEL_DETERMINISTIC, action);

		//! build [A]
		build_action_set(action);

	#ifdef __DEBUG__
		cout << "ACTION " << action << endl;
	#endif
		//! store the current input before performing the selected action
		/*!
		* used by the genetic algorithm
		*/
		previous_input = Environment->state();

		Environment->perform(action);

		//! if the environment is single step, the system error is collected
		if (Environment->single_step())
		{
			double payoff = prediction_array[action.value()].payoff;
			system_error = fabs(payoff-Environment->reward());
		}

	#ifdef __DEBUG__
		cout << "ACTION " << action << "\t";
		cout << "REWARD " << Environment->reward() << endl;

		if (flag_update_test)
			cerr << "Update During Test" << endl;
		else 
			cerr << "No Update During Test" << endl;
	#endif

	}	// performAction
	else
	{
		total_reward = total_reward + Environment->reward();

		//! reinforcement component
		
		//! if [A]-1 is not empty it computes P
		if ((exploration_mode || flag_update_test) && previous_action_set.size())
		{
			//cout << "[xcs]: in reinforcment stuff that i donÄt want" << endl;
	#ifdef __DEBUG__
			cerr << "Fa l'update" << endl;
	#endif

			vector<t_system_prediction>::iterator	pr = prediction_array.begin();
			max_prediction = pr->payoff;

			for(pr = prediction_array.begin(); pr!=prediction_array.end(); pr++)
			{
				if (max_prediction<pr->payoff)
				{
					max_prediction = pr->payoff;
				}
			}

			P = previous_reward + discount_factor * max_prediction;

			//! use P to update the classifiers parameters
			update_set(P, previous_action_set);
		}

		if (Environment->stop())
		{
			P = Environment->reward();
			if (exploration_mode || flag_update_test)
			{
	#ifdef __DEBUG__
				cerr << "Fa l'update" << endl;
	#endif
				update_set(P, action_set);
			}
		}

		//! apply the genetic algorithm to [A] if needed
		if (flag_discovery_component && need_ga(action_set, exploration_mode))
		{
			genetic_algorithm(action_set, previous_input, condensationMode);
			stats.no_ga++;
		}
		
		//!	[A]-1 <= [A]
		//!	r-1 <= r
		previous_action_set = action_set;
		action_set.clear();
		previous_reward = Environment->reward();
	}

}

void	
xcs_classifier_system::save_population(ostream& output) 
{


	t_set_iterator		pp;
	t_classifier_set 	save;

	save = population;

	std::sort(save.begin(), save.end(), comp_numerosity());
	for(pp=save.begin(); pp!=save.end(); pp++)
	{
		(**pp).print(output);
		output << endl;
	}
}

void	
xcs_classifier_system::save_state(ostream& output) 
{
	output << stats << endl;
	output << total_steps << endl;
	t_classifier::save_state(output);
	output << macro_size << endl;

	t_set_iterator	pp;

	for(pp=population.begin(); pp!=population.end(); pp++)
	{
		output << **pp << endl;
		output << endl;
	}
	output << endl;
}

void	
xcs_classifier_system::save_population_state(ostream& output) 
{
	t_set_iterator	pp;

	for(pp=population.begin(); pp!=population.end(); pp++)
	{
		output << (**pp) << endl;
	}
}


void
xcs_classifier_system::restore_state(istream& input)
{
	unsigned long size;
	input >> stats;
	input >> total_steps;
	t_classifier::restore_state(input);
	input >> size;
	
	population.clear();
	
    	t_classifier in_classifier;
	population_size = 0;
	macro_size = 0;

	for(unsigned long cl=0; cl<size; cl++)
	{
		if (!input.eof() && (input >> in_classifier))
		{
			t_classifier	*classifier = new t_classifier(in_classifier);
			population.push_back(classifier);
			population_size += classifier->numerosity;
			macro_size++;
		}
	};
	assert(macro_size==size);
}

//! defines what has to be done when a new experiment begins
void 
xcs_classifier_system::begin_experiment()
{
	//! reset the overall time step
	total_steps = 0;

	//! reset the number of overall learning steps
	total_learning_steps = 0;

	//! init the experiment statistics
	stats.reset();
	
	//! [P] contains 0 macro/micro classifiers
	population_size = 0;
	macro_size = 0;

	//! init [P]
	init_classifier_set();
}


//! defines what has to be done when a new problem begins. 
void
xcs_classifier_system::begin_problem()
{
	//! clear [A]-1
	previous_action_set.clear();
	
	//! clear [A]
	action_set.clear();

	//! set the steps within the problem to 0
	problem_steps = 0;

	//! clear the total reward gained
	total_reward = 0;

	//! added to clear the population
	/*while (population_size>max_population_size)
		delete_classifier();*/
}

//! defines what must be done when the current problem ends
void 
xcs_classifier_system::end_problem() 
{
	match_set.clear();
	action_set.clear();
}



bool	
xcs_classifier_system::perform_nma_covering(t_classifier_set &match_set, const t_state& detectors)
{
	vector<t_system_prediction>::iterator	pr;
	t_action		act;
	unsigned long		total_actions = act.actions();
	unsigned long		covered_actions = total_actions;
	bool			covered_some_actions = false;		//! becomes true when covering classifiers are created

	//! clear the prediction array
	init_prediction_array();

	//! build P(.)
	build_prediction_array();

	/*for(pr=prediction_array.begin(); pr!=prediction_array.end(); pr++ )
	{
		//! 
		if (pr->n==0)
		{	
			covered_actions--;
		}
	}
	*/

	//! the number of actions that are covered is computed as the number of available actions
	//! in the prediction array P(.)
	
	covered_actions = available_actions.size();
	cout << "[xcs]: covered actions = " << covered_actions << endl;
	cout << "[xcs]: theta_nma = " << tetha_nma <<endl;
	covered_some_actions = false;

	if (covered_actions<tetha_nma)
	{
		for(pr=prediction_array.begin(); pr!=prediction_array.end() && (covered_actions<tetha_nma); pr++ )
		{
			//! 
			if (pr->n==0)
			{
				t_classifier	classifier;
				
				classifier.cover(detectors);
				classifier.action = pr->action;

				init_classifier(classifier, flag_cover_average_init);
				
				insert_classifier(classifier);

				delete_classifier();

				covered_actions++;
				cout << "[xcs]: covered actions increased"  << endl;
			}
		}
		covered_some_actions = true;
	}

	return covered_some_actions;
};

void	
xcs_classifier_system::init_classifier(t_classifier& classifier, bool average)
{
	if (!average || (population_size==0))
	{
		classifier.prediction = init_prediction;
		classifier.error = init_error;
		classifier.fitness = init_fitness;
	
		classifier.actionset_size = init_set_size;
		classifier.experience = 0;
		classifier.time_stamp = total_steps;

		classifier.numerosity = 1;
	} else {
		t_set_iterator	cl;

		double		toterror = 0;
	   	double		totprediction = 0;
	   	double		totfitness = 0;
	   	double		totactionset_size = 0;
		unsigned long	popSize = 0;
		unsigned long	pop_sz = 0;
			
		for(cl=population.begin(); cl!=population.end(); cl++)
		{
			toterror += (**cl).error * (**cl).numerosity;
			totprediction += (**cl).prediction * (**cl).numerosity;
			totfitness += (**cl).fitness;
			totactionset_size += (**cl).actionset_size * (**cl).numerosity;
			popSize += (**cl).numerosity;
			pop_sz++;
		}

		classifier.prediction = totprediction/popSize;
		classifier.error = .25 * toterror/popSize;
		classifier.fitness = 0.1 * totfitness/pop_sz;
		classifier.actionset_size = totactionset_size/popSize;
		classifier.numerosity = 1;
		classifier.time_stamp = total_steps;
		assert(classifier.actionset_size>=0);
		assert(classifier.fitness>=0);
	};
}

//! build [A] from [M] and an action "act"
/*!
 * \param action selected action
 */
void	
xcs_classifier_system::build_action_set(const t_action& action)
{
	//! iterator in [M]
	t_set_iterator 	mp;

	//! clear [A]
	action_set.clear();

	//! fill [A] with the classifiers in [M] that have action "act"
	for( mp=match_set.begin(); mp!=match_set.end(); mp++ )
	{
		//if ((**mp).action==action)
		if ((*mp)->action==action)
		//if (true)
		{
			action_set.push_back(*mp);
		}
	}
	random_shuffle(action_set.begin(), action_set.end());
}

//! clear [P]
void
xcs_classifier_system::clear_population()
{
	//! iterator in [P]
	t_set_iterator	pp;

	//! delete all the classifiers in [P]
	for(pp=population.begin(); pp!=population.end(); pp++)
	{
		delete *pp; 
	}

	//! delete all the pointers in [P]
	population.clear();

	//! number of macro classifiers is set to 0
	macro_size = 0;

	//! number of micro classifiers is set to 0
	population_size = 0;
}

//! print a set of classifiers
/*!
 * \param set set of classifiers
 * \bug cause a memory leak!
 */
void 
xcs_classifier_system::print_set(t_classifier_set &set, ostream& output) 
{
	t_set_const_iterator	pp;
	t_classifier	cl;

	output << "================================================================================" << endl;
	for(pp=set.begin(); pp!=set.end(); pp++)
	{
		(**pp).print(output);
		output << endl;
	}
	output << "================================================================================" << endl;
}

//! check the integrity of the population
/*!
 * \param str comment that is printed before the trace information
 * \param output stream where the check information are printed
 */
void
xcs_classifier_system::check(string str, ostream& output) 
{
	//! iterator in [P]
	t_set_iterator	pp;

	//! check for the number of microclassifiers
	unsigned long check_population_size = 0;
	//! check for the number of macroclassifiers
	unsigned long check_macro_size = 0;

	//! count the number of micro and macro classifiers in [P]
	output << "CHECK <" << str << ">" << endl;
	output << "======================================================================" << endl;
	for(pp=population.begin(); pp!=population.end(); pp++)
	{
		check_population_size += (**pp).numerosity;
		check_macro_size ++;
	}
	output << "counter   = " << population_size << endl;
	output << "check     = " << check_population_size << endl;
	output << "limit     = " << max_population_size << endl;
	output  << "======================================================================" << endl;

	//! compare che check parameters to the current parameters
	assert(check_macro_size==macro_size);
	assert(check_population_size==population_size);
}


//! perform the operations needed at the end of the experiment
void 
xcs_classifier_system::end_experiment() 
{

}

//@{
//! set the strategy to init [P] at the beginning of the experiment
/*! 
 * \param strategy can be either \emph empty or \emph random
 * two strategies are allowed \emph empty set [P] to the empty set
 * \emph random fills [P] with random classifiers
 */
void
xcs_classifier_system::set_init_strategy(string strategy)
{

#ifdef __DEBUG__
	cout << "STRATEGY " << strategy << endl;
	cout << "PREFIX " << strategy.substr(0,5)<< endl;
	cout << "SUFFIX " << strategy.substr(5)<< endl;
#endif

	if ( (strategy!="random") && (strategy!="empty") && (strategy.substr(0,5)!="load:") )
	{	
		//!	unrecognized deletion strategy
		string	msg = "Unrecognized population init policy";
		xcs_utility::error(class_name(),"set_init_strategy", msg, 1);
	}

	if (strategy=="empty")
		population_init = INIT_EMPTY;
	else if (strategy=="random")
		population_init = INIT_RANDOM;
	else if (strategy.substr(0,5)=="load:")
	{
		population_init = INIT_LOAD;
		population_init_file = strategy.substr(5);
		cout << "LOAD FROM <" << population_init_file << ">" << endl;
	} else {
		//!	unrecognized deletion strategy
		string	msg = "Unrecognized population init policy";
		xcs_utility::error(class_name(),"set_init_strategy", msg, 1);
	}
}
//@}

//! select offspring classifiers from the population
//	20020722 modified the two for cycles so to continue from one to another
void
xcs_classifier_system::select_offspring(t_classifier_set &action_set, t_set_iterator &clp1, t_set_iterator &clp2)
{
	t_set_iterator	as;		//! iterator in [A]
	vector<double>	select;		//! vector used to implement the roulette wheel
	unsigned long	sel;		//! counter

	double		fitness_sum;
	double		random1;
	double		random2;

	select.clear();

	fitness_sum = 0;
	for(as=action_set.begin(); as!=action_set.end(); as++)
	{
		fitness_sum += (**as).fitness;
		select.push_back( fitness_sum );
	}

	random1 = (xcs_random::random())*fitness_sum;
	random2 = (xcs_random::random())*fitness_sum;

	if (random1>random2)
		swap(random1,random2);

	for(sel = 0; (sel<select.size()) && (random1>=select[sel]); sel++);
	clp1 = action_set.begin()+sel;	// to be changed if list containers are used
	assert(sel<select.size());

	for(; (sel<select.size()) && (random2>=select[sel]); sel++);
	clp2 = action_set.begin()+sel;	// to be changed if list containers are used
	assert(sel<select.size());
}

void	
xcs_classifier_system::set_covering_strategy(const string strategy, double threshold)
{
	if (strategy=="standard")
	{
		covering_strategy = COVERING_STANDARD;
		fraction_for_covering = threshold;
	} else if (strategy=="action_based") {
		t_action	action;

		covering_strategy = COVERING_ACTION_BASED;

		//! the special value 0 for tetha_nma specifies that all the actions must be covered
		
		if (threshold==0)
			tetha_nma = action.actions();
		else 
			tetha_nma = threshold;

		if (tetha_nma>action.actions())
		{
			xcs_utility::error(class_name(),"set_covering_strategy", "value for the threshold must not exceed the number of available actions", 1);

		}
	}
}

void	
xcs_classifier_system::create_prediction_array()
{
	t_action		action;
	t_system_prediction	prediction;

	//! clear the prediction array
	prediction_array.clear();

	//cout << "NUMBER OF ACTIONS " << action.actions() << endl;
	//!	build the prediction array with all the possible actions
	action.reset_action();

	do {
		//clog << "ADD ACTION IN PREDICTION ARRAY <" << action << ">" << endl; 
		prediction.action = action;
		prediction.n = 0;
		prediction.payoff = 0;
		prediction.sum = 0;
		prediction_array.push_back(prediction);
	} while (action.next_action());

}

void	
xcs_classifier_system::init_prediction_array()
{
	vector<t_system_prediction>::iterator	sp;

	for(sp=prediction_array.begin(); sp!=prediction_array.end(); sp++)
	{
		sp->n =0;
		sp->payoff = 0;
		sp->sum = 0;
	}
}

void 
xcs_classifier_system::erase_population()
{
	t_set_iterator			pp;		//! iterator for visiting [P]
	for(pp=population.begin(); pp!=population.end(); pp++)
	{
		delete (*pp);
	}
	population.clear();

	population_size = 0;
}

//! delete a set of classifiers from [P], [M], [A], [A]-1
void	
xcs_classifier_system::as_subsume(t_set_iterator classifier, t_classifier_set &set)
{
	t_set_iterator	sp;		//! iterator for visiting the set of classifier
	t_set_iterator	pp;		//! iterator for visiting [P]

        t_classifier *most_general;	//! keeps track of the most general classifier

	most_general = *classifier;

	for(sp=set.begin(); sp!=set.end(); sp++)
	{
		//! remove cl from [M], [A], and [A]-1
		t_set_iterator	clp;
		clp = find(action_set.begin(),action_set.end(), *sp);
		if (clp!=action_set.end())
		{
			action_set.erase(clp);
		}

		clp = find(match_set.begin(),match_set.end(), *sp);
		if (clp!=match_set.end())
		{
			match_set.erase(clp);
		}

		clp = find(previous_action_set.begin(),previous_action_set.end(), *sp);
		if (clp!=previous_action_set.end())
		{
			previous_action_set.erase(clp);
		}


		pp = lower_bound(population.begin(),population.end(),*sp,compare_cl);
		if ((pp!=population.end()))
		{
			//! found another classifier, something is wrong
			if ( (*pp)!=(*sp) )
			{
				xcs_utility::error(class_name(),"as_subsumption", "classifier not found", 1);
				exit(-1);
			}
		}

		macro_size--;

                most_general->numerosity += (*pp)->numerosity;

		delete *pp;
		population.erase(pp);
	}
	set.clear();
}

//! find the classifiers in set that are subsumed by the classifier 
void	
xcs_classifier_system::find_as_subsumed(
	t_set_iterator classifier, 
	t_classifier_set &set, 
	t_classifier_set &subsumed)
const
{
	t_set_iterator	sp;	//! pointer to the elements in the classifier set

	subsumed.clear();

	if (classifier!=set.end())
	{
		for( sp=set.begin(); sp!=set.end(); sp++ )
		{
			if (*classifier!=*sp)
			{
				//if ( classifier_could_subsume( (**sp), epsilon_zero, theta_sub) && 
				//     (*classifier)->is_more_general_than(**sp))
				if ((*classifier)->is_more_general_than(**sp))
				{
					subsumed.push_back(*sp);
				}
			}
		}
	}
}

//! perform action set subsumption on the classifier in the set
void	
xcs_classifier_system::do_as_subsumption(t_classifier_set &set)
{
	/*! 
	 * \brief check whether the condition type allow action set subsumption
	 *
	 * the same check is already performed at construction time. 
	 * thus this might be deleted.
	 */
	t_condition	cond;
	if (!cond.allow_as_subsumption())
	{
		xcs_utility::error(class_name(),
			"do_as_subsumption", 
			"condition does not allow action set subsumption", 1);
	}

	//! find the most general classifier
	t_set_iterator most_general;
	
	most_general = find_most_general(set);

	if ((most_general!=set.end()) && !classifier_could_subsume(**most_general, epsilon_zero, theta_sub))
	{
		xcs_utility::error(class_name(),
			"do_as_subsumption", 
			"classifier could not subsume", 1);
	}


	//! if there is a "most general" classifier, it extracts all the subsumed classifiers
	if (most_general!=set.end())
	{
		t_classifier_set subsumed;

		find_as_subsumed(most_general, set, subsumed);

		if (subsumed.size())
		{
			as_subsume(most_general, subsumed);
		}
	}
}

xcs_classifier_system::t_set_iterator
xcs_classifier_system::find_most_general(t_classifier_set &set) const
{
	t_set_iterator 	most_general = set.end();	
	
	t_set_iterator	sp;

	for( sp=set.begin(); sp!=set.end(); sp++ )
	{
		if ( classifier_could_subsume( (**sp), epsilon_zero, theta_as_sub) )
		{
			if (most_general==set.end())
				most_general = sp;
			else {
				if ( (*sp)->subsume(**most_general))
						
					most_general = sp;
			}
		}
	}

	return most_general;
}

void
xcs_classifier_system::init_population_random()
{
	unsigned long	cl;
	timer		check;

	erase_population();

	check.start();
	for(cl=0; cl<max_population_size; cl++)
	{
		t_classifier *classifier = new t_classifier();
		classifier->random();
		init_classifier(*classifier);
		insert_classifier(*classifier);
	}

	check.stop();
	macro_size = population.size();
	population_size = max_population_size;
};

void
xcs_classifier_system::print_prediction_array(ostream& output) const
{
	vector<t_system_prediction>::const_iterator		pr;	

	for(pr=prediction_array.begin(); pr!=prediction_array.end(); pr++)
	{
		output << "(" << pr->action << ", " << pr->payoff << ")";
	}
}

void
xcs_classifier_system::select_offspring_ts(t_classifier_set& set, t_set_iterator& clp)
{
	t_set_iterator	as;				//! iterator in set
	t_set_iterator	winner = set.end();

	while (winner==set.end())
	{
		for(as=set.begin(); as!=set.end(); as++)
		{
			bool selected = false;

			for(unsigned long num=0; (!selected && (num<(**as).numerosity)); num++)
			{
				if (xcs_random::random()<tournament_size)
				{
					if ((winner==set.end()) ||
					    (((**winner).fitness/(**winner).numerosity)<((**as).fitness/(**as).numerosity)))
					{
						winner = as;
						selected = true;
					}
				}
			}
		}
	}

	clp = winner;

}

void
xcs_classifier_system::init_population_load(string filename)
{

	population.clear();		//! clear [P] before loading (20030808)

	ifstream	POPULATION(filename.c_str());

	if (!POPULATION.good())
	{
		xcs_utility::error( class_name(), "init_population_load", "file <"+filename+"> not found", 1);
	}
	t_classifier	in_classifier;
	unsigned long	n = 0;
	macro_size = 0;
	population_size = 0;

	while(!(POPULATION.eof()) && POPULATION>>in_classifier)
	{
		t_classifier	*classifier = new t_classifier(in_classifier);
		classifier->time_stamp = total_steps;
		population.push_back(classifier);
		population_size += classifier->numerosity;
		macro_size++;
	}
	sort(population.begin(),population.end(),compare_cl);
}

//! random deletion 
xcs_classifier_system::t_set_iterator
xcs_classifier_system::select_delete_random(t_classifier_set &set)
{
	t_set_iterator 	pp;
	vector<double>	select;

	unsigned long	random;
	unsigned long	size;

	unsigned long	sel;
	unsigned long	sum;

	select.clear();
	select.reserve(set.size());

	//! check [P]

	size = 0;

	for(pp=set.begin(); pp!=set.end(); pp++)
	{
		size += (**pp).numerosity;
		select.push_back(size);
	}

	random = xcs_random::dice(size);


	pp = set.begin();

	for(sel=0; ((sel<select.size()) && (random>select[sel])); sel++,pp++);
	assert(sel<select.size());
	
	return pp;
}

//! roulette wheel selection
xcs_classifier_system::t_set_iterator
xcs_classifier_system::select_delete_rw(t_classifier_set &set)
{
	t_set_iterator 	pp;
	vector<double>	select;

	double		average_fitness = 0.;
	double		vote_sum;
	double		vote;
	double		random;
	double		size;

	unsigned long	sel;

	select.clear();
	select.reserve(set.size());

	//! check [P]
	//check("enter delete", clog);

	for(pp=set.begin(); pp!=set.end(); pp++)
	{
		average_fitness += (**pp).fitness;
		size += (**pp).numerosity;
	}

	average_fitness /= ((double) size);

	vote_sum = 0;

	unsigned long	el = 0;		//! set element
	for(pp=set.begin(); pp!=set.end(); pp++)
	{
		//! compute the deletion vote;
		vote = (**pp).actionset_size * (**pp).numerosity;

		if (flag_delete_with_accuracy)
		{
			if (((**pp).experience>theta_del) && ((((**pp).fitness)/double((**pp).numerosity))<delta_del*average_fitness))
			{
				vote = vote * average_fitness/(((**pp).fitness)/double((**pp).numerosity));
			}
		}
		vote_sum += vote;
		select.push_back(vote_sum);
	}

	random = vote_sum*(xcs_random::random());

	pp = set.begin();

	for(sel=0; ((sel<select.size()) && (random>select[sel])); sel++,pp++);

	assert(sel<select.size());
	
	return pp;
}

//! delete classifier from the population according to the selected strategy
void
xcs_classifier_system::delete_classifier()
{	
	t_set_iterator 	pp;

	double		average_fitness = 0.;
	double		vote_sum;
	double		vote;
	double		random;

	unsigned long	sel;

	if (population_size<=max_population_size)
		return;

	switch(delete_strategy)
	{
 		case XCS_DELETE_RWS_SETBASED:
 		case XCS_DELETE_RWS_FITNESS:
			pp = select_delete_rw(population);
			break;
		case XCS_DELETE_RANDOM:				//! random delete
		case XCS_DELETE_RANDOM_WITH_ACCURACY:		//! random delete
			pp = select_delete_random(population);
			break;
		default:
			xcs_utility::error(class_name(),"delete_classifier", "delete strategy not allowed", 1);
	}

	if ((**pp).numerosity>1)
	{
		(**pp).numerosity--;
		population_size--;
	} else {
		//	remove cl from [M], [A], and [A]-1
		t_set_iterator	clp;
		clp = find(action_set.begin(),action_set.end(), *pp);
		if (clp!=action_set.end())
		{
			action_set.erase(clp);
		}

		clp = find(match_set.begin(),match_set.end(), *pp);
		if (clp!=match_set.end())
		{
			match_set.erase(clp);
		}

		clp = find(previous_action_set.begin(),previous_action_set.end(), *pp);
		if (clp!=previous_action_set.end())
		{
			previous_action_set.erase(clp);
		}

		delete *pp;
		
		population.erase(pp);
		population_size--;
		macro_size--;
	}
}

double	
xcs_classifier_system::specificity(const t_classifier_set &set) const
{
	double specificity = 0;
	double sz = 0;

	t_set_const_iterator	clp;

	for(clp=set.begin(); clp != set.end(); clp++)
	{
// 		specificity += (**clp).condition.specificity()*(**clp).numerosity; 
		sz += (**clp).numerosity;
	}
	return specificity/sz;
}

double	
xcs_classifier_system::average_gradient(const t_classifier_set &set) const
{
	t_set_const_iterator	clp;

	double	fitness_sum = 0;	//! sum of classifier fitness in [A]
	double	avg_gradient = 0;	//! average gradient
	
	//! compute fitness sum
	for(clp=set.begin(); clp != set.end(); clp++)
	{
		fitness_sum += (**clp).fitness;
	}

	//! compute sum of gradients
	avg_gradient = 0;
	for(clp=set.begin(); clp != set.end(); clp++)
	{
		double gr = ((**clp).fitness/fitness_sum);
		assert(gr<=double(1.0));
		avg_gradient += ((**clp).fitness/fitness_sum);
	}

#ifdef __DEBUG__
	cerr << "AVG GR = "  << avg_gradient << "/" << action_set.size();
#endif
	//! average gradient
	avg_gradient = avg_gradient/action_set.size();

	return avg_gradient;
}

//! check whether cl is subsumed by any of the classifiers in the action set
void
xcs_classifier_system::ga_a_subsume(t_classifier_set &action_set, const t_classifier &cl, t_set_iterator &mg)
{
	cout << "SI ";
	t_set_iterator	as;
	
	mg = action_set.end();
	
	//! set the time stamp of classifiers in [A]
	for(as=action_set.begin(); (mg==action_set.end())&&(as!=action_set.end()); as++)
	{
		if (subsume(**as, cl))
		{
			mg = as;	
		}
	}
}
