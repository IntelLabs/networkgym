---
hide-toc: true
firstpage:
lastpage:
---

# NetworkGym: Revolutionizing Network AI Research and Development
NetworkGym is an innovative Simulation-as-a-Service framework designed to transform Network AI Research and Development. At the core of NetworkGym lies the simplified representation of reinforcement learning known as the "agent-environment loop," which facilitates seamless integration with various network agents.

![network_gym](network_gym.png)

## Network Agent Options
NetworkGym offers four types of network agents, each serving a specific purpose when interacting with NetworkGym Environments:
- **System default agent**: By sending an "empty" action to the environment, this agent enables the collection of data for offline training purposes.
- **Custom algorithm agent**: A flexible option that allows users to define their own specialized agents using the demo code.
- **Stable-Baselines3 network agent**: A powerful RL agent with advanced stability and reliability features.
- **CleanRL network agent** *(Link to be provided later)*: An efficient and clean RL agent option for custom algorithms.

We have included additional information on how to train the agent in the [Training Agents](tutorials/training_agents.md) tutorial. This tutorial provides detailed guidance and instructions for effectively training your agent using various network agents offered by NetworkGym.


## Gymnasium API
The interaction between the network agent and the environment is facilitated through the standard [Gymnasium](https://gymnasium.farama.org/) based action and observation space API.

## Key Components of NetworkGym
NetworkGym consists of three essential components, each playing a crucial role in the framework:
- **NetworkGym Client**: This Python-based interface provides a Gymnasium-based API that enables the network agent to remotely interact with the NetworkGym Environment. Additionally, it offers its own NetworkGym API to configure the parameters of the NetworkGym Environment.
- **NetworkGym Server**: The server acts as the intermediary, managing connections between Clients and Environments.
- **NetworkGym Environment**:  Leveraging open-source network simulation tools such as [ns-3](https://www.nsnam.org/), NetworkGym enhances these tools with customized capabilities and use-cases, such as Traffic Steering, Network Slicing, Distributed Compute, Dynamic QoS, Energy Saving, and more. This enables comprehensive and realistic network simulations to support cutting-edge research and development in the field of Network AI.

For more comprehensive information about the key components of NetworkGym, please refer to the [Overview](content/overview.md) page. 

### NetworkGym: Why Divide?
There are several compelling reasons for dividing NetworkGym into three distinct components:
- **Language and Platform Flexibility**: Separating the Client and Environment allows for the use of different programming languages. For instance, a Python-based agent can seamlessly interact with a C++(ns-3) based simulation environment. This flexibility enables the integration of diverse tools and technologies.
- **Decoupling Deployment**: By decoupling the Client and Environment, they can be deployed on separate machines or platforms. This means an agent deployed on a machine optimized for ML workloads can efficiently interact with an Environment deployed on a machine optimized for simulation workloads. This separation optimizes the performance and resource allocation of each component.

### NetworkGym: Scope and Limitations

#### ✔️ In-Scope 
- NetworkGym focuses on the development of **open and reference** AI models/algorithms tailored for **networking research (Layer 2 & above)**. 
- It also involves the development of **abstract models**, such as channel, compute, power, etc., simulating aspects of the physical world and systems.
- NetworkGym endeavors to create **full-stack models** that can effectively simulate **end-to-end (Access/Edge/Cloud)** network operations with high fidelity.

#### ❌ Out-of-Scope
- NetworkGym does not involve the development of **proprietary** AI algorithms or models. 
- Develop AI **application or software**, e.g. xApp/rApp, etc., based on controller architecture, e.g. O-RAN RIC, etc.
- It is not centered around creating AI **applications or software**, like xApp/rApp, based on specific controller architectures (e.g., O-RAN RIC).
- NetworkGym is not designed to create a **"Digital Twin"** to simulate the physical world with high fidelity.

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
Gymnasium Tutorial <https://gymnasium.farama.org/tutorials/gymnasium_basics/>

```

```{toctree}
:hidden:
:caption: Development

Github <https://github.com/IntelLabs/gma/tree/network-gym>
Contribute to the Docs <https://github.com/IntelLabs/gma/tree/network-gym/docs>
```