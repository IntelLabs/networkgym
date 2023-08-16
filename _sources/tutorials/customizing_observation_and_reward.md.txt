---
title: Customizing Observation and Reward
---

# Customizing Observation and Reward

Personalizing the observation and reward functions to match your experimental goals is achievable within the provided environments. Within the `Adapter` class for each environment, both the `get_observation` and `get_reward` functions take a network statistics measurement as input. An illustration of such a network statistics measurement is provided below:

```python
      cid direction end_ts group             name start_ts          user                                              value
0     LTE        DL   6000   GMA            ap_id     5900  [0, 1, 2, 3]                       [255.0, 255.0, 255.0, 255.0]
1   Wi-Fi        DL   6000   GMA            ap_id     5900  [0, 1, 2, 3]                               [0.0, 0.0, 1.0, 1.0]
2     All        DL   6000   GMA  delay_violation     5900  [0, 1, 2, 3]                       [100.0, 100.0, 100.0, 100.0]
3     All        DL   6000   GMA          max_owd     5900  [0, 1, 2, 3]                       [335.0, 703.0, 422.0, 299.0]
4     LTE        DL   6000   GMA          max_owd     5900  [0, 1, 2, 3]                       [335.0, 703.0, 422.0, 299.0]
5   Wi-Fi        DL   6000   GMA          max_owd     5900  [0, 1, 2, 3]                               [0.0, 1.0, 1.0, 0.0]
6     LTE        DL   6000   PHY         max_rate     5900  [0, 1, 2, 3]                           [35.0, 35.0, 22.0, 35.0]
7   Wi-Fi        DL   6000   PHY         max_rate     5900  [0, 1, 2, 3]                           [78.0, 78.0, 65.0, 78.0]
8     All        DL   6000   GMA   measurement_ok     5900  [0, 1, 2, 3]                               [1.0, 1.0, 1.0, 1.0]
9     All        DL   6000   GMA    missed_action     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
10    All        DL   6000   GMA              owd     5900  [0, 1, 2, 3]                       [327.0, 680.0, 403.0, 279.0]
11    LTE        DL   6000   GMA              owd     5900  [0, 1, 2, 3]                       [327.0, 680.0, 404.0, 280.0]
12  Wi-Fi        DL   6000   GMA              owd     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
13    LTE        DL   6000   GMA      qos_marking     5900  [0, 1, 2, 3]                               [1.0, 1.0, 1.0, 0.0]
14  Wi-Fi        DL   6000   GMA      qos_marking     5900  [0, 1, 2, 3]                               [1.0, 1.0, 1.0, 0.0]
15    All        DL   6000   GMA         qos_rate     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
16    LTE        DL   6000   GMA         qos_rate     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
17  Wi-Fi        DL   6000   GMA         qos_rate     5900  [0, 1, 2, 3]                               [0.0, 0.0, 0.0, 0.0]
18    All        DL   6000   GMA             rate     5900  [0, 1, 2, 3]                       [15.912, 9.056, 7.2, 14.984]
19    LTE        DL   6000   GMA             rate     5900  [0, 1, 2, 3]                       [8.472, 8.472, 5.336, 8.472]
20  Wi-Fi        DL   6000   GMA             rate     5900  [0, 1, 2, 3]                        [0.696, 4.64, 3.016, 4.992]
21    LTE        DL   6000   GMA      split_ratio     5900  [0, 1, 2, 3]                           [31.0, 16.0, 19.0, 22.0]
22  Wi-Fi        DL   6000   GMA      split_ratio     5900  [0, 1, 2, 3]                            [1.0, 16.0, 13.0, 10.0]
23    LTE        DL   6000   GMA    traffic_ratio     5900  [0, 1, 2, 3]                           [92.0, 65.0, 64.0, 63.0]
24  Wi-Fi        DL   6000   GMA    traffic_ratio     5900  [0, 1, 2, 3]                            [8.0, 35.0, 36.0, 37.0]
25    All        DL   6000   GMA          tx_rate     5900  [0, 1, 2, 3]                     [17.075, 9.408, 7.434, 15.797]
26    All        DL   6000   GMA            x_loc     5900  [0, 1, 2, 3]  [12.040140997999378, 6.710302486045565, 52.547...
27    All        DL   6000   GMA            y_loc     5900  [0, 1, 2, 3]  [6.167369006263322, 2.450729590703729, 9.45877...
```

