---
title: Training Agents
---

# Training Agents

NetworkGym provides a range of network agents, each designed to fulfill specific roles when interacting with NetworkGym Environments.

## System Default Agent

The System Default Agent enables the collection of data for offline training purposes. To activate system default algorithms, a agent simply sends an "empty" action (`numpy.array`) to the environment. You can use the [demo](https://github.com/IntelLabs/gma/blob/network-gym/network_gym_client_demo.py) code and modify the action as shown below:

```python
    # action = env.action_space.sample()  # agent policy that uses the observation and info
    action = np.array([])  # No action from the RL agent will trigger the system default algorithm provided by the environment.
``` 

## Custom Algorithm Agent
NetworkGym offers flexibility by allowing users to define their own specialized agents by using the Gymnasium's API. Refer to the Gymnasium's [tutorial](https://gymnasium.farama.org/tutorials/training_agents/) for detailed instructions on creating custom agents.

## Stable-Baselines3 Network Agent
The [Stable-Baselines3 Network Agent](https://github.com/pinyaras/GMAClient) includes the State-of-the-Art (SOTA) Reinforcement Learning (RL) algorithms sourced from [stable-baselines3](https://stable-baselines3.readthedocs.io/en/master/). These algorithms include popular ones such as PPO (Proximal Policy Optimization), DDPG (Deep Deterministic Policy Gradient), SAC (Soft Actor-Critic), TD3 (Twin Delayed Deep Deterministic Policy Gradient), and A2C (Advantage Actor-Critic). Moreover, these algorithms have been integrated to seamlessly interact with the NetworkGym Environment.

## CleanRL Network Agent
An efficient and clean RL agent option is available for custom algorithms. *(Link to be provided later)*
