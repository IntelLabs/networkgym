---
title: Multi-Access QoS Traffic Steering
---
(cards-mx-qos-traffic-steering)=
# Multi-Access QoS Traffic Steering

```{figure} qos_steer.png
---
width: 70%
---
```
[config.json]: https://github.com/IntelLabs/networkgym/blob/main/network_gym_client/envs/qos_steer/config.json

The Multi-Access (MX) QoS Traffic Steering environment is part of the multi-access traffic management environments and provides general information about the environment.
In this environment, the agent performs periodic actions to select a link to steer traffic over for each user, considering both Wi-Fi and LTE connections.

| | |
| ----- | ---- |
| Arguments | [config.json] |
| Observation Space | `Box(0, Inf, (3, N,), float32)`  |
| Action Space |  `MultiDiscrete([2, 2, ..., 2]) # N dimension`  |
| Select Environment | `config_json = load_config_file('qos_steer')` <br> `env = NetworkGymEnv(client_id, config_json)`  |

## Description

The Multi-Access (MX) QoS Traffic Steering scenario presents the challenge of efficiently managing traffic for multiple users distributed randomly across a 2D plane. Each user has access to both Cellular and Wi-Fi links. The primary goal of traffic management is to intelligently direct the traffic through either of these links, aiming to achieve the best Quality of Service (QoS).

If `enable_dynamic_flow_prioritization` is false, all users contend for available bandwidth over Cellular and WiFi without any restrictions on user admission. However, if `enable_dynamic_flow_prioritization` is true, GMA dynamically selects a subset of users to assign high priority with DSCP marking. Both Cellular and WiFi prioritize users with high priority.

We can adjust the following QoS parameters:
```json
"qos_requirement": {//only for qos_steer environment
        "test_duration_ms": 500,//duration for qos testing
        "delay_bound_ms": 100,//max delay for qos flow
        "delay_violation_target":0.02, //delay violation target for qos flow
        "loss_target": 0.001 //loss target for qos flow
      },
```
During the QoS evaluation, if a flow meets all the specified QoS requirements regarding delay and packet loss, we will calculate its qos_rate as the throughput. Otherwise, if the QoS requirements are not met, the qos_rate will be set to zero.

## Prerequisite

