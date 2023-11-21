---
title: Multi-Access Traffic Splitting
---
(cards-mx-traffic-splitting)=
# Multi-Access Traffic Splitting

```{figure} nqos_split_env.png
---
width: 70%
---
```
[config.json]: https://github.com/IntelLabs/networkgym/blob/main/network_gym_client/envs/nqos_split/config.json

The Multi-Access (MX) Traffic Splitting environment is part of the multi-access traffic management environments and provides general information about the environment.
In this environment, the agent performs periodic actions to update the traffic split ratio for each user, considering both Wi-Fi and LTE connections.

| | |
| ----- | ---- |
| Arguments | [config.json]  |
| Observation Space | `Box(0, Inf, (3, N,), float32)`  |
| Action Space |  `Box(0.0, 1.0, (N,), float32)`  |
| Select Environment | `config_json = load_config_file('nqos_split')` <br> `env = NetworkGymEnv(client_id, config_json)`  |

## Description

The Multi-Access (MX) Traffic Splitting environment represents a traffic management problem where multiple users are randomly distributed on a 2D plane, with each user connecting to a Cellular base station and the closest Wi-Fi access point. The received signal-based handover between Wi-Fi access points can be enabled or disabled.
 The goal of traffic management is to strategically split traffic over both links, aiming to achieve high throughput and low latency.

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
    "episodes_per_session": 5, //the number of episodes per environment session. Environment session end is indicated by terminated signal.
    "random_seed": 2, //change the random seed for this simulation run
    "downlink_traffic": true, //set to true to simulate downlink data flow, set to false to simulate uplink data flow.
    "max_wait_time_for_action_ms": -1, //the max time the network gym worker will wait for an action. set to -1 will cap the wait time to 600*1000 milliseconds.
    "enb_locations":{//x, y and z locations of the base station, we support 1 base station only
      "x":0,
      "y":0,
      "z":3
    },
    "ap_locations":[//x, y and z location of the Wi-Fi access point, add or remove element in this list to increase or reduce AP number. We support 0 AP as well.
      {"x":15,"y":0,"z":3},
      {"x":35,"y":0,"z":3}
    ],
    "num_users" : 4,
    "user_random_walk":{ // configure random walk model with Distance mode. https://www.nsnam.org/docs/release/3.16/doxygen/classns3_1_1_random_walk2d_mobility_model.html
      "min_speed_m/s": 1, //A random variable used to pick the min random walk speed (m/s). Set min and max speed to 0 to disable random walk
      "max_speed_m/s": 2, //A random variable used to pick the max random walk speed (m/s). Set min and max speed to 0 to disable random walk
      "min_direction_gradients": 0.0, //A random variable used to pick the min random walk direction (gradients). [Min=0.0|Max=6.283184]
      "max_direction_gradients": 6.283184, //A random variable used to pick the max random walk direction (gradients). [Min=0.0|Max=6.283184]
      "distance_m": 3 //change current direction and speed after moving for this distance (m)
    },
    "user_location_range":{//initially, users will be randomly deployed within this x, y range. if user_random_walk_max_speed_m/s > 0, the user will random walk within this boundary.
      "min_x":0,
      "max_x":55,
      "min_y":0,
      "max_y":10,
      "z":1.5
    },
    "measurement_start_time_ms": 1000, //the first measurement start time. The first measurement will be sent to the agent between [measurement_start_time_ms, measurement_start_time_ms + measurement_interval_ms].
    "transport_protocol": "tcp", //"tcp" or "udp"
    "udp_poisson_arrival": true, // if "transport_protocol" is "udp", this para controls whether the generater using poisson process.
    "min_udp_rate_per_user_mbps": 6, // if "transport_protocol" is "udp", this para controls the min sending rate.
    "max_udp_rate_per_user_mbps": 6, // if "transport_protocol" is "udp", this para controls the max sending rate.
    "qos_requirement": {//only for qos_steer environment
      "delay_bound_ms": 100,//max delay for qos flow
      "delay_test_1_thresh_ms": 200, //only for delay violation test 1 measurement, this para does not impact flow qos.
      "delay_test_2_thresh_ms": 400, //only for delay violation test 2 measurement, this para does not impact flow qos.
    },
    "GMA": {
      "downlink_mode": "split", //"auto", "split", or "steer". "auto" will config UDP and TCP ACK as steer and TCP data as split.
      "uplink_mode": "auto", //"auto", "split", or "steer". "auto" will config UDP and TCP ACK as steer and TCP data as split.
      "enable_dynamic_flow_prioritization": false, //When DFP is enabled, for each cell, mark 70%~90% of traffic or users to high priority. We recommand to only use this feature for UDP traffic with QoS requirement.
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
      "resource_block_num": 50, //number of resouce blocks for LTE, 25 for 5 MHZ, 50 for 10 MHZ, 75 for 15 MHZ and 100 for 20 MHZ.
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
              id                        name source    ts                               value
0   [1, 2, 3, 4]                dl::max_rate    lte  6000            [37.0, 37.0, 23.0, 37.0]
1   [1, 2, 3, 4]                     cell_id    lte  6000                [1.0, 1.0, 1.0, 1.0]
2   [1, 2, 3, 4]                    slice_id    lte  6000                [0.0, 0.0, 0.0, 0.0]
3   [1, 2, 3, 4]                dl::rb_usage    lte  6000            [25.0, 25.0, 25.0, 25.0]
4            [1]          dl::cell::max_rate    lte  6000   [{'slice': [0], 'value': [33.5]}]
5            [1]          dl::cell::rb_usage    lte  6000  [{'slice': [0], 'value': [100.0]}]
6   [1, 2, 3, 4]           ul::missed_action    gma  6000                [0.0, 0.0, 0.0, 0.0]
7   [1, 2, 3, 4]          ul::measurement_ok    gma  6000                [1.0, 1.0, 1.0, 1.0]
8   [1, 2, 3, 4]                    ul::rate    gma  6000         [0.152, 0.32, 0.112, 0.256]
9   [1, 2, 3, 4]                ul::qos_rate    gma  6000         [0.152, 0.32, 0.112, 0.256]
10  [1, 2, 3, 4]         ul::delay_violation    gma  6000                [0.0, 0.0, 0.0, 0.0]
11  [1, 2, 3, 4]  ul::delay_test_1_violation    gma  6000                [0.0, 0.0, 0.0, 0.0]
12  [1, 2, 3, 4]  ul::delay_test_2_violation    gma  6000                [0.0, 0.0, 0.0, 0.0]
13  [1, 2, 3, 4]                     ul::owd    gma  6000                [1.0, 1.0, 1.0, 1.0]
14  [1, 2, 3, 4]                 ul::max_owd    gma  6000                [2.0, 3.0, 1.0, 1.0]
15  [1, 2, 3, 4]                 dl::tx_rate    gma  6000     [12.312, 17.888, 6.272, 14.868]
16  [1, 2, 3, 4]          wifi::dl::priority    gma  6000                [0.0, 0.0, 0.0, 0.0]
17  [1, 2, 3, 4]              wifi::ul::rate    gma  6000         [0.152, 0.32, 0.112, 0.256]
18  [1, 2, 3, 4]          wifi::ul::qos_rate    gma  6000         [0.152, 0.32, 0.112, 0.256]
19  [1, 2, 3, 4]     wifi::ul::traffic_ratio    gma  6000        [100.0, 100.0, 100.0, 100.0]
20  [1, 2, 3, 4]               wifi::ul::owd    gma  6000                [1.0, 1.0, 1.0, 1.0]
21  [1, 2, 3, 4]           wifi::ul::max_owd    gma  6000                [2.0, 3.0, 1.0, 1.0]
22  [1, 2, 3, 4]           lte::dl::priority    gma  6000                [0.0, 0.0, 0.0, 0.0]
23  [1, 2, 3, 4]               lte::ul::rate    gma  6000                [0.0, 0.0, 0.0, 0.0]
24  [1, 2, 3, 4]           lte::ul::qos_rate    gma  6000                [0.0, 0.0, 0.0, 0.0]
25  [1, 2, 3, 4]      lte::ul::traffic_ratio    gma  6000                [0.0, 0.0, 0.0, 0.0]
26  [1, 2, 3, 4]                lte::ul::owd    gma  6000            [-1.0, -1.0, -1.0, -1.0]
27  [1, 2, 3, 4]            lte::ul::max_owd    gma  6000            [-1.0, -1.0, -1.0, -1.0]
28  [1, 2, 3, 4]       wifi::dl::split_ratio    gma  6000       [56.25, 50.0, 53.125, 21.875]
29  [1, 2, 3, 4]        lte::dl::split_ratio    gma  6000       [43.75, 50.0, 46.875, 78.125]
30  [1, 2, 3, 4]           dl::missed_action    gma  6000                [0.0, 0.0, 0.0, 0.0]
31  [1, 2, 3, 4]          dl::measurement_ok    gma  6000                [1.0, 1.0, 1.0, 1.0]
32  [1, 2, 3, 4]                    dl::rate    gma  6000        [8.824, 17.072, 6.736, 14.4]
33  [1, 2, 3, 4]                dl::qos_rate    gma  6000        [8.824, 17.072, 6.736, 14.4]
34  [1, 2, 3, 4]         dl::delay_violation    gma  6000                [0.0, 0.0, 0.0, 0.0]
35  [1, 2, 3, 4]  dl::delay_test_1_violation    gma  6000                [0.0, 0.0, 0.0, 0.0]
36  [1, 2, 3, 4]  dl::delay_test_2_violation    gma  6000                [0.0, 0.0, 0.0, 0.0]
37  [1, 2, 3, 4]                     dl::owd    gma  6000         [298.0, 65.0, 682.0, 507.0]
38  [1, 2, 3, 4]                 dl::max_owd    gma  6000         [310.0, 71.0, 689.0, 523.0]
39  [1, 2, 3, 4]                 ul::tx_rate    gma  6000         [0.158, 0.307, 0.12, 0.257]
40  [1, 2, 3, 4]                       x_loc    gma  6000     [13.871, 6.703, 52.198, 33.715]
41  [1, 2, 3, 4]                       y_loc    gma  6000        [7.964, 1.676, 9.021, 0.988]
42  [1, 2, 3, 4]          wifi::ul::priority    gma  6000                [0.0, 0.0, 0.0, 0.0]
43  [1, 2, 3, 4]              wifi::dl::rate    gma  6000         [7.08, 9.176, 3.136, 3.248]
44  [1, 2, 3, 4]          wifi::dl::qos_rate    gma  6000         [7.08, 9.176, 3.136, 3.248]
45  [1, 2, 3, 4]     wifi::dl::traffic_ratio    gma  6000            [45.0, 52.0, 36.0, 28.0]
46  [1, 2, 3, 4]               wifi::dl::owd    gma  6000                [0.0, 0.0, 0.0, 0.0]
47  [1, 2, 3, 4]           wifi::dl::max_owd    gma  6000                [2.0, 2.0, 0.0, 1.0]
48  [1, 2, 3, 4]           lte::ul::priority    gma  6000                [0.0, 0.0, 0.0, 0.0]
49  [1, 2, 3, 4]               lte::dl::rate    gma  6000        [8.592, 8.592, 5.456, 8.472]
50  [1, 2, 3, 4]           lte::dl::qos_rate    gma  6000        [8.592, 8.592, 5.456, 8.472]
51  [1, 2, 3, 4]      lte::dl::traffic_ratio    gma  6000            [55.0, 48.0, 64.0, 72.0]
52  [1, 2, 3, 4]                lte::dl::owd    gma  6000         [298.0, 65.0, 682.0, 507.0]
53  [1, 2, 3, 4]            lte::dl::max_owd    gma  6000         [310.0, 71.0, 689.0, 523.0]
54  [1, 2, 3, 4]                     cell_id   wifi  6000                [0.0, 0.0, 1.0, 1.0]
55  [1, 2, 3, 4]                dl::max_rate   wifi  6000            [78.0, 78.0, 65.0, 78.0]
56  [1, 2, 3, 4]                ul::max_rate   wifi  6000            [78.0, 78.0, 78.0, 78.0]
```

:::


To subscribe to a specific measurement, add its `source`::`name` combination to the "subscribed_network_stats" list. For example, appending `gma::lte::dl::rate` to the list would enable receiving measurements with the name `lte::dl::rate` from the source `gma`. Refer to [Working with Measurements](../../tutorials/networkgym_basics/working_with_measurements.md) for further elaboration of the measurements.

## Observation Space

The observation is an `ndarray` with shape `(3,N,)` representing three features for N users selected from the available measurements for this environment.
The first feature is MAX LTE rate, representing the user's estimation of channel capacity (e.g., the max rate if the user utilizes all resources). The second feature is the Max Wi-Fi rate, and the third feature is the received throughput, combining both links.

| Feature | Observation | Min | Max |
| ----- | ---- | ----- | ---- |
| 0 | MAX LTE rate (mbps) | 0.0 | Inf |
| 1 | MAX Wi-Fi rate  (mbps) | 0.0 | Inf |
| 2 | throughput (mbps) | 0.0 | Inf|

## Action Space
The action space is a `ndarray` with shape `(N,)` representing the traffic ratio over Wi-Fi for N users. The traffic ratio over Cellular equals (1.0 - action).

| Num | Action | Min | Max |
| ----- | ---- | ----- | ---- |
| 0 | Wi-Fi traffic ratio for user 0 | 0.0 | 1.0 |
| 1 | Wi-Fi traffic ratio for user 1| 0.0 | 1.0 |
| ... | | | |
| N-1 | Wi-Fi traffic ratio for user N-1| 0.0 | 1.0 |

## Transition Dynamics
Given an action (Wi-Fi split ratio for user i: R[i]), transmitter i follows the following transition dynamics:
- transmit R[i] traffic over Wi-Fi link.
- transmit (1-R[i]) traffic over LTE link.

## Reward

```{eval-rst}
We compute a utility function :math:`f = 0.5 \times log(throughput)-0.5 \times log(OWD)` using the reward function :meth:`network_gym_client.envs.nqos_split.Adapter.netowrk_util`, where OWD is the one-way delay.
The goal of the utility function is to maximize the throughput and minimizing delay.
```

## Custom Observation Space and Reward
```{eval-rst}
- Customize Observation Space in the :meth:`network_gym_client.envs.nqos_split.Adapter.get_observation` function.
- Customize Reward in the :meth:`network_gym_client.envs.nqos_split.Adapter.get_reward` function.
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