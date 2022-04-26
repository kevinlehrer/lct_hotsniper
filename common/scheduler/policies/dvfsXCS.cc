#include "dvfsXCS.h"
#include <iomanip>
#include <iostream>

using namespace std;

t_classifier_system* XCS;
t_environment* Environment;

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
    cout << "[Scheduler][xcs] Initializing XCS classifier system" << endl;
    cerr << "\tXCSLIB\tVERSION " << __XCSLIB_VERSION__ << endl;
    cerr << "      \t\tBUILT   " << __DATE__ << endl;
    cerr << "      \t\tTIME    " << __TIME__ << endl;
    cerr << endl;
    cerr << "      \t\tSTATE       " << __INPUTS_VERSION__ << endl;
    cerr << "      \t\tACTION      " << __ACTION_VERSION__ << endl;
    cerr << "      \t\tCONDITIIONS " << __CONDITION_VERSION__ << endl;
    cerr << endl << endl;

    cout << "[Scheduler][xcs] Initializing configuration manager" << endl;
    //! init the configuration manager
    //xcs_config2 = new xcs_config_mgr2();
    //xcs_config2 = xcs_config_mgr2(); 
	//! init random the number generator
	//xcs_random::set_seed(&xcs_config2);

    //cout << "[Scheduler][xcs] Initializing action class" << endl;
	//! init the action class
	//dummy_action = t_action(xcs_config2);
    dummy_action = new t_action(xcs_config2);

    cout << "[Scheduler][xcs] Initializing environment" << endl;
	//! init the environment
	Environment = new t_environment(xcs_config2);

    cout << "[Scheduler][xcs] Initializing condition class" << endl;
	//! init the condition class
    //dummy_condition = t_condition(xcs_config2);
    dummy_condition = new t_condition(xcs_config2);

    cout << "[Scheduler][xcs] Initializing classifier system" << endl;
	//! init the XCS classifier system
	//xcs_classifier = t_classifier_system(xcs_config2);
    XCS = new t_classifier_system(xcs_config2);

    cout << "[Scheduler][xcs] Initializing experiment manager" << endl;
	//! init the experiment manager
	Session = new experiment_mgr(xcs_config2);

    cout << "[Scheduler][xcs] Initializing done..." << endl;
    //Session->perform_experiments();
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
    if (throttle())
    {
        std::vector<int> minFrequencies(coreRows * coreColumns, minFrequency);
        cout << "[Scheduler][ondemand-DTM]: in throttle mode -> return min. \
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
                /* get performance counters for current core    */
                float power = performanceCounters->getPowerOfCore(coreCounter);     // not needed
                float temperature = performanceCounters->getTemperatureOfCore(
                    coreCounter);
                int frequency = oldFrequencies.at(coreCounter);
                float utilization = performanceCounters->getUtilizationOfCore(
                    coreCounter);
                float ips = performanceCounters->getIPSOfCore(coreCounter);

                
                cout << "[Scheduler][xcs]: Core " << setw(2) << coreCounter
                     << ":";
                cout << " P=" << fixed << setprecision(3) << power << " W";
                cout << " f=" << frequency << " MHz";
                cout << " T=" << fixed << setprecision(1) << temperature << " C";
                cout << " IPS=" << fixed << setprecision(3) << ips;
                // avoid the little circle symbol, it is not ASCII
                cout << " utilization=" << fixed << setprecision(3) << utilization
                     << endl;
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