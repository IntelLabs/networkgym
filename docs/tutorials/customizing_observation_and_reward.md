---
title: Customizing Observation and Reward
---

# Customizing Observation and Reward

Personalizing the observation and reward functions to match your experimental goals is achievable within the provided environments. Within the `Adapter` class for each environment, both the `get_observation` and `get_reward` functions take a network statistics measurement as input. An illustration of such a network statistics measurement is provided below:

```python
      cid direction end_ts group                    name start_ts          user                                              value
0   Wi-Fi        DL   6000   GMA                 cell_id     5900  [0, 1, 2, 3]                               [0.0, 0.0, 1.0, 1.0]
1     LTE        DL   6000   PHY                 cell_id     5900  [0, 1, 2, 3]                               [1.0, 1.0, 1.0, 1.0]
2     All        DL   6000   GMA  delay_test_1_violation     5900  [0, 1, 2, 3]             [100.0, 100.0, 72.99270072992701, 0.0]
3     All        DL   6000   GMA  delay_test_2_violation     5900  [0, 1, 2, 3]                           [100.0, 100.0, 0.0, 0.0]
4     All        DL   6000   GMA         delay_violation     5900  [0, 1, 2, 3]            [100.0, 100.0, 100.0, 89.8989898989899]
5     All        DL   6000   GMA                 max_owd     5900  [0, 1, 2, 3]                       [499.0, 538.0, 239.0, 195.0]
6     LTE        DL   6000   GMA                 max_owd     5900  [0, 1, 2, 3]                       [499.0, 538.0, 239.0, 195.0]
7   Wi-Fi        DL   6000   GMA                 max_owd     5900  [0, 1, 2, 3]                               [3.0, 2.0, 3.0, 4.0]
8     LTE        DL   6000   PHY                max_rate     5900  [0, 1, 2, 3]                           [35.0, 35.0, 22.0, 35.0]
9   Wi-Fi        DL   6000   PHY                max_rate     5900  [0, 1, 2, 3]                           [78.0, 78.0, 65.0, 78.0]
10    All        DL   6000   GMA          measurement_ok     5900  [0, 1, 2, 3]                               [1.0, 1.0, 1.0, 1.0]
11    All        DL   6000   GMA           missed_action     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
12    All        DL   6000   GMA                     owd     5900  [0, 1, 2, 3]                       [484.0, 517.0, 214.0, 102.0]
13    LTE        DL   6000   GMA                     owd     5900  [0, 1, 2, 3]                       [491.0, 517.0, 217.0, 145.0]
14  Wi-Fi        DL   6000   GMA                     owd     5900  [0, 1, 2, 3]                               [0.0, 1.0, 0.0, 0.0]
15    LTE        DL   6000   GMA                priority     5900  [0, 1, 2, 3]                               [1.0, 1.0, 1.0, 0.0]
16  Wi-Fi        DL   6000   GMA                priority     5900  [0, 1, 2, 3]                               [1.0, 1.0, 1.0, 1.0]
17    All        DL   6000   GMA                qos_rate     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
18    LTE        DL   6000   GMA                qos_rate     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
19  Wi-Fi        DL   6000   GMA                qos_rate     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
20    All        DL   6000   GMA                    rate     5900  [0, 1, 2, 3]                    [13.352, 9.056, 15.912, 45.992]
21    LTE        DL   6000   GMA                    rate     5900  [0, 1, 2, 3]                        [8.472, 8.472, 5.336, 8.24]
22  Wi-Fi        DL   6000   GMA                    rate     5900  [0, 1, 2, 3]                     [6.848, 0.928, 10.336, 15.912]
23    LTE        DL   6000   GMA             split_ratio     5900  [0, 1, 2, 3]                             [17.0, 32.0, 9.0, 3.0]
24  Wi-Fi        DL   6000   GMA             split_ratio     5900  [0, 1, 2, 3]                            [15.0, 0.0, 23.0, 29.0]
25    LTE        DL   6000   GMA           traffic_ratio     5900  [0, 1, 2, 3]                           [55.0, 90.0, 34.0, 34.0]
26  Wi-Fi        DL   6000   GMA           traffic_ratio     5900  [0, 1, 2, 3]                           [45.0, 10.0, 66.0, 66.0]
27    All        DL   6000   GMA                 tx_rate     5900  [0, 1, 2, 3]                     [14.636, 9.757, 14.52, 38.681]
28    All        DL   6000   GMA                   x_loc     5900  [0, 1, 2, 3]  [13.871206061072092, 6.702857783599929, 52.198...
29    All        DL   6000   GMA                   y_loc     5900  [0, 1, 2, 3]  [7.964088643728281, 1.6756194439050196, 9.0212...
```