## Working with the DataFrame

### Columns

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

### Rows

Now, we elaborate on each row:

| Row | Name | Description |
| ----- | ---- | ---- |
| 0,1 | ap_id | LTE cell ID and Wi-Fi access point ID as measured by each user. *(NOTE: 255 is the default value, representing no measurement.)*|
| 2 | delay_violation | One-way delay violation percentage (%) as measured by each user; delay bound (delay_bound_ms) can be configured in the JSON file. *(NOTE: Only for QoS use case)* |
| 3,4,5 | max_owd | Maximum one-way delay measured by each user, involving LTE link, Wi-Fi link, and ALL. *(ALL: after reordering out-of-order packets from both links)*. |
| 6,7 | max_rate | LTE/Wi-Fi link capacity as measured by each user. |
| 8 | measurement_ok |	1: Measurement is valid; 0: An issue exists with the measurement.|
| 9 | missed_action | Count of missed actions by the environment. *(NOTE: Applicable in testbed environment where RL agent might take too long to compute an action. Simulator waits for action by default in simulations, therefore should always be 0.)* |
| 10,11,12 | owd | One-way delay measured by each user, involving LTE link, Wi-Fi link, and ALL. *(ALL: after reordering out-of-order packets from both links)*. |
| 13,14 | qos_marking | 1: QoS user; 0: Non-QoS user.|
| 15,16,17 | qos_rate | QoS delivery rate (output traffic throughput meeting QoS requirement) as measured by each user, comprising traffic over LTE link, Wi-Fi link, and ALL (combined). |
| 18,19,20 | rate | Delivery rate (output traffic throughput) measured by each user, including traffic over LTE link, Wi-Fi link, and ALL (combined).|
| 21,22 | split_ratio | Traffic split ratio (in range of [0, 32]) as measured by each user, covering LTE link, Wi-Fi link. LTE split ratio + Wi-Fi split ratio equals 32. |
| 23,34 | traffic_ratio | Traffic ratio (%) sending to each link measured by each user, involving LTE link, Wi-Fi link. traffic_ratio might not equal split_ratio, since the system takes sine time to apply the new action. |
| 25 | tx_rate | Load (input traffic throughput) measured by each user. |
| 26 | x_loc | x coordinate per user. |
| 27 | y_loc | y coordinate per user. |


### Retrieving a Measurement

To retrieve a specific network statistic, such as `rate`, you can utilize the following code:
```python
df_rate = df[df['name'] == 'rate'].reset_index(drop=True) # get the rate
```

For isolating the rate measurement to a specific link, such as `All`, you can execute the subsequent code:
```python
df_rate = df_rate[df_rate['cid'] == 'All'].reset_index(drop=True) # get the rate for the all link combined
```

Lastly, employ the `explode` function to transform the nested data frame into its original structure:

```python
df_rate = df_rate.explode(column=['user', 'value']) #keep the flow rate.
```
This is the final result of the `rate` for each user, considering the combination of `All` links:
```python
   cid direction end_ts group  name start_ts user   value
0  All        DL   6000   GMA  rate     5900    0   13.24
0  All        DL   6000   GMA  rate     5900    1  18.464
0  All        DL   6000   GMA  rate     5900    2   6.968
0  All        DL   6000   GMA  rate     5900    3   19.28
```

### Filling the Missing Data
```{eval-rst}
Occasionally, certain users might lack measurements for some network metrics. For instance, if data is transmitted over an LTE link, the measurement for the Wi-Fi link might be absent. To address this scenario, we offer the :meth:`network_gym_client.Adapter.fill_empty_feature` function, which serves to populate the missing data.
```

```{tip}
When you make updates to the `get_observation` function, ensure that you also modify the `get_observation_space` function to maintain consistency in the number of features.
```