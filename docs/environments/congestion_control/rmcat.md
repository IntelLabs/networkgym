---
title: RMCAT
---
(cards-rmcat)=
# RMCAT
```{figure} rmcat.png
---
width: 100%
---
```

[config.json]: https://github.com/IntelLabs/networkgym/blob/main/network_gym_client/envs/rmcat/config.json

The RMCAT environment is part of the congestion control environments and provides general information about the environment.
In this environment, the agent performs periodic actions to adjust the sending rate of a real-time traffic flow using RTP Media Congestion Avoidance Techniques (RMCAT).
This environment is based on the GitHub Repo: [cisco/ns3-rmcat](https://github.com/cisco/ns3-rmcat).

| | |
| ----- | ---- |
| Arguments | [config.json]  |
| Observation Space | `Box(0, Inf, (3,), float32)`  |
| Action Space |  `Box(150000.0, 1500000.0, (1,), float32)`  |
| Select Environment | `config_json = load_config_file('rmcat')` <br> `env = NetworkGymEnv(client_id, config_json)`  |

## Description

The RMCAT environment represents a congestion control problem for real-time traffic. The goal of congestion control is to strategically control the sending rate, aiming to achieve high throughput, low latency, and low loss. If no action is given to the environment, RMCAT uses [NADA](https://datatracker.ietf.org/doc/html/draft-ietf-rmcat-nada-05) as its default algorithm.

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
    "steps_per_episode": 100, //the number of steps for each episode. Episode end is indicated by truncated signal.
    "episodes_per_session": 10, //the number of episodes per environment session. Environment session end is indicated by terminated signal.
    "nada_flows": 1, //number of nada flows. For single agent case, we only support 1 nada flow.
    "nada_flow_start_time_diff_ms": 1, //wait for this time, before starting a new nada flows.
    "tcp_flows": 0, //the number of tcp flows. May be used as background traffic.
    "tcp_flow_start_time_diff_ms": 50000, //wait for this time, before starting a new tcp flows.
    "udp_flows": 0, //the number of udp flows. May be used as background traffic.
    "udp_flow_bw_kbps":100, //data rate for upd traffic in kbps.
    "upd_flow_start_time_diff_ms": 50000, //wait for this time, before starting a new udp flows.
    "measurement_start_time_ms": 1000, //the first measurement start time. The first measurement will be sent to the agent between [measurement_start_time_ms, measurement_start_time_ms + measurement_interval_ms].
    "measurement_interval_ms": 100, //duration of a measurement interval. It cannot be changed since the NADA fixed it to 100 ms.
    "topo_default_bw_kbps": 1000, // default link bandwith in kbps.
    "topo_default_delay_ms": 10, // default link delay in ms.
    "bw_trace_file": "bw_trace.csv" //the name of bandwith trace file.
  }

}
```

:::

## Measurements

Edit the "subscribed_network_stats" in the [config.json] to include only the necessary measurements required for computing observations and rewards. This minimizes the transmission of 'unused' data and reduces overhead. A complete list of available measurements is povided in the following:

:::{dropdown} Click to view available measurements for this environment.

```
     id        name source      ts                value
0   [0]      loglen  rmcat  100710               [58.0]
1   [0]        qdel  rmcat  100710               [30.0]
2   [0]         rtt  rmcat  100710               [55.0]
3   [0]       ploss  rmcat  100710                [0.0]
4   [0]         plr  rmcat  100710                [0.0]
5   [0]       xcurr  rmcat  100710  [30.17099952697754]
6   [0]       rrate  rmcat  100710         [774531.375]
7   [0]       srate  rmcat  100710           [585416.0]
8   [0]  nada_srate  rmcat  100710        [557536.1875]
9   [0]      avgint  rmcat  100710                [0.0]
10  [0]      curint  rmcat  100710                [0.0]
```

:::

To subscribe to a specific measurement, add its `source`::`name` combination to the "subscribed_network_stats" list. For example, appending `rmcat::rtt` to the list would enable receiving measurements with the name `rtt` from the source `rmcat`. Refer to [Working with Measurements](../../tutorials/networkgym_basics/working_with_measurements.md) for further elaboration of the measurements.

## Observation Space

The observation is an `ndarray` with shape `(3,)` representing three features selected from the available measurements for this environment. 

The first feature is `rtt`, representing the round trip time. The second feature is `xcurr`, represeting the congestion level. The third feature is the received throughput `rrate`.

| Feature | Observation | Min | Max |
| ----- | ---- | ----- | ---- |
| 0 | rtt (ms) | 0.0 | Inf |
| 1 | xcurr | 0.0 | 500.0 |
| 2 | rrate (bps) | 0.0 | Inf|

## Action Space
The action space is a `ndarray` with shape `(1,)` representing the sending rate of rmcat flow. The min value is 150000 and max value is 1500000.


## Reward

Not implemented yet.

## Customize Observation Space and Reward

```{eval-rst}
- Customize Observation Space in the :meth:`network_gym_client.envs.rmcat.Adapter.get_observation` function.
- Customize Reward in the :meth:`network_gym_client.envs.rmcat.Adapter.get_reward` function.
```


## Episode End
A NetworkGym environment operates for a specified number of episodes, denoted as episodes_per_session (E), and each episode is truncated after a certain number of time steps, denoted as steps_per_episode (L). Resulting in E*L time steps per environment session. Both E and L can be customized through [config.json].
The episode ends if either of the following happens:
1. Truncation: The length of each episode is L steps. Once an episode is truncated, the environment continues to run, generating results for the subsequent episode. It’s important to note that the environment parameters cannot be reconfigured after the end of a truncated episode.
2. Termination: The environment terminates after E episodes. At this point, the agent has the option to reconfigure the environment to continue training or to exit the program entirely.

```{tip}
See [Handling Time Limits](../../tutorials/networkgym_basics/handling_time_limits.md) for more details about Episode End.
```