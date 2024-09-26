/**
 * Copyright (c) 2019, Bosch Engineering Center Cluj and BFMC organizers
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.

 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.

 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/

#include "brain/powermanager.hpp"

#define miliseconds_in_h 3600000
#define display_interval_miliseconds 30000
#define battery_mAmps 5500
#define battery_cells 2
#define battery_maxVoltage (4200 * battery_cells)
#define battery_shutdownVoltage 7100
#define battery_shutdownWarning 7200
#define maxPercent 100
#define zeroPercent 0
#define one_byte 256
#define scale_factor 1000

// TODO: Add your code here
namespace brain
{
   /**
    * @brief Class constructorpowermanager
    *
    */
    CPowermanager::CPowermanager(
        std::chrono::milliseconds f_period,
        brain::CKlmanager& f_CKlmanager,
        UnbufferedSerial& f_serial,
        periodics::CTotalVoltage& f_totalVoltage,
        periodics::CInstantConsumption& f_instantConsumption
    )
    : utils::CTask(f_period)
    , m_CKlmanager(f_CKlmanager)
    , m_serial(f_serial)
    , m_totalVoltage(f_totalVoltage)
    , m_instantConsumption(f_instantConsumption)
    , m_period(f_period.count())
    {
        /* constructor behaviour */
    }

    /** @brief  CPowermanager class destructor
     */
    CPowermanager::~CPowermanager()
    {
    };

    void CPowermanager::_run()
    {
        char buffer[one_byte];
        uint16_t battery_mAmps_var;

        m_totalVoltage.void_TotalSafetyMeasure();
        m_instantConsumption.void_InstantSafetyMeasure(m_period);

        if(uint16_globalsV_battery_mAmps_user != 0.0) battery_mAmps_var = uint16_globalsV_battery_mAmps_user;
        else battery_mAmps_var = battery_mAmps;

        int mAmps_actual = (int_globalsV_battery_totalVoltage * battery_mAmps_var) / battery_maxVoltage;

        uint32_t temp_average = uint32_globalsV_consumption_Total_mAmpsH / uint32_globalsV_numberOfMiliseconds_Total;

        int_globalsV_instantConsumption_Avg_Total_mAmpsH = (uint16_t)temp_average;

        uint32_globalsV_range_left_shutdown = (((mAmps_actual - (battery_shutdownVoltage*battery_mAmps_var/battery_maxVoltage)))/(uint16_t)currentEMA)*3600;

        bool_globalsV_ShuttedDown = false;

        return;

        if(battery_shutdownVoltage < int_globalsV_battery_totalVoltage && int_globalsV_battery_totalVoltage <= battery_shutdownWarning)
        {
            if((uint32_globalsV_numberOfMiliseconds_Total%display_interval_miliseconds == 0) && (uint32_globalsV_numberOfMiliseconds_Total > 0))
            {
                if(!bool_globalsV_warningFlag)
                {
                    int h = uint32_globalsV_range_left_shutdown/3600;

                    int m = (uint32_globalsV_range_left_shutdown%3600)/60;

                    int s = (uint32_globalsV_range_left_shutdown%3600)%60;

                    snprintf(buffer, sizeof(buffer), "@warning:%dH;%dM;%dS;;\r\n", h, m, s);
                    m_serial.write(buffer, strlen(buffer));

                    bool_globalsV_warningFlag = !bool_globalsV_warningFlag;
                }
            }
            else
            {
                bool_globalsV_warningFlag = !bool_globalsV_warningFlag;
            }   
        }
        else if (int_globalsV_battery_totalVoltage <= battery_shutdownVoltage)
        {
            if(uint32_globalsV_numberOfMiliseconds_Total%display_interval_miliseconds == 0)
            {
                snprintf(buffer, sizeof(buffer), "@shutdown:ack");
                m_serial.write(buffer, strlen(buffer));

                char buffer2[one_byte];

                if(int_globalsV_value_of_kl!=0)
                {
                    m_CKlmanager.serialCallbackKLCommand("0",buffer2);
                }
                
                ThisThread::sleep_for(chrono::milliseconds(200));
                bool_globalsV_ShuttedDown = true;
                int_globalsV_value_of_kl = 0;
            }
        }
    }

}; // namespace periodics