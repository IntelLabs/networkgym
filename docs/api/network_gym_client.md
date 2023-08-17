---
title: network_gym_client
---

# network_gym_client

```{toctree}
:hidden:
network_gym_client/env
network_gym_client/adapter
network_gym_client/northbound_interface
```

```{mermaid}
flowchart TB

agent <--> gymnasium.env

gymnasium.env -- action --> adapter
adapter -- obs,rewards --> gymnasium.env

subgraph network_gym_client
gymnasium.env
adapter
northbound_interface[[northbound Interface]]
end


adapter --env_config,policy--> northbound_interface
northbound_interface --network_stats--> adapter

click gymnasium.env "./network_gym_client/env.html" _blank
style gymnasium.env fill:#1E90FF,color:white,stroke:white

click adapter "./network_gym_client/adapter.html" _blank
style adapter fill:#1E90FF,color:white,stroke:white

click northbound_interface "./network_gym_client/northbound_interface.html" _blank
style northbound_interface fill:#1E90FF,color:white,stroke:white

```

NetworkGym Client includes the three components, a **custom gymnasium.env**, **adapter** and **northbound** interface.
- The **custom gymnasium.env** inherets the environment class of [gymnasium](https://gymnasium.farama.org/) and communicates with the agent using the standard gymnasium interfaces. E.g., exchange obs, reward and action in the reset() and step() functions.
```python
import gymnasium as gym
class Env(gym.Env):
    """Custom Environment that follows gym interface."""

    def __init__(self, arg1, arg2, ...):
        super().__init__()
        ...

    def step(self, action):
        ...
        return observation, reward, terminated, truncated, info

    def reset(self, seed=None, options=None):
        ...
        return observation, info
```
- The **adapter** transform the data format from gymnasium to network_gym or the other way around. E.g., it transforms network stats to obs and reward, and changes action to policy.
- The **northbound** interface connects the client to the server, configure the environment parameters, communicate network stats and policy between client and network_gym envrionment.