---
title: Working with Measurements
---

# Working with Measurements

Personalizing the observation and reward functions to match your experimental goals is achievable within the provided environments. Within the `Adapter` class for each environment, both the `get_observation` and `get_reward` functions take a network statistics measurement as input. An illustration of such a network statistics measurement is provided below:

```
             id      name  source    ts                             value
0  [1, 2, 3, 4]  dl::rate     gma  6000  [11.616, 11.616, 16.024, 57.144]
```

## Measurement Columns

The network statistics measurement contains 5 columns, with each field's explanation provided below:

| Column | Description |
| ----- | ---- |
| id | Represents the unique identifier for the measurement entity list. By default, it corresponds to a user ID. However, it could the refers a cell ID or flow ID in some measurements. For instance, `dl::max_rate` measures the maximum rate per user, whereas `dl:cell:::max_rate` measures the maximum rate per cell.|
| name | Denotes the name of the measurement. The naming convention includes descriptive tags that can be prefixed to the name and separated by `::`. For example, a measurement capturing traffic ratio over an LTE link and downlink direction might be represented as `lte::dl::traffic_ratio`.|
| source | Indicates the origin providing the measurement data, such as lte.|
| ts | Represents the timestamp indicating when the measurement was recorded.|
| value | Stores the value of the measurement. The value is stored as a list, and its size matches that of the corresponding `id`. Elements can be numerical, or JSON objects to store more complex data structure. |


## Supported Measurements by Source
Click a source to view the available measurements. A measurement can be identified as `source`::`name`, e.g., `gma::ul::rate`.

::::{tab-set}

:::{tab-item} gma
### Measurements from Generic Multi-Accee (GMA). 

