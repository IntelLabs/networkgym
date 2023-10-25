---
title: Cellular Network Slicing
---

(cards-mx-cellular-slicing)=
# Cellular Network Slicing

```{figure} cellular_slicing.png
---
width: 100%
---
```
[config.json]: https://github.com/IntelLabs/networkgym/blob/main/network_gym_client/envs/network_slicing/config.json

In the Cellular Network Slicing environment, the entire network is partitioned into multiple slices, each tailored to serve distinct user groups. Utilizing its perceptive observations, the agent possesses the ability to allocate network resources, like resource blocks, to each slice with great efficiency.

Within each slice, the agent can designate resources as dedicated, prioritized, or shared as needed. Meanwhile, for users belonging to the same slice, the Cellular MAC scheduler implements the proportional fair algorithm, guaranteeing an equitable and impartial distribution of resources among them.

| | |
| ----- | ---- |
| Arguments | [config.json]  |
| Observation Space |  `Box(0.0, Inf, (5, N,), float32)` |
| Action Space | `Box(0.0, 1.0, (N,), float32)`  |
| Select Environment | `config_json = load_config_file('network_slicing')` <br> `env = NetworkGymEnv(client_id, config_json)`  |

## Description

In the Network Slicing environment, resource scheduling poses as a challenge, with multiple slices competing for the network's finite resources. The primary objective of resource scheduling is to strategically allocate these resources among slices using dedicated, prioritized, and shared resources, all while striving to meet the service level agreements for each slice.

## Prerequisite