Ensure that you have access to the NetworkGym Server on [vLab](https://registration.intel-research.net/) machines and have downloaded the [NetworkGym](https://github.com/IntelLabs/networkgym).


## Arguments

The arguements for this environment can be customized in the [config.json] file. When the client initiates, the JSON file is loaded and sent to the server to set up the environment.

:::{dropdown} Click to view configuration suggestions for arguments.

```json
{
  //never use negative value for any configure vale!!!

  "rl_config":{
    "agent": "",
    "reward_type" : "",
  },

  "env_config":{
    "type": "env-start", //do not change
    "subscribed_network_stats":[], //the environment will only report the subscribed measurements.
    "steps_per_episode": 10, //the number of steps for each episode. Episode end is indicated by truncated signal.
    "episodes_per_session": 1, //the number of episodes per environment session. Environment session end is indicated by terminated signal.
    "random_seed": 2, //change the random seed for this simulation run
    "downlink_traffic": true, //set to true to simulate downlink data flow, set to false to simulate uplink data flow.
    "max_wait_time_for_action_ms": -1, //the max time the network gym worker will wait for an action. set to -1 will cap the wait time to 600*1000 milliseconds.
    "enb_locations":{//x, y and z locations of the base station, we support 1 base station only
      "x":40,
      "y":0,
      "z":3
    },
    "ap_locations":[//x, y and z location of the Wi-Fi access point, add or remove element in this list to increase or reduce AP number. We support 0 AP as well.
      {"x":40,"y":0,"z":3},
    ],
    "num_users" : 8,
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
    "transport_protocol": "udp", //"tcp" or "udp"
    "udp_poisson_arrival": false, // if "transport_protocol" is "udp", this para controls whether the generater using poisson process.
    "min_udp_rate_per_user_mbps": 2, // if "transport_protocol" is "udp", this para controls the min sending rate.
    "max_udp_rate_per_user_mbps": 3, // if "transport_protocol" is "udp", this para controls the max sending rate.
    "qos_requirement": {//only for qos_steer environment
      "test_duration_ms": 500,//duration for qos testing
      "delay_bound_ms": 100,//max delay for qos flow
      "delay_test_1_thresh_ms": 200, //only for delay violation test 1 measurement, this para does not impact flow qos.
      "delay_test_2_thresh_ms": 400, //only for delay violation test 2 measurement, this para does not impact flow qos.
      "delay_violation_target":0.02, //delay violation target for qos flow
      "loss_target": 0.001 //loss target for qos flow
    },
    "GMA": {
      "downlink_mode": "auto", //"auto", "split", or "steer". "auto" will config UDP and TCP ACK as steer and TCP data as split.
      "uplink_mode": "auto", //"auto", "split", or "steer". "auto" will config UDP and TCP ACK as steer and TCP data as split.
      "enable_dynamic_flow_prioritization": false, //When DFP is enabled, for each cell, mark 70%~90% of traffic or users to high priority. We recommand to only use this feature for UDP traffic with QoS requirement.
      "measurement_interval_ms": 1000, //duration of a measurement interval.
      "measurement_guard_interval_ms": 0 //gap between 2 measurement interval
    },
    "Wi-Fi": {
      "ap_share_same_band": false, //set to true, ap will share the same frequency band.
      "enable_rx_signal_based_handover": false, //Always connect to the Wi-Fi AP with strongest rx signal, the rx signal is measured from BEACONS.
      "measurement_interval_ms": 1000,
      "measurement_guard_interval_ms": 0
    },
    "LTE": {
      "resource_block_num": 25, //number of resouce blocks for LTE, 25 for 5 MHZ, 50 for 10 MHZ, 75 for 15 MHZ and 100 for 20 MHZ.
      "measurement_interval_ms": 1000,
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
                          id                        name source     ts                                              value
0   [1, 2, 3, 4, 5, 6, 7, 8]                dl::max_rate    lte  11000   [18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0, 18.0]
1   [1, 2, 3, 4, 5, 6, 7, 8]                     cell_id    lte  11000           [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
2   [1, 2, 3, 4, 5, 6, 7, 8]                    slice_id    lte  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
3   [1, 2, 3, 4, 5, 6, 7, 8]                dl::rb_usage    lte  11000  [0.09166666666666666, 0.09166666666666666, 17....
4                        [1]          dl::cell::max_rate    lte  11000                  [{'slice': [0], 'value': [18.0]}]
5                        [1]          dl::cell::rb_usage    lte  11000     [{'slice': [0], 'value': [41.45833333333333]}]
6   [1, 2, 3, 4, 5, 6, 7, 8]           ul::missed_action    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
7   [1, 2, 3, 4, 5, 6, 7, 8]          ul::measurement_ok    gma  11000           [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
8   [1, 2, 3, 4, 5, 6, 7, 8]                    ul::rate    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
9   [1, 2, 3, 4, 5, 6, 7, 8]                ul::qos_rate    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
10  [1, 2, 3, 4, 5, 6, 7, 8]         ul::delay_violation    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
11  [1, 2, 3, 4, 5, 6, 7, 8]  ul::delay_test_1_violation    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
12  [1, 2, 3, 4, 5, 6, 7, 8]  ul::delay_test_2_violation    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
13  [1, 2, 3, 4, 5, 6, 7, 8]                     ul::owd    gma  11000   [-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0]
14  [1, 2, 3, 4, 5, 6, 7, 8]                 ul::max_owd    gma  11000   [-1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0]
15  [1, 2, 3, 4, 5, 6, 7, 8]                 dl::tx_rate    gma  11000  [3.05, 2.033, 3.061, 3.061, 2.044, 2.044, 2.03...
16  [1, 2, 3, 4, 5, 6, 7, 8]          wifi::dl::priority    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
17  [1, 2, 3, 4, 5, 6, 7, 8]              wifi::ul::rate    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
18  [1, 2, 3, 4, 5, 6, 7, 8]          wifi::ul::qos_rate    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
19  [1, 2, 3, 4, 5, 6, 7, 8]     wifi::ul::traffic_ratio    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
20  [1, 2, 3, 4, 5, 6, 7, 8]               wifi::ul::owd    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
21  [1, 2, 3, 4, 5, 6, 7, 8]           wifi::ul::max_owd    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0]
22  [1, 2, 3, 4, 5, 6, 7, 8]           lte::dl::priority    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
23  [1, 2, 3, 4, 5, 6, 7, 8]               lte::ul::rate    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
24  [1, 2, 3, 4, 5, 6, 7, 8]           lte::ul::qos_rate    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
25  [1, 2, 3, 4, 5, 6, 7, 8]      lte::ul::traffic_ratio    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
26  [1, 2, 3, 4, 5, 6, 7, 8]                lte::ul::owd    gma  11000   [11.0, 11.0, 11.0, 11.0, 11.0, 11.0, 11.0, 11.0]
27  [1, 2, 3, 4, 5, 6, 7, 8]            lte::ul::max_owd    gma  11000   [12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0, 12.0]
28  [1, 2, 3, 4, 5, 6, 7, 8]       wifi::dl::split_ratio    gma  11000  [100.0, 100.0, 0.0, 100.0, 100.0, 100.0, 0.0, ...
29  [1, 2, 3, 4, 5, 6, 7, 8]        lte::dl::split_ratio    gma  11000     [0.0, 0.0, 100.0, 0.0, 0.0, 0.0, 100.0, 100.0]
30  [1, 2, 3, 4, 5, 6, 7, 8]           dl::missed_action    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
31  [1, 2, 3, 4, 5, 6, 7, 8]          dl::measurement_ok    gma  11000           [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0]
32  [1, 2, 3, 4, 5, 6, 7, 8]                    dl::rate    gma  11000  [3.072, 2.04, 3.048, 3.056, 2.04, 2.04, 2.032,...
33  [1, 2, 3, 4, 5, 6, 7, 8]                dl::qos_rate    gma  11000  [3.072, 2.04, 3.048, 3.056, 2.04, 2.04, 2.032,...
34  [1, 2, 3, 4, 5, 6, 7, 8]         dl::delay_violation    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
35  [1, 2, 3, 4, 5, 6, 7, 8]  dl::delay_test_1_violation    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
36  [1, 2, 3, 4, 5, 6, 7, 8]  dl::delay_test_2_violation    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
37  [1, 2, 3, 4, 5, 6, 7, 8]                     dl::owd    gma  11000           [0.0, 0.0, 3.0, 0.0, 0.0, 0.0, 2.0, 2.0]
38  [1, 2, 3, 4, 5, 6, 7, 8]                 dl::max_owd    gma  11000           [4.0, 3.0, 5.0, 3.0, 2.0, 3.0, 3.0, 3.0]
39  [1, 2, 3, 4, 5, 6, 7, 8]                 ul::tx_rate    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
40  [1, 2, 3, 4, 5, 6, 7, 8]                       x_loc    gma  11000  [17.205, 11.659, 79.389, 55.285, 23.856, 30.13...
41  [1, 2, 3, 4, 5, 6, 7, 8]                       y_loc    gma  11000  [7.294, 3.042, 3.656, 9.48, 4.126, 8.449, 1.69...
42  [1, 2, 3, 4, 5, 6, 7, 8]          wifi::ul::priority    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
43  [1, 2, 3, 4, 5, 6, 7, 8]              wifi::dl::rate    gma  11000  [3.048, 2.032, 0.032, 3.056, 2.04, 2.032, 0.01...
44  [1, 2, 3, 4, 5, 6, 7, 8]          wifi::dl::qos_rate    gma  11000  [3.048, 2.032, 0.032, 3.056, 2.04, 2.032, 0.01...
45  [1, 2, 3, 4, 5, 6, 7, 8]     wifi::dl::traffic_ratio    gma  11000    [99.0, 99.0, 1.0, 100.0, 100.0, 99.0, 1.0, 1.0]
46  [1, 2, 3, 4, 5, 6, 7, 8]               wifi::dl::owd    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
47  [1, 2, 3, 4, 5, 6, 7, 8]           wifi::dl::max_owd    gma  11000           [2.0, 3.0, 1.0, 3.0, 2.0, 1.0, 1.0, 1.0]
48  [1, 2, 3, 4, 5, 6, 7, 8]           lte::ul::priority    gma  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
49  [1, 2, 3, 4, 5, 6, 7, 8]               lte::dl::rate    gma  11000  [0.016, 0.008, 3.008, 0.0, 0.0, 0.008, 2.008, ...
50  [1, 2, 3, 4, 5, 6, 7, 8]           lte::dl::qos_rate    gma  11000  [0.016, 0.008, 3.008, 0.0, 0.0, 0.008, 2.008, ...
51  [1, 2, 3, 4, 5, 6, 7, 8]      lte::dl::traffic_ratio    gma  11000        [1.0, 1.0, 99.0, 0.0, 0.0, 1.0, 99.0, 99.0]
52  [1, 2, 3, 4, 5, 6, 7, 8]                lte::dl::owd    gma  11000           [3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0, 3.0]
53  [1, 2, 3, 4, 5, 6, 7, 8]            lte::dl::max_owd    gma  11000           [4.0, 3.0, 5.0, 3.0, 3.0, 3.0, 3.0, 3.0]
54  [1, 2, 3, 4, 5, 6, 7, 8]                     cell_id   wifi  11000           [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
55  [1, 2, 3, 4, 5, 6, 7, 8]                dl::max_rate   wifi  11000   [65.0, 65.0, 39.0, 78.0, 78.0, 78.0, 78.0, 78.0]
56  [1, 2, 3, 4, 5, 6, 7, 8]                ul::max_rate   wifi  11000   [52.0, 58.5, 52.0, 78.0, 78.0, 78.0, 65.0, 65.0]
```

:::

To subscribe to a specific measurement, add its `source`::`name` combination to the "subscribed_network_stats" list. For example, appending `gma::lte::dl::rate` to the list would enable receiving measurements with the name `lte::dl::rate` from the source `gma`. Refer to [Working with Measurements](../../tutorials/networkgym_basics/working_with_measurements.md) for further elaboration of the measurements.

## Observation Space

The observation is an `ndarray` with shape `(3,N,)` representing three features for N users selected from the available measurement for this environment. 


The first feature is MAX LTE rate, representing the user's estimation of channel capacity (e.g., the max rate if the user utilizes all resources). The second feature is the Max Wi-Fi rate, and the third feature is the received throughput, combining both links.

| Feature | Observation | Min | Max |
| ----- | ---- | ----- | ---- |
| 0 | MAX LTE rate (mbps) | 0.0 | Inf |
| 1 | MAX Wi-Fi rate  (mbps) | 0.0 | Inf |
| 2 | throughput (mbps) | 0.0 | Inf|

## Action Space

The action is a `ndarray` with shape `(N,)` which can take values {0, 1} indicating the link to transmit data. N is the number of users.
- 0: Steer traffic over Cellular
- 1: Steer traffic over Wi-Fi

## Reward

```{eval-rst}

We computes the number of Quality of Service (QoS) users over Wi-Fi as reward function :meth:`network_gym_client.envs.qos_steer.Adapter.calculate_wifi_qos_user_num`. Given that Cellular already implements an admission control mechanism to support the maximum number of QoS users, maximizing the QoS users over Wi-Fi also leads to the maximization of the total number of users in the network that meet the QoS requirements. In essence, optimizing the QoS users over Wi-Fi indirectly ensures the best possible overall network performance with the desired QoS levels for all users.

```

## Custom Observation Space and Reward
```{eval-rst}
- Customize Observation Space in the :meth:`network_gym_client.envs.qos_steer.Adapter.get_observation` function.
- Customize Reward in the :meth:`network_gym_client.envs.qos_steer.Adapter.get_reward` function.
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