🔽 user measurement
| Name | Description |
| ---- | ---- |
| ul::missed_action | Count of missed agent actions in uplink by the environment. *(NOTE: Applicable in testbed environment where RL agent might take too long to compute an action. Simulator waits for action by default in simulations, therefore should always be 0.)* |
| dl::missed_action | Count of missed agent actions in downlink by the environment. *(NOTE: Applicable in testbed environment where RL agent might take too long to compute an action. Simulator waits for action by default in simulations, therefore should always be 0.)* |
| ul::measurement_ok |	1: Uplink measurement is valid; 0: An issue exists with the uplink measurement.|
| dl::measurement_ok |  1: Downlink measurement is valid; 0: An issue exists with the downlink measurement.|
| ul::rate | Uplink delivery rate (output traffic throughput) measured by each user in Mbps.|
| dl::rate | Downlink delivery rate (output traffic throughput) measured by each user in Mbps.|
| ul::tx_rate | Uplink load (input traffic throughput) measured by each user in Mbps. |
| dl::tx_rate | Downlink load (input traffic throughput) measured by each user in Mbps. |
| wifi::ul::rate | WiFi uplink delivery rate (output traffic throughput) measured by each user in Mbps.|
| wifi::dl::rate | WiFi downlink delivery rate (output traffic throughput) measured by each user in Mbps.|
| lte::ul::rate | LTE uplink delivery rate (output traffic throughput) measured by each user in Mbps.|
| lte::dl::rate | LTE downlink delivery rate (output traffic throughput) measured by each user in Mbps.|
| ul::qos_rate | Uplink QoS delivery rate (output traffic throughput meeting QoS requirement) as measured by each user in Mbps. |
| dl::qos_rate | QoS delivery rate (output traffic throughput meeting QoS requirement) as measured by each user in Mbps. |
| wifi::ul::qos_rate | WiFi uplink QoS delivery rate (output traffic throughput meeting QoS requirement) as measured by each user in Mbps. |
| wifi::dl::qos_rate | WiFi downlink QoS delivery rate (output traffic throughput meeting QoS requirement) as measured by each user in Mbps. |
| lte::ul::qos_rate | LTE uplink QoS delivery rate (output traffic throughput meeting QoS requirement) as measured by each user in Mbps. |
| lte::dl::qos_rate | LTE downlink QoS delivery rate (output traffic throughput meeting QoS requirement) as measured by each user in Mbps. |
| ul::delay_violation | Uplink one-way delay violation percentage (%) as measured by each user; delay bound (delay_bound_ms) can be configured in the JSON file. *(NOTE: Only for QoS use case)* |
| dl::delay_violation | Downlink one-way delay violation percentage (%) as measured by each user; delay bound (delay_bound_ms) can be configured in the JSON file. *(NOTE: Only for QoS use case)* |
| ul::delay_test_1_violation | Uplink one-way delay violation percentage (%) *--for testing purpose--* as measured by each user; delay threshold (delay_test_1_thresh_ms) can be configured in the JSON file.|
| dl::delay_test_1_violation | Downlink one-way delay violation percentage (%) *--for testing purpose--* as measured by each user; delay threshold (delay_test_1_thresh_ms) can be configured in the JSON file.|
| ul::delay_test_2_violation | Uplink one-way delay violation percentage (%) *--for testing purpose--* as measured by each user; delay threshold (delay_test_2_thresh_ms) can be configured in the JSON file.|
| dl::delay_test_2_violation | Downlink one-way delay violation percentage (%) *--for testing purpose--* as measured by each user; delay threshold (delay_test_2_thresh_ms) can be configured in the JSON file.|
| ul::owd | Uplink one-way delay measured by each user in ms. *(after reordering out-of-order packets from all links)*. |
| dl::owd | Downlink one-way delay measured by each user in ms. *(after reordering out-of-order packets from all links)*. |
| wifi::ul::owd | WiFi uplink one-way delay measured by each user in ms. |
| wifi::dl::owd | WiFi downlink one-way delay measured by each user in ms. |
| lte::ul::owd | LTE uplink one-way delay measured by each user in ms. |
| lte::dl::owd | LTE downlink one-way delay measured by each user in ms. |
| ul::max_owd | Uplink maximum one-way delay measured by each user in ms. *(after reordering out-of-order packets from all links)*. |
| dl::max_owd | Downlink maximum one-way delay measured by each user in ms. *(after reordering out-of-order packets from all links)*. |
| wifi::ul::max_owd | WiFi uplink maximum one-way delay measured by each user in ms. |
| wifi::dl::max_owd | WiFi downlink maximum one-way delay measured by each user in ms.  |
| lte::ul::max_owd | LTE uplink maximum one-way delay measured by each user in ms. |
| lte::dl::max_owd | LTE downlink maximum one-way delay measured by each user in ms.  |
| wifi::ul::priority | WiFi uplink user priority. 1: high priority; 0: low priority. When Dynamic Flow Prioritization is enabled, for each cell, mark 70%~90% of traffic or users to high priority. |
| wifi::dl::priority | WiFi downlink user priority. 1: high priority; 0: low priority. When Dynamic Flow Prioritization is enabled, for each cell, mark 70%~90% of traffic or users to high priority. |
| lte::ul::priority | LTE uplink user priority. 1: high priority; 0: low priority. When Dynamic Flow Prioritization is enabled, for each cell, mark 70%~90% of traffic or users to high priority. |
| lte::dl::priority | LTE downlink user priority. 1: high priority; 0: low priority. When Dynamic Flow Prioritization is enabled, for each cell, mark 70%~90% of traffic or users to high priority. |
| wifi::ul::traffic_ratio | Traffic ratio (%) sending to WiFi uplink measured by each user. traffic_ratio might not equal split_ratio, since it takes time for the system to apply the new configuration. |
| wifi::dl::traffic_ratio | Traffic ratio (%) sending to WiFi downlink measured by each user. traffic_ratio might not equal split_ratio, since it takes time for the system to apply the new configuration. |
| lte::ul::traffic_ratio | Traffic ratio (%) sending to LTE uplink measured by each user. traffic_ratio might not equal split_ratio, since it takes time for the system to apply the new configuration. |
| lte::dl::traffic_ratio | Traffic ratio (%) sending to LTE downlink measured by each user. traffic_ratio might not equal split_ratio, since it takes time for the system to apply the new configuration. |
| wifi::ul::split_ratio | Traffic split ratio (%) for WiFi uplink as configured for each user. |
| wifi::dl::split_ratio | Traffic split ratio (%) for WiFi downlink as configured for each user. |
| lte::ul::split_ratio | Traffic split ratio (%) for LTE uplink as configured for each user. |
| lte::dl::split_ratio | Traffic split ratio (%) for LTE downlink as configured for each user. |
| x_loc | x coordinate per user in meters. |
| y_loc | y coordinate per user in meters. |