Ensure that you have access to the NetworkGym Server on [vLab](https://registration.intel-research.net/) machines and have downloaded the [NetworkGym](https://github.com/IntelLabs/networkgym).

## Arguments

The arguements for this environment can be customized in the [config.json] file. When the client initiates, the JSON file is loaded and sent to the server to set up the environment.

:::{dropdown} Click to view configuration suggestions for arguments.

```json
{
  //never use negative value for any configure vale!!!

  "rl_config":{
    "agent": "",//name of the agent
    "reward_type" : "",//reward function
  },

  "env_config":{
    "type": "env-start", //do not change
    "subscribed_network_stats":[], //the environment will only report the subscribed measurements.
    "steps_per_episode": 10, //the number of steps for each episode. Episode end is indicated by truncated signal.
    "episodes_per_session": 2, //the number of episodes per environment session. Environment session end is indicated by terminated signal.
    "random_seed": 2, //change the random seed for this simulation run
    "downlink_traffic": true, //set to true to simulate downlink data flow, set to false to simulate uplink data flow.
    "max_wait_time_for_action_ms": -1, //the max time the network gym worker will wait for an action. set to -1 will cap the wait time to 600*1000 milliseconds.
    "enb_locations":{//x, y and z locations of the base station, we support 1 base station only
      "x":40,
      "y":0,
      "z":3
    },
    "ap_locations":[//x, y and z location of the Wi-Fi access point, add or remove element in this list to increase or reduce AP number. We support 0 AP as well.
    ],
    "slice_list":[ //network slicing environment only, resouce block group (rbg) size maybe 1, 2, 3 or 4, it depends on the resource block num, see table 7.1.6.1-1 of 36.213
      {"num_users":2,"dedicated_rbg":0,"prioritized_rbg":13,"shared_rbg":25},
      {"num_users":3,"dedicated_rbg":0,"prioritized_rbg":12,"shared_rbg":25}
    ],
    "user_random_walk":{ // configure random walk model with Distance mode. https://www.nsnam.org/docs/release/3.16/doxygen/classns3_1_1_random_walk2d_mobility_model.html
      "min_speed_m/s": 1, //A random variable used to pick the min random walk speed (m/s). Set min and max speed to 0 to disable random walk
      "max_speed_m/s": 2, //A random variable used to pick the max random walk speed (m/s). Set min and max speed to 0 to disable random walk
      "min_direction_gradients": 0.0, //A random variable used to pick the min random walk direction (gradients). [Min=0.0|Max=6.283184]
      "max_direction_gradients": 6.283184, //A random variable used to pick the max random walk direction (gradients). [Min=0.0|Max=6.283184]
      "distance_m": 3 //change current direction and speed after moving for this distance (m)
    },
    "user_location_range":{//initially, users will be randomly deployed within this x, y range. if user_random_walk_max_speed_m/s > 0, the user will random walk within this boundary.
      "min_x":0,
      "max_x":80,
      "min_y":0,
      "max_y":10,
      "z":1.5
    },
    "measurement_start_time_ms": 1000, //the first measurement start time. The first measurement will be sent to the agent between [measurement_start_time_ms, measurement_start_time_ms + measurement_interval_ms].
    "transport_protocol": "tcp", //"tcp" or "udp"
    "udp_poisson_arrival": false, // if "transport_protocol" is "udp", this para controls whether the generater using poisson process.
    "min_udp_rate_per_user_mbps": 2, // if "transport_protocol" is "udp", this para controls the min sending rate.
    "max_udp_rate_per_user_mbps": 2, // if "transport_protocol" is "udp", this para controls the max sending rate.
    "qos_requirement": {//only for qos_steer environment
      "delay_bound_ms": 100,//max delay for qos flow
      "delay_test_1_thresh_ms": 200, //only for delay violation test 1 measurement, this para does not impact flow qos.
      "delay_test_2_thresh_ms": 400, //only for delay violation test 2 measurement, this para does not impact flow qos.
    },
    "GMA": {
      "downlink_mode": "auto", //"auto", "split", or "steer". "auto" will config UDP and TCP ACK as steer and TCP data as split.
      "uplink_mode": "auto", //"auto", "split", or "steer". "auto" will config UDP and TCP ACK as steer and TCP data as split.
      "enable_dynamic_flow_prioritization": false, //When DFP is enabled, for each cell, mark 70%~90% of traffic or users to high priority.
      "measurement_interval_ms": 100, //duration of a measurement interval.
      "measurement_guard_interval_ms": 0 //gap between 2 measurement interval
    },
    "Wi-Fi": {
      "ap_share_same_band": false, //set to true, ap will share the same frequency band.
      "enable_rx_signal_based_handover": false, //Always connect to the Wi-Fi AP with strongest rx signal, the rx signal is measured from BEACONS.
      "measurement_interval_ms": 100,
      "measurement_guard_interval_ms": 0
    },
    "LTE": {
      "resource_block_num": 25, //number of resouce blocks for LTE, 25 for 5 MHZ, 50 for 10 MHZ, 75 for 15 MHZ and 100 for 20 MHZ.
      "measurement_interval_ms": 100,
      "measurement_guard_interval_ms": 0
    }
  }
}
```

:::

## Measurements

Edit the "subscribed_network_stats" in the [config.json] to include only the necessary measurements required for computing observations and rewards. This minimizes the transmission of 'unused' data and reduces overhead. A complete list of available measurements is povided in the following:

:::{dropdown} Click to view available measurements for this environment.

```
                 id                        name source    ts                                              value
0   [1, 2, 3, 4, 5]                dl::max_rate    lte  3000                     [75.0, 55.0, 55.0, 75.0, 75.0]
1   [1, 2, 3, 4, 5]                     cell_id    lte  3000                          [1.0, 1.0, 1.0, 1.0, 1.0]
2   [1, 2, 3, 4, 5]                    slice_id    lte  3000                          [0.0, 0.0, 1.0, 1.0, 1.0]
3   [1, 2, 3, 4, 5]                dl::rb_usage    lte  3000                     [2.88, 4.08, 4.32, 2.88, 2.88]
4               [1]          dl::cell::max_rate    lte  3000  [{'slice': [0, 1], 'value': [65.0, 68.33333333...
5               [1]          dl::cell::rb_usage    lte  3000        [{'slice': [0, 1], 'value': [6.96, 10.08]}]
6   [1, 2, 3, 4, 5]           ul::missed_action    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
7   [1, 2, 3, 4, 5]          ul::measurement_ok    gma  3000                          [1.0, 1.0, 1.0, 1.0, 1.0]
8   [1, 2, 3, 4, 5]                    ul::rate    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
9   [1, 2, 3, 4, 5]                ul::qos_rate    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
10  [1, 2, 3, 4, 5]         ul::delay_violation    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
11  [1, 2, 3, 4, 5]  ul::delay_test_1_violation    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
12  [1, 2, 3, 4, 5]  ul::delay_test_2_violation    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
13  [1, 2, 3, 4, 5]                     ul::owd    gma  3000                     [-1.0, -1.0, -1.0, -1.0, -1.0]
14  [1, 2, 3, 4, 5]                 ul::max_owd    gma  3000                     [-1.0, -1.0, -1.0, -1.0, -1.0]
15  [1, 2, 3, 4, 5]                 dl::tx_rate    gma  3000                [2.056, 2.056, 1.942, 2.056, 2.056]
16  [1, 2, 3, 4, 5]           lte::dl::priority    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
17  [1, 2, 3, 4, 5]               lte::ul::rate    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
18  [1, 2, 3, 4, 5]           lte::ul::qos_rate    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
19  [1, 2, 3, 4, 5]      lte::ul::traffic_ratio    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
20  [1, 2, 3, 4, 5]                lte::ul::owd    gma  3000                     [-1.0, -1.0, -1.0, -1.0, -1.0]
21  [1, 2, 3, 4, 5]            lte::ul::max_owd    gma  3000                     [-1.0, -1.0, -1.0, -1.0, -1.0]
22  [1, 2, 3, 4, 5]           dl::missed_action    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
23  [1, 2, 3, 4, 5]          dl::measurement_ok    gma  3000                          [1.0, 1.0, 1.0, 1.0, 1.0]
24  [1, 2, 3, 4, 5]                    dl::rate    gma  3000                [2.056, 2.056, 2.056, 2.056, 2.056]
25  [1, 2, 3, 4, 5]                dl::qos_rate    gma  3000                [2.056, 2.056, 2.056, 2.056, 2.056]
26  [1, 2, 3, 4, 5]         dl::delay_violation    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
27  [1, 2, 3, 4, 5]  dl::delay_test_1_violation    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
28  [1, 2, 3, 4, 5]  dl::delay_test_2_violation    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
29  [1, 2, 3, 4, 5]                     dl::owd    gma  3000                          [3.0, 3.0, 3.0, 3.0, 3.0]
30  [1, 2, 3, 4, 5]                 dl::max_owd    gma  3000                          [3.0, 3.0, 3.0, 3.0, 3.0]
31  [1, 2, 3, 4, 5]                 ul::tx_rate    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
32  [1, 2, 3, 4, 5]                       x_loc    gma  3000              [19.83, 2.836, 78.31, 46.913, 27.607]
33  [1, 2, 3, 4, 5]                       y_loc    gma  3000                [8.645, 0.388, 7.493, 4.089, 2.242]
34  [1, 2, 3, 4, 5]           lte::ul::priority    gma  3000                          [0.0, 0.0, 0.0, 0.0, 0.0]
35  [1, 2, 3, 4, 5]               lte::dl::rate    gma  3000                [2.056, 2.056, 2.056, 2.056, 2.056]
36  [1, 2, 3, 4, 5]           lte::dl::qos_rate    gma  3000                [2.056, 2.056, 2.056, 2.056, 2.056]
37  [1, 2, 3, 4, 5]      lte::dl::traffic_ratio    gma  3000                [100.0, 100.0, 100.0, 100.0, 100.0]
38  [1, 2, 3, 4, 5]                lte::dl::owd    gma  3000                          [3.0, 3.0, 3.0, 3.0, 3.0]
39  [1, 2, 3, 4, 5]            lte::dl::max_owd    gma  3000                          [3.0, 3.0, 3.0, 3.0, 3.0]
```

:::

To subscribe to a specific measurement, add its `source`::`name` combination to the "subscribed_network_stats" list. For example, appending `gma::lte::dl::rate` to the list would enable receiving measurements with the name `lte::dl::rate` from the source `gma`. Refer to [Working with Measurements](../../tutorials/networkgym_basics/working_with_measurements.md) for further elaboration of the measurements.

## Observation Space

The observation space is represented by a multidimensional array (`ndarray`) with a shape of `(5, N)`, where each of the `N` slices contains five distinct features selected from the available measurements for this environment:

1. **Total Throughput**: This measures the total data rate for each slice in Mbps, ranging from `0` to `Inf`.
2. **Resource Block Usage Rate**: It represents the fraction of resource blocks utilized by each slice, expressed as a percentage in the range of `[0, 1]`, with the sum across all slices equal to 1.
3. **Average Delay Violation Rate**: Initially measured user-wise and then averaged for each slice, this rate indicates the average proportion of delay violations, expressed as a percentage in the range of `[0, 1]`.
4. **Maximum Delay**: This records the maximum delay experienced by a slice at a specific moment, expressed in milliseconds and ranging from `0` to `Inf`.
5. **Average Delay**: It computes the average delay experienced by a slice, expressed in milliseconds and ranging from `0` to `Inf`.

| Feature | Observation | Min | Max |
| --- | --- | --- | --- |
| 0 | Total Throughput per slice (mbps) | 0.0 | Inf |
| 1 | Resource Block Usage Rate per slice | 0.0 | 1.0 |
| 2 | Average Delay Violation Rate per slice | 0.0 | 1.0 |
| 3 | Maximum Delay per slice (ms) | 0.0 | Inf |
| 4 | Average Delay per slice (ms) | 0.0 | Inf |


## Action Space

The action space is an `ndarray` with a shape of `(N,)`, indicating the allocation of resource blocks to `N` slices. It is important to note that the sum of the actions in this allocation should not exceed one. In case the sum surpasses one, the actions will be appropriately scaled down using the softmax function to ensure adherence to the constraint. This can be expressed mathematically as follows:
```{eval-rst}
if :math:`\sum_{i=1}^{N}a_i > 1, \quad a_i = \frac{e^{a_i}}{\sum_{j}e^{a_j}}.`
```

| Num | Action | Min | Max |
| --- | --- | --- | --- |
| 0 | Prioritized Rb allocated to slice 0 | 0.0 |  1.0 |
| 1 | Prioritized Rb allocated to slice 1 | 0.0 | 1.0 |
| ... |  |  | |
| N | Prioritized Rb allocated to slice N | 0.0 | 1.0 |

## Reward

The reward function in our system takes into account three factors: throughput, delay violation rate, and resource cost. It is formulated as follows:

```{eval-rst}
:math:`R(t) = \sum_{i = 1}^N \left( \frac{{\text{throughput}_i}}{{\text{load}_i}} - \lambda \cdot \text{delay_violation_rate}_i - \gamma \cdot \text{rb_usage_rate}_i \right)`
```

```{eval-rst}
In this formulation, we can adjust the emphasis given to the delay violation rate and the resource block usage rate using the parameters (:math:`\lambda`) and (:math:`\gamma`) respectively. By default, (:math:`\lambda=1.5`) and (:math:`\gamma = 0.5`), which prioritize delay considerations.
```

## Custom Observation Space and Reward
```{eval-rst}
- Customize Observation Space in the :meth:`network_gym_client.envs.network_slicing.Adapter.get_observation` function.
- Customize Reward in the :meth:`network_gym_client.envs.network_slicing.Adapter.get_reward` function.
```




## Starting State
The position of the users is assigned by a uniform random value in a 2D plane with configurable (x, y) boundaries. Every user installs a [random walk mobility model](https://www.nsnam.org/docs/release/3.20/doxygen/classns3_1_1_random_walk2d_mobility_model.html).The mobility parameters can also be configured in the JSON file.

## Episode End
A NetworkGym environment operates for a specified number of episodes, denoted as episodes_per_session (E), and each episode is truncated after a certain number of time steps, denoted as steps_per_episode (L). Resulting in E*L time steps per environment session. Both E and L can be customized through [config.json].
The episode ends if either of the following happens:
1. Truncation: The length of each episode is L steps. Once an episode is truncated, the environment continues to run, generating results for the subsequent episode. It’s important to note that the environment parameters cannot be reconfigured after the end of a truncated episode.
2. Termination: The environment terminates after E episodes. At this point, the agent has the option to reconfigure the environment to continue training or to exit the program entirely.

```{tip}
See [Handling Time Limits](../../tutorials/networkgym_basics/handling_time_limits.md) for more details about Episode End.
```