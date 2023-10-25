---
title: Quickstart
---

# Quickstart

## Accessing the NetworkGym Service via vLab
```{figure} ../tutorials/network_gym_overview.png
---
width: 40%
---
```
We provide access to NetworkGym Server/Environment through **vLab machines** [(Requst Access)](https://registration.intel-research.net/), facilitating collaboration with MLWiNS/RINGs universities. The NetworkGym Northbound Interface allows a NetworkGym Client to establish a connection with the NetworkGym Environment, either through the public internet or Intel's internal network. In case of any access issues, feel free to reach out to us at [netaigym@gmail.com](mailto:netaigym@gmail.com).

Additionally, we plan to launch NetworkGym Sim-aaS via **Intel DevCloud**, making it available to all Intel developers.
Meanwhile, we are actively collaborating with the research community to enhance NetworkGym Simulator with new use-cases and capabilities, such as 5G/O-RAN, distributed computing, RAN energy saving, predictive QoS, and more.

## Basic Usage

```{figure} network_gym_workflow.png
---
width: 100%
---
```

Once you have gained access to vLab machines, you can begin by downloading the [Client](https://github.com/IntelLabs/networkgym) to initiate your simulations.

```{admonition} ▶️ Upon starting the Client, the following series of steps occur:
1. The Client initiates the process by sending a JSON configuration file to the Server, prompting the launch of an ns-3 simulation.
```

```{admonition} 🔁 During the simulation, the process repeats as follows:
2. The Simulator collects measurement metrics and communicates them to the Server.
    - The Server forwards the measurements to the Client using the NetworkGym NorhtBound (NB) Interface.
3. An Algorithm Agent, which could be AI or non-AI, computes an action based on the received measurements and stores relevant data in WandB.
4. The Client transmits the computed action to the Simulator through the NetworkGym NB and SouthBound (SB) interface, enabling the simulation to continue with the new action.
```

```{admonition} ⏹️ When the NetworkGym or the simulation concludes:
5. The data stored in Wandb can be conveniently visualized on the Wandb website.
```
The provided python code below demonstrates how to interact with NetworkGym environments.

### Initializing Environments

Initializing environments in NetworkGym is straightforward and can be done via the `network_gym_client` package:

```python
from network_gym_client import Env as NetworkGymEnv
# Create a network env using the client_id and configure file arguments.
env = NetworkGymEnv(client_id, config_json) 
```

This will return an ``Env`` for users to interact with.

### Interacting with the Environment

The classic "agent-environment loop" is implemented using the following code:

```python
from network_gym_client import load_config_file
from network_gym_client import Env as NetworkGymEnv

client_id = 0
env_name = "nqos_split"
config_json = load_config_file(env_name)

# Create a network env using the client_id and configure file arguments.
env = NetworkGymEnv(client_id, config_json) 

num_steps = 1000
obs, info = env.reset()

for step in range(num_steps):

    action = env.action_space.sample()  # agent policy that uses the observation and info
    obs, reward, terminated, truncated, info = env.step(action)

    # If the environment is end, exit
    if terminated:
        break

    # If the epsiode is up (environment still running), then start another one
    if truncated:
        obs, info = env.reset()
```

### Explaining the code

First, the configuration file is loaded using the `load_config_file` function by providing the selected environment name. Second, an environment is created using `NetworkGymEnv` class with the additional keywords `client_id` and `config_json`. A `client_id` is associated with a dedicated environment worker until the environment session ends. A client may use different `client_id` to launch parallel environments. After initializing the environment, we `reset` the environment to obtain the first observation of the environment.

Next, the agent performs an action in the environment, `step`. As a result, the agent receives a new observation from the updated environment along with a reward for taking the action. One such action-observation exchange is referred to as a timestep.

In NetworkGym, an environment includes multiple episodes. The terminal state for an episode and environment is signaled using the truncated and terminated signals returned by `step`. For example, after a fixed number of timesteps, an episode is ended by issuing a truncated signal. After multiple episodes, the environment terminates with a terminated signal. If `truncated` is `True`, then reset should be called next to restart the episode. If the `terminated` is `True`, then the client needs to restart the environment. For scenarios where sequential training is essential, the agent can initiate multiple environment sessions in succession. Further insights into effectively managing time constraints can be found in the [Handling Time Limits](../tutorials/networkgym_basics/handling_time_limits.md). An illustrative example is presented below.

```{mermaid}
sequenceDiagram

    agent->>+env: Environment Start (episodes=3)
    env-->>agent: Episode End (Truncation=True, Termination=False)
    env-->>agent: Episode End (Truncation=True, Termination=False)
    env-->>-agent: Episode and Environment End (Truncation=True, Termination=True)

    agent->>+env: Environment Start (episodes=1)
    env-->>-agent: Episode and Environment End (Truncation=True, Termination=True)

```


### Action and observation spaces

Every environment specifies the format of valid actions and observations with the `env.action_space` and `env.observation_space` attributes. This helps in understanding the expected input and output of the environment, as all valid actions and observations should be contained within the respective space.

In the provided example, we sampled random actions via `env.action_space.sample()` instead of using an agent policy, mapping observations to actions as users may prefer. Additionally, some of the NetworkGym environments have system default algorithms, which can be enabled by sending an "empty" action to the environment, as shown below:

```python
    # No action from the RL agent -> the environment will use the system default policy
    obs, reward, terminated, truncated, info = env.step([])
```

Every environment should have the attributes `action_space` and `observation_space`, both of which should be instances of classes that inherit from `Space`. Gymnasium supports a majority of possible spaces users might need:
- ``Box``: describes an n-dimensional continuous space. It's a bounded space where we can define the upper and lower
  limits which describe the valid values our observations can take.
- ``Discrete``: describes a discrete space where {0, 1, ..., n-1} are the possible values our observation or action can take.
  Values can be shifted to {a, a+1, ..., a+n-1} using an optional argument.
- ``Dict``: represents a dictionary of simple spaces.
- ``Tuple``: represents a tuple of simple spaces.
- ``MultiBinary``: creates an n-shape binary space. Argument n can be a number or a list of numbers.
- ``MultiDiscrete``: consists of a series of ``Discrete`` action spaces with a different number of actions in each element.


### Modifying the environment

NetworkGym also supports the [Wrappers](https://gymnasium.farama.org/content/basic_usage/#modifying-the-environment) provided by Gymnasium, enabling the customization of environments with additional functionalities. Some examples include:
- `TimeLimit`: Issue a truncated signal if a maximum number of timesteps has been exceeded (or the base environment has issued a truncated signal).
- `ClipAction`: Clip the action such that it lies in the action space (of type `Box`).
- `RescaleAction`: Rescale actions to lie in a specified interval
- `TimeAwareObservation`: Add information about the index of timestep to observation. In some cases helpful to ensure that transitions are Markov.
- `FlattenObservation`: Observation wrapper that flattens the observation.
- `NormalizeObservation`: This wrapper will normalize observations s.t. each coordinate is centered with unit variance.

```python
# wrap the env with NormalizeObservation
env = NetworkGymEnv(args.client_id, config_json)
normal_obs_env = NormalizeObservation(env)
```