## Working with the Measurement

### Measurement Columns

The network statistics measurement contains 8 columns, with each field's explanation provided below:

| Column | Description |
| ----- | ---- |
| cid | Connection ID: Wi-Fi, LTE, or All, where All signifies the combination of all links. |
| direction | DL: downlink; UL: uplink. |
| end_ts | The ending timestamp of this measurement. |
| group | GMA: GMA convergence layer measurement; PHY: measurement from the PHY layer. |
| name | Name of the measurement.|
| start_ts | The starting timestamp of this measurement. |
| user | A list of user IDs for this measurement. |
| value | A list of measured values for this measurement (user and value should have the same size). |

### Measurement Rows

Now, we elaborate on each row:

| Row | Name | Description |
| ----- | ---- | ---- |
| 0,1 | cell_id | LTE cell ID and Wi-Fi access point ID as measured by each user. *(NOTE: 255 is the default value, representing no measurement.)*|
| 2,3 | delay_test_*_violation | One-way delay violation percentage (%) *--for testing purpose--* as measured by each user; delay threshold (delay_test_*_thresh_ms) can be configured in the JSON file.|
| 4 | delay_violation | One-way delay violation percentage (%) as measured by each user; delay bound (delay_bound_ms) can be configured in the JSON file. *(NOTE: Only for QoS use case)* |
| 5,6,7 | max_owd | Maximum one-way delay measured by each user, involving LTE link, Wi-Fi link, and ALL. *(ALL: after reordering out-of-order packets from both links)*. |
| 8,9 | max_rate | LTE/Wi-Fi link capacity as measured by each user. |
| 10 | measurement_ok |	1: Measurement is valid; 0: An issue exists with the measurement.|
| 11 | missed_action | Count of missed actions by the environment. *(NOTE: Applicable in testbed environment where RL agent might take too long to compute an action. Simulator waits for action by default in simulations, therefore should always be 0.)* |
| 12,13,14 | owd | One-way delay measured by each user, involving LTE link, Wi-Fi link, and ALL. *(ALL: after reordering out-of-order packets from both links)*. |
| 15,16 | priority | 1: high priority; 0: low priority. When Dynamic Flow Prioritization is enabled, for each cell, mark 70%~90% of traffic or users to high priority. |
| 17,18,19 | qos_rate | QoS delivery rate (output traffic throughput meeting QoS requirement) as measured by each user, comprising traffic over LTE link, Wi-Fi link, and ALL (combined). |
| 20,21,22 | rate | Delivery rate (output traffic throughput) measured by each user, including traffic over LTE link, Wi-Fi link, and ALL (combined).|
| 23,24 | split_ratio | Traffic split ratio (in range of [0, 32]) as configured for each user, covering LTE link, Wi-Fi link. LTE split ratio + Wi-Fi split ratio equals 32. |
| 25,26 | traffic_ratio | Traffic ratio (%) sending to each link measured by each user, involving LTE link, Wi-Fi link. traffic_ratio might not equal split_ratio, since it takes time for the system to apply the new configuration. |
| 27 | tx_rate | Load (input traffic throughput) measured by each user. |
| 28 | x_loc | x coordinate per user. |
| 29 | y_loc | y coordinate per user. |


### Retrieving a Measurement

To retrieve a specific network statistic, such as `rate`, over a specifi link, such as `All`, you can utilize the following code:
```python
 for index, row in df.iterrows():
            if row['name'] == 'rate':
                if row['cid'] == 'All':
                    df_rate = row
```

Optionally, employ the `explode` function to transform the row object into the dataframe structure:

```python
df_rate = df_rate.explode(column=['user', 'value']) #keep the flow rate.
```

### Filling the Missing Data
```{eval-rst}
Occasionally, certain users might lack measurements for some network metrics. For instance, if data is transmitted over an LTE link, the measurement for the Wi-Fi link might be absent. To address this scenario, we offer the :meth:`network_gym_client.Adapter.fill_empty_feature` function, which serves to populate the missing data.
```

```{tip}
When you make updates to the `get_observation` function, ensure that you also modify the `get_observation_space` function to maintain consistency in the number of features.
```
