---
hide-toc: true
firstpage:
lastpage:
---

# NetworkGym: Democratizing Network AI Research and Development
NetworkGym is an innovative Simulation-as-a-Service framework designed to democratize Network AI Research and Development. At the core of NetworkGym lies the simplified representation of reinforcement learning known as the "agent-environment loop," which facilitates seamless integration with various network agents.

```{figure} network_gym_intro.png
---
width: 100%
---
```

## Agent
NetworkGym offers four types of network agents, each serving a specific purpose when interacting with NetworkGym Environments:
- **System default agent**: By sending an "empty" action to the environment, this agent enables the collection of data for offline training purposes.
- **Custom algorithm agent**: A flexible option that allows users to define their own specialized agents using the demo code.
- **Stable-Baselines3 network agent**: A powerful RL agent with advanced stability and reliability features.
- **CleanRL network agent**: An efficient and clean RL agent option for custom algorithms.

We have included additional information on how to train the agent in the [Training Agents](tutorials/training_agents.md) tutorial. This tutorial provides detailed guidance and instructions for effectively training your agent using various network agents offered by NetworkGym.


## Gymnasium API
The interaction between the network agent and the environment is facilitated through the standard [Gymnasium](https://gymnasium.farama.org/) API, including action (↓), observation (↑), and reward (↑).

## NetworkGym API
NetworkGym consists of three essential components and two interfaces, each playing a crucial role in the framework:
- **NetworkGym Client**: connects Agent to the Server, configures the Environment, and enables data conversion between Gym and NetworkGym format.
    - **Northbound Interface**: interface between Client and Server, including network configuration (↓), action (↓), and network stats (↑).
- **NetworkGym Server**: manages connections between Clients and Environments.
    - **Southbound Interface**: interface between Server and Environment, including network configuration (↓), action (↓), and network stats (↑).
- **NetworkGym Environment**: simulates or implements network environments tailored for various network AI use cases. E.g., open-source network simulation tools such as [ns-3](https://www.nsnam.org/).

For more comprehensive information about the key components of NetworkGym, please refer to the [Overview](content/overview.md) page. 

```{admonition} Why Seperating Agent/Client and Server/Environment?
The Agent/Client component is designed for open-source utilization, whereas the Server/Environment component is maintained by us as a service. The Agent/Client connects to the Server/Environment via the public internet. Multiple convincing rationales underlie the decision to divide NetworkGym into these distinct Agent/Client and Server/Environment components.
- **Language and Platform Flexibility**: Separating the Agent and Environment allows for the use of different programming languages. For instance, a Python-based Agent can seamlessly interact with a C++(ns-3) based simulation Environment. This flexibility enables the integration of diverse tools and technologies.
- **Controlled Access through the NetworkGym API**: Since message exchanges occur via the networkgym API, the user is not granted direct access to the simulator or the underlying implementation of the network environment.
- **Decoupling Deployment**: By decoupling the Agent and Environment, they can be deployed on separate machines or platforms optmized for specific workloads.
```
## NetworkGym Scope and Limitations

✔️ **In-Scope:** 
- NetworkGym focuses on the development of open and reference AI models/algorithms tailored for networking research (Layer 2 and above). 
- It also involves the development of abstract models, such as channel, compute, power, etc., simulating aspects of the physical world and systems.
- NetworkGym endeavors to create full-stack models that can effectively simulate end-to-end (Access/Edge/Cloud) network operations with high fidelity.

❌ **Out-of-Scope:**
- NetworkGym does not involve the development of proprietary AI algorithms or models. 
- It is not centered around creating AI applications or software, like xApp/rApp, based on specific controller architectures (e.g., O-RAN RIC).
- NetworkGym is not designed to create a Digital Twin to simulate the physical world with high fidelity.

```{toctree}
:hidden:
:caption: Introduction

content/motivation
content/overview
content/quickstart
```

```{toctree}
:hidden:
:caption: API

api/network_gym_client
api/network_gym_server
api/network_gym_env
```

```{toctree}
:hidden:
:caption: Environments

environments/mx_traffic_management
environments/mx_network_slicing
```

```{toctree}
:hidden:
:caption: Tutorials

tutorials/handling_time_limits
tutorials/training_agents
tutorials/customizing_observation_and_reward
tutorials/implementing_custom_environment
Gymnasium Tutorial <https://gymnasium.farama.org/tutorials/gymnasium_basics/>

```

```{toctree}
:hidden:
:caption: Development

Github <https://github.com/IntelLabs/networkgym>
Contribute to the Docs <https://github.com/IntelLabs/networkgym/tree/main/docs>
contact

```