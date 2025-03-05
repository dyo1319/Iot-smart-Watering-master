"state": "64" - This indicates the current operational mode of the system. The value "64" corresponds to MANUAL_MODE as defined in ESP32 (#define MANUAL_MODE 64). The four possible states are:

61: Temperature mode
62: Soil moisture mode
63: Sabbath mode
64: Manual mode


"tempMode": { ... } - This object contains the configuration for the temperature-based watering mode:

"temp": 23.50 - The target temperature in Celsius. If the measured temperature exceeds this value, the system will water for the maxTime duration; otherwise, it will water for the minTime duration.
"minTime": 1 - The watering duration in minutes when the external temperature is lower than or equal to the target temperature.
"maxTime": 3 - The watering duration in minutes when the external temperature is higher than the target temperature (plants need more water in hot conditions).


"SOILMOISTURE": { ... } - Configuration for the soil moisture-based watering mode:

"desiredMoisture": 60 - The target soil moisture level as a percentage (%).
"toleranceRange": 10 - The acceptable deviation from the target moisture level. This creates a range of 50-70% (60% ± 10%) within which the system will not activate watering.


"SABBATH": { ... } - Configuration for the Sabbath mode (scheduled watering based on time, not sensors):

"schedule": [...] - An array of scheduled watering times:

First schedule: {"dayOfWeek": 5, "startHour": 18, "duration": 30} - Water for 30 minutes starting at 6:00 PM on Friday (dayOfWeek 5, where Sunday is 0).
Second schedule: {"dayOfWeek": 6, "startHour": 8, "duration": 20} - Water for 20 minutes starting at 8:00 AM on Saturday (dayOfWeek 6).




"MANUAL": { ... } - Configuration for manual control mode:

"pumpState": false - The current desired state of the water pump (false means OFF).
"lastCommand": 0 - Timestamp of the last command sent to the system, which helps track when the manual command was issued. The value 0 indicates no recent command.



How This Configuration Is Used:

The ESP32 reads this configuration from the server periodically using HTTP requests.
The system first checks the state value to determine which operational mode to use.
Based on the mode, it reads the relevant configuration section and adjusts its behavior accordingly.
In temperature mode, it compares the current temperature with the target and activates the pump for the appropriate duration.
In soil moisture mode, it tries to keep the soil moisture within the specified range.
In Sabbath mode, it follows the predetermined schedule regardless of sensor readings.
In manual mode, it simply obeys the direct commands indicated by pumpState.
