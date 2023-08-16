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

The Multi-Access (MX) QoS Traffic Steering environment is part of the multi-access traffic management environments and provides general information about the environment.
In this environment, the agent performs periodic actions to select a link to steer traffic over for each user, considering both Wi-Fi and LTE connections.

| | |
| ----- | ---- |
| Observation Space | `Box(0, Inf, (3, N,), float32)`  |
| Action Space |  `MultiDiscrete([2, 2, ..., 2]) # N dimension`  |
| Arguments | [config.json](https://github.com/IntelLabs/networkgym/network_gym_client/envs/qos_steer/config.json)  |
| Select Environment | `config_json = load_config_file('qos_steer')` <br> `env = NetworkGymEnv(client_id, config_json)`  |

## Description

The Multi-Access (MX) QoS Traffic Steering scenario presents the challenge of efficiently managing traffic for multiple users distributed randomly across a 2D plane. Each user has access to both Cellular and Wi-Fi links. The primary goal of traffic management is to intelligently direct the traffic through either of these links, aiming to achieve the best Quality of Service (QoS).

Regarding the Cellular link, there is a bandwidth constraint, and the scheduler will halt the admission of users when the bandwidth capacity is reached. For instance, if 10 users are directed over the Cellular link, but only 5 of them can be accommodated with the required QoS, the Cellular link will prioritize serving the first 5 users with QoS and will drop the remaining 5 users.

On the other hand, the Wi-Fi link operates differently as it lacks admission control. All users contend for available bandwidth without any restrictions on user admission.

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


## Observation Space

The observation is an `ndarray` with shape `(3,N,)` representing three features for N users. The first feature is MAX LTE rate, representing the user's estimation of channel capacity (e.g., the max rate if the user utilizes all resources). The second feature is the Max Wi-Fi rate, and the third feature is the received throughput, combining both links.

| Feature | Observation | Min | Max |
| ----- | ---- | ----- | ---- |
| 0 | MAX LTE rate (mbps) | 0.0 | Inf |
| 1 | MAX Wi-Fi rate  (mbps) | 0.0 | Inf |
| 2 | throughput (mbps) | 0.0 | Inf|


````{Note}

```{eval-rst}
The Observation Space can be customized in the :meth:`network_gym_client.envs.qos_steer.Adapter.get_observation` function.
```
See [Customizing Observation and Reward](../../tutorials/customizing_observation_and_reward.md) for more details.
````

```{tip}
The Observation Space can be normalized using the [NormalizeObservation](https://gymnasium.farama.org/_modules/gymnasium/wrappers/normalize/#NormalizeObservation) wrapper.
```

## Action Space

The action is a `ndarray` with shape `(N,)` which can take values {0, 1} indicating the link to transmit data. N is the number of users.
- 0: Steer traffic over Cellular
- 1: Steer traffic over Wi-Fi

```{tip}
The Action Space can be rescaled using the [RescaleAction](https://gymnasium.farama.org/_modules/gymnasium/wrappers/rescale_action/) wrapper.
```

## Reward

```{eval-rst}

We computes the number of Quality of Service (QoS) users over Wi-Fi as reward function :meth:`network_gym_client.envs.qos_steer.Adapter.calculate_wifi_qos_user_num`. Given that Cellular already implements an admission control mechanism to support the maximum number of QoS users, maximizing the QoS users over Wi-Fi also leads to the maximization of the total number of users in the network that meet the QoS requirements. In essence, optimizing the QoS users over Wi-Fi indirectly ensures the best possible overall network performance with the desired QoS levels for all users.

```

````{Note}

```{eval-rst}
The Reward can be customized in the :meth:`network_gym_client.envs.qos_steer.Adapter.get_reward` function.
```
See [Customizing Observation and Reward](../../tutorials/customizing_observation_and_reward.md) for more details.
````

## Arguments

All the network configurable parameters are defined in the JSON files. When the client starts, the JSON files will be loaded and transmitted to the server to configure the environment.
See the [NetworkGym GitHub Repo](https://github.com/IntelLabs/networkgym#%EF%B8%8F-configurable-file-format) for more details.

## Starting State
The position of the users is assigned by a uniform random value in a 2D plane with configurable (x, y) boundaries. Every user installs a [random walk mobility model](https://www.nsnam.org/docs/release/3.20/doxygen/classns3_1_1_random_walk2d_mobility_model.html).The mobility parameters can also be configured in the JSON file.

## Episode End

A NetworkGym environment operates for a specified number of episodes, denoted as episodes_per_session (E), and each episode is truncated after a certain number of time steps, denoted as steps_per_episode (L). Resulting in E*L time steps per environment session. Both E and L can be customized through JSON configuration files.
The episode ends if either of the following happens:
1. Truncation: The length of each episode is L steps. Once an episode is truncated, the environment continues to run, generating results for the subsequent episode. It’s important to note that the environment parameters cannot be reconfigured after the end of a truncated episode.
2. Termination: The environment terminates after E episodes. At this point, the agent has the option to reconfigure the environment to continue training or to exit the program entirely.

```{tip}
See [Handling Time Limits](../../tutorials/handling_time_limits.md) for more details about Episode End.
```