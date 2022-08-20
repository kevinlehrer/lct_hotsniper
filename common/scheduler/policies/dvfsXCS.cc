#include "dvfsXCS.h"
#include <iomanip>
#include <iostream>
#include <vector>
#include <fstream>

using namespace std;

t_classifier_system* XCS;
t_environment* Environment;
int global_core_id;         // temporary stores a core id for the current core in control loop
float global_frequency;     // temporary stores old frequency for current core in control loop
int global_delta_frequency; // unit to adjust frequency

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
    
    //! set initialized to false - set true once getFrequencies is called
    initialized = false;

    # if 0
	int coreCounter = global_core_id;
	/* get performance counters for current core    */
	float power = performanceCounters->getPowerOfCore(coreCounter);     // not needed
	float temperature = performanceCounters->getTemperatureOfCore(
		coreCounter);
	//int frequency = oldFrequencies.at(coreCounter);
	float utilization = performanceCounters->getUtilizationOfCore(
		coreCounter);ſ
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

    //! set xcs action flag to true
    //xcs_perform_action = true;

    //! initialize xcs classifier systems
    for(auto i=0; i< coreRows * coreColumns; i++)
    {
        xcs_perform_action.push_back(true);
    }

    //! initialize frequencies to min frequency
    for(auto i=0; i< coreRows * coreColumns; i++)
    {
        frequencies.push_back(minFrequency);
    }

    //! create files for tracing reward
    stringstream sstm;
    system("mkdir xcs_trace");
    for (int i=0; i<(coreRows * coreColumns); i++)
    {
        ofstream trace;
        sstm.str("");
        sstm << "./xcs_trace/xcs_trace_core" << i << ".log";
        trace.open(sstm.str());
        //trace.open(sstm.str(), std::ofstream::out | std::ofstream::trunc);
        traceFile.push_back(std::move(trace));
    }

    traceFile[0].flush();
    traceTest.open("traceTest.log");
    traceTest << "just another stupid shit";
    //traceTest.close();

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
    if (!initialized)
    {
        std::vector<int> minFrequencies(coreRows * coreColumns, minFrequency);
        cout << "[Scheduler][xcs]: system initialized with min frequency " << endl;
        initialized = true;
        return minFrequencies;
    }
    else
    {
        //cout << "[Scheduler][xcs]: calling xcs " << endl;
        //std::vector<int> frequencies(coreRows * coreColumns);
        
        for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns;
                coreCounter++)
        {
            /* check if current core is active  */
            if (activeCores.at(coreCounter))
            {
                /* set global core id to current core for xcs to access core    */
                global_core_id = coreCounter;

                /* set global frequency to old frequency of current core        */
                global_frequency = oldFrequencies.at(coreCounter);
                global_delta_frequency = 0;

                /* step in xcs classifier of current core                       */
                Environment->update_inputs();
                xcs_systems[coreCounter].step(flag_exploration, flag_condensation, xcs_perform_action[coreCounter]);

                /* trace reward for debugging                                   */
                Environment->trace(traceFile[coreCounter]);
                traceFile[coreCounter].flush();
                
                /* set new frequency for current core -> action of xcs system   */
                if(xcs_perform_action[coreCounter])
                {
                    frequencies.at(coreCounter) = global_frequency;
                    printf("f_core=%i / f_global=%f\n", frequencies.at(coreCounter), global_frequency);
                }
                else
                {
                    // if don't perform action -> keep frequency that is allready stored
                    // in global frequencies vector
                    //frequencies.at(coreCounter) = frequencies.at(coreCounter);
                    printf("f_core=%i\n", frequencies.at(coreCounter));
                }
                    
                //cout << "[Scheduler][xcs]: global_frequency = " << global_frequency << endl;
            }
            else
            {
                frequencies.at(coreCounter) = minFrequency;
            }
            xcs_perform_action[coreCounter] = !xcs_perform_action[coreCounter];
        }

        #if 0
        if(xcs_perform_action) // change frequencies according to xcs
        {
            //cout << "[xcs]: perform action" << endl;
            /* loop through all avaiable cores  */
            for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns;
                coreCounter++)
            {
                /* check if current core is active  */
                if (activeCores.at(coreCounter))
                {
                    /* set global core id to current core for xcs to access core    */
                    global_core_id = coreCounter;

                    /* set global frequency to old frequency of current core        */
                    global_frequency = oldFrequencies.at(coreCounter);
                    global_delta_frequency = 0;

                    /* step in xcs classifier of current core                       */
                    Environment->update_inputs();
                    xcs_systems[coreCounter].step(flag_exploration, flag_condensation,xcs_perform_action);

                    /* set new frequency for current core -> action of xcs system   */
                    frequencies.at(coreCounter) = global_frequency;
                    //cout << "[Scheduler][xcs]: global_frequency = " << global_frequency << endl;
                }
                else
                {
                    frequencies.at(coreCounter) = minFrequency;
                }
            }
            xcs_perform_action = false;
        }
        else // keep frequencies and only update reward and learning parameters
        {   
            //cout << "[xcs]: perform learning stuff" << endl;
            /* loop through all avaiable cores  */
            for (unsigned int coreCounter = 0; coreCounter < coreRows * coreColumns;
                coreCounter++)
            {
                /* check if current core is active  */
                if (activeCores.at(coreCounter))
                {
                    /* set global core id to current core for xcs to access core    */
                    global_core_id = coreCounter;

                    /* set global frequency to old frequency of current core        */
                    global_frequency = oldFrequencies.at(coreCounter);
                    global_delta_frequency = 0;
                    //cout << "[Scheduler][xcs]: global_frequency = " << global_frequency << endl;
                    
                    /* step in xcs classifier of current core                       */
                    Environment->update_inputs();
                    xcs_systems[coreCounter].step(flag_exploration, flag_condensation, xcs_perform_action);

                    /* trace reward for debugging                                   */
                    Environment->trace(traceFile[coreCounter]);
                    traceFile[coreCounter].flush();
                    /* set new frequency for current core -> action of xcs system   */
                    frequencies.at(coreCounter) = global_frequency + global_delta_frequency;
                }
                else
                {
                    frequencies.at(coreCounter) = minFrequency;
                }
            }
            xcs_perform_action = true;
        }
        #endif
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