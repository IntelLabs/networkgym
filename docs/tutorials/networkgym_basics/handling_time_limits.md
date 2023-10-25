---
title: Handling Time Limits
---

# Handling Time Limits

This section outlines the handling of time limits (referred to as `termination` and `truncation`) in the NetworkGym Environment.
A NetworkGym environment operates for a specified number of episodes, denoted as episodes_per_session (E), and each episode is truncated after a certain number of time steps, denoted as steps_per_episode (L). Resulting in E*L time steps per environment session. Both E and L can be customized through JSON configuration files.

## Truncation

The length of each episode is L steps. Once an episode is truncated, the environment continues to run, generating results for the subsequent episode. It's important to note that the environment parameters cannot be reconfigured after the end of a truncated episode.

## Termination

The environment terminates after E episodes. At this point, the agent has the option to reconfigure the environment to continue training or to exit the program entirely.

## Sequential Training Example
Below is an illustration of sequential training an agent with two different environments. The first environment lasts for three episodes, while the second environment session lasts for one episode.

```{mermaid}
sequenceDiagram

    agent->>+env: Environment Start (E=3)
    env-->>agent: Episode End (Truncation=True, Termination=False)
    env-->>agent: Episode End (Truncation=True, Termination=False)
    env-->>-agent: Episode and Environment End (Truncation=True, Termination=True)

    agent->>+env: Environment Start (E=1)
    env-->>-agent: Episode and Environment End (Truncation=True, Termination=True)
```

### Python code:
```python
# In this example, we perform sequential training for two environment sessions.
# The first session lasts for 3 episodes, and the second session lasts for 1 episode.

from network_gym_client import load_config_file
from network_gym_client import Env as NetworkGymEnv

client_id = 0
env_name = "nqos_split"
config_json = load_config_file(env_name)

# run 3 episodes for this environment session
config_json["env_config"]["episodes_per_session"] = 3

env = NetworkGymEnv(client_id, config_json) 
obs, info = env.reset() # environment start

num_steps = 1000
for step in range(num_steps):

    action = env.action_space.sample()
    obs, reward, terminated, truncated, info = env.step(action)

    # If the environment is end, exit
    if terminated:
        break

    # If the epsiode is up (environment still running), then start another one
    if truncated:
        obs, info = env.reset()

# run 1 episode for this environment session
# you may also change other parameters in the config_json file
config_json["env_config"]["episodes_per_session"] = 1

env = NetworkGymEnv(client_id, config_json)
num_steps=1000
obs, info = env.reset() # environment start

for step in range(num_steps):

    action = env.action_space.sample()
    obs, reward, terminated, truncated, info = env.step(action)

    # If the environment is end, exit
    if terminated:
        break

    # If the epsiode is up (environment still running), then start another one
    if truncated:
        obs, info = env.reset()
```
This example demonstrates how to train an agent using two sequential environment sessions, with each session having a specific number of episodes. The provided Python code showcases the process of starting and interacting with the environments using the NetworkGym.