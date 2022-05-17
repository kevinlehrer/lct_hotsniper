/**
* This header implements the ondemand governor with Dynamic Thermal Managment (DTM)
* The ondemand governor implementation is based on
* Pallipadi, Venkatesh, and Alexey Starikovskiy.
* "The ondemand governor."
* Proceedings of the Linux Symposium. Vol. 2. No. 00216. 2006.
*/

#ifndef __DVFS_XCS_H
#define __DVFS_XCS_H

/* system header files      */
#include <vector>

/* hot sniper header files  */
#include "dvfspolicy.h"
#include "performance_counters.h"

/* xcslib header files      */
#include "xcs_definitions.hpp"
#include "xcs_classifier.hpp"
#include "xcs_config_mgr2.hpp"
#include "experiment_mgr.hpp"

class DVFSxcs : public DVFSPolicy {
    public:
        DVFSxcs(
            const PerformanceCounters *performanceCounters,
            int coreRows,
            int coreColumns,
            int minFrequency,
            int maxFrequency,
            int frequencyStepSize,
            float upThreshold,
            float downThreshold,
            float dtmCriticalTemperature,
            float dtmRecoveredTemperature);
            
            experiment_mgr* Session;
            xcs_config_mgr2 xcs_config2;
            t_action* dummy_action;
            t_condition* dummy_condition;

        virtual std::vector<int> getFrequencies(
            const std::vector<int> &oldFrequencies,
            const std::vector<bool> &activeCores);

    private:
        const PerformanceCounters *performanceCounters;
        
        unsigned int coreRows;
        unsigned int coreColumns;
        int minFrequency;
        int maxFrequency;
        int frequencyStepSize;
        float upThreshold;
        float downThreshold;
        float dtmCriticalTemperature;
        float dtmRecoveredTemperature;
        
        bool in_throttle_mode = false;
        bool throttle();

        /**********************************************************************/
        /* XCS specific stuff *************************************************/
        /**********************************************************************/
        bool flag_exploration;
        bool flag_condensation;

        
};

#endif
