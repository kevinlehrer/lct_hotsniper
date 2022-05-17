#include "dvfsXCS.h"
#include <iomanip>
#include <iostream>
#include <vector>

using namespace std;

t_classifier_system* XCS;
t_environment* Environment;
int global_core_id;         // temporary stores a core id for the current core in control loop
float global_frequency;     // temporary stores old frequency for current core in control loop

vector<t_classifier_system> xcs_systems;


DVFSxcs::DVFSxcs(
        const PerformanceCounters *performanceCounters,
        int coreRows,
        int coreColumns,
        int minFrequency,
        int maxFrequency,
        int frequencyStepSize,
        float upThreshold,
        float downThreshold,
        float dtmCriticalTemperature,
        float dtmRecoveredTemperature)
    : performanceCounters(performanceCounters),
      coreRows(coreRows),
      coreColumns(coreColumns),
      minFrequency(minFrequency),
      maxFrequency(maxFrequency),
      frequencyStepSize(frequencyStepSize),
      upThreshold(upThreshold),
      downThreshold(downThreshold),
      dtmCriticalTemperature(dtmCriticalTemperature),
      dtmRecoveredTemperature(dtmRecoveredTemperature)
{
    //! output information about xcs classifier build
    cout << "[Scheduler][xcs] Initializing XCS classifier system" << endl;
    cerr << "\tXCSLIB\tVERSION " << __XCSLIB_VERSION__ << endl;
    cerr << "      \t\tBUILT   " << __DATE__ << endl;
    cerr << "      \t\tTIME    " << __TIME__ << endl;
    cerr << endl;
    cerr << "      \t\tSTATE       " << __INPUTS_VERSION__ << endl;
    cerr << "      \t\tACTION      " << __ACTION_VERSION__ << endl;
    cerr << "      \t\tCONDITIIONS " << __CONDITION_VERSION__ << endl;
    cerr << endl << endl;

    //! init the configuration manager
    xcs_config2 = xcs_config_mgr2(); 

	//! init random the number generator
	xcs_random::set_seed(xcs_config2);

    //! set global core id initially to 0
    global_core_id = 0;

	//! init the action class
    dummy_action = new t_action(xcs_config2);

	//! init the environment
	Environment = new t_environment(xcs_config2, performanceCounters);

	//! init the condition class
    dummy_condition = new t_condition(xcs_config2);

	//! init the XCS classifier system
    XCS = new t_classifier_system(xcs_config2);

	//! init the experiment manager
	Session = new experiment_mgr(xcs_config2);

    //! true if condensation is active
    flag_condensation = false;

    //! the first problem is always solved in exploration
    flag_exploration = true;

    //! init XCS for the current experiment
    XCS->begin_experiment();
    XCS->begin_problem();
    Environment->begin_problem(true);

    //! create xcs classifier for each core
    for(auto i=0; i< coreRows * coreColumns; i++)
    {
        cout << "[Scheduler][xcs] Classifier System: Creating system for core " << i << endl;
        t_classifier_system xcs_system = t_classifier_system(xcs_config2);
        xcs_systems.push_back(xcs_system);
    }

    //! initialize xcs classifier systems
    for(auto i=0; i< coreRows * coreColumns; i++)
    {
        cout << "[Scheduler][xcs] Classifier System: Starting problem for core " << i << endl;
        xcs_systems[i].begin_experiment();
        xcs_systems[i].begin_problem();
    }
    




    # if 1
	int coreCounter = global_core_id;
	/* get performance counters for current core    */
	float power = performanceCounters->getPowerOfCore(coreCounter);     // not needed
	float temperature = performanceCounters->getTemperatureOfCore(
		coreCounter);
	//int frequency = oldFrequencies.at(coreCounter);
	float utilization = performanceCounters->getUtilizationOfCore(
		coreCounter);
	float ips = performanceCounters->getIPSOfCore(coreCounter);
	int frequency = performanceCounters->getFreqOfCore(global_core_id);

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


/**
 * @brief Returns vector with frequencies that should be used by each core
 * 
 * @param oldFrequencies 
 * @param activeCores 
 * @return std::vector<int> 
 */
std::vector<int> DVFSxcs::getFrequencies(
        const std::vector<int> &oldFrequencies,
        const std::vector<bool> &activeCores)
{
    cout << "[Scheduler][xcs]: getFrequencies() called " << endl;
    cout << "[Scheduler][xcs]: oldFrequencies: " << endl;
    for(auto i=0; i<coreRows*coreColumns; i++)
        cout << "\t\tc_" << i << "f_" << oldFrequencies.at(i) << endl;


    
    if (throttle())
    {
        std::vector<int> minFrequencies(coreRows * coreColumns, minFrequency);
        cout << "[Scheduler][xcs]: in throttle mode -> return min. \
            frequencies " << endl;
            return minFrequencies;
    }
    else
    {
        std::vector<int> frequencies(coreRows * coreColumns);

        /* loop through all avaiable cores  */
        for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns;
             coreCounter++)
        {
            /* check if current core is active  */
            if (activeCores.at(coreCounter))
            {
                /* set global core id to current core for xcs to access core    */
                global_core_id = coreCounter;

                /* set global frequency to old frequency of current core    */
                global_frequency = oldFrequencies.at(coreCounter);
                cout << "[Scheduler][xcs]: global_frequency = " << global_frequency << endl;

                /* step in xcs classifier of current core   */
                Environment->update_inputs();
                xcs_systems[coreCounter].step(flag_exploration, flag_condensation);
            
                #if 0
                // use same period for upscaling and downscaling as described
                // in "The ondemand governor."
                if (utilization > upThreshold)
                {
                    cout << "[Scheduler][xcs]: utilization > upThreshold";
                    if (frequency == maxFrequency)
                    {
                        cout << " but already at max frequency" << endl;
                    }
                    else
                    {
                        cout << " -> go to max frequency" << endl;
                        frequency = maxFrequency;
                    }
                }
                else if (utilization < downThreshold)
                {
                    cout << "[Scheduler][xcs]: utilization < downThreshold";
                    if (frequency == minFrequency)
                    {
                        cout << " but already at min frequency" << endl;
                    }
                    else
                    {
                        cout << " -> lower frequency" << endl;
                        frequency = frequency * 80 / 100;
                        frequency = (frequency / frequencyStepSize) *
                                    frequencyStepSize; // round
                        if (frequency < minFrequency)
                        {
                            frequency = minFrequency;
                        }
                    }
                }
                frequencies.at(coreCounter) = frequency;
                #endif
            }
            else
            {
                frequencies.at(coreCounter) = minFrequency;
            }
        }
        return frequencies;
    }
}


bool DVFSxcs::throttle()
{
    if (performanceCounters->getPeakTemperature() > dtmCriticalTemperature)
    {
        if (!in_throttle_mode)
        {
            cout << "[Scheduler][xcs-DTM]: detected thermal violation" << endl;
        }
        in_throttle_mode = true;
    }
    else if (performanceCounters->getPeakTemperature() <
             dtmRecoveredTemperature)
    {
        if (in_throttle_mode)
        {
            cout << "[Scheduler][xcs-DTM]: thermal violation ended" << endl;
        }
        in_throttle_mode = false;
    }
    return in_throttle_mode;
}