🔽 base station (cell) measurement
| Name | Description |
| ---- | ---- |
| ul::cell::rate | Uplink delivery rate (output traffic throughput) per slice as measured by each base station in Mbps.|
| ul::cell::qos_rate | Uplink QoS delivery rate (output traffic throughput meeting QoS requirement) per slice as measured by each base station in Mbps. |
| ul::cell::tx_rate | Uplink load (input traffic throughput) per slice as measured by each base station in Mbps. |
| ul::cell::delay_violation | Uplink one-way delay violation percentage (%) as measured by each base station; delay bound (delay_bound_ms) can be configured in the JSON file. *(NOTE: Only for QoS use case)* |
| dl::cell::rate | Downlink delivery rate (output traffic throughput) per slice as measured by each base station in Mbps.|
| dl::cell::qos_rate | Downlink QoS delivery rate (output traffic throughput meeting QoS requirement) per slice as measured by each base station in Mbps. |
| dl::cell::tx_rate | Downlink load (input traffic throughput) per slice as measured by each base station in Mbps. |
| dl::cell::delay_violation | Downlink one-way delay violation percentage (%) as measured by each base station; delay bound (delay_bound_ms) can be configured in the JSON file. *(NOTE: Only for QoS use case)* |

:::

:::{tab-item} lte
### Measurements from LTE.

🔽 user measurement
| Name | Description |
| ---- | ---- |
| cell_id | LTE cell ID as measured by each user. *(NOTE: 255 is the default value, representing no measurement.)*|
| slice_id | LTE slice ID as measured by each user. |
| dl::max_rate | LTE downlink link capacity as measured by each user in Mbps. |
| dl::rb_usage | LTE downlink link resource block usage/utilization (%) as measured by each user. |

🔽 base station (cell) measurement
| Name | Description |
| ---- | ---- |
| dl::cell::max_rate | LTE downlink link capacity as measured by the base station for each slice in Mbps. *(NOTE: if a slice includes multiple users, the link capacity is computed as the sum of the per user capacity divided by the number of users.)*|
| dl::cell::rb_usage | LTE downlink link resource block usage/utilization (%) as measured by the base staion for each slice. *(NOTE: if a slice includes multiple users, the usage is computed as the sum of the per user usage.)*|:::

:::

:::{tab-item} wifi
### Measurements from WiFi.

🔽 user measurement
| Name | Description |
| ---- | ---- |
| cell_id | LTE cell/Access Point ID as measured by each user. *(NOTE: 255 is the default value, representing no measurement.)*|
| dl::max_rate | WiFi downlink link capacity as measured by each user in Mbps. |
| ul::max_rate | WiFi uplink link capacity as measured by each user in Mbps. |

:::

:::{tab-item} rmcat
### Measurements from RTP Media Congestion Avoidance Techniques (RMCAT).

🔽 flow measurement
| Name | Description |
| ---- | ---- |
| loglen, | packet history size. |
| qdel | queuing delay in ms. |
| rtt | round trip time in ms. |
| ploss | packet loss count in last 500 ms. |
| plr | packet loss ratio. |
| xcurr | aggregated congestion signal that accounts for queuing delay, ECN. |
| rrate | current receive rate in bps. |
| srate | current estimated available bandwidth in bps. *(If no action is provided, srate uses NADA's algorithm. Otherwise, srate is the action from the RL agent.)*|
| nada_srate | current estimated available bandwidth in bps using NADA's algorithm, it is not used for adjusting sending rate. *(sending rate calculate using NADA's algorithm even the RL agent inputs an action. This can be used as an expert policy for RL agent)*|
| avgint | average inter-loss interval in packets. |
| curint | most recent (currently growing) inter-loss interval in packets. |

:::

::::


## Retrieving a Measurement

To subscribe to a specific measurement, add its `source`::`name` combination to the "subscribed_network_stats" list. For example, appending `gma::dl::rate` to the list would enable receiving measurements with the name `dl::rate` from the source `gma`. After subscription, in the `get_observation` or `get_reward` function, you can utilize the following code to retrieve it:
```python
for index, row in df.iterrows():
    if row['source'] == 'gma':
        if row['name'] == 'dl::rate':
            df_rate = row
            print(df_rate)
```