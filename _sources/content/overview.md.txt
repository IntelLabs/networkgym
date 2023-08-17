---
title: Overview
---
# Overview

## NetworkGym Components and Interfaces

The NetworkGym framework consists of three essential components - the **Client**, the **Server**, and the **Environment** - each playing a crucial role in the system. Additionally, it encompasses two interfaces - the **Northbound** and the **Southbound** Interfaces, facilitating seamless communication and interaction within the framework.

Displayed below is a graphical depiction of the NetworkGym architecture:

```{mermaid}
flowchart TB

subgraph network_gym_server
northbound <--> southbound[[southbound_interface]]
end

subgraph network_gym_env
southbound_interface <--> configure
southbound_interface <--> simulator
southbound_interface <-..-> emulator
southbound_interface <-..-> testbed
end

agent <--> gymnasium.env

gymnasium.env -- action --> adapter
adapter -- obs,rewards --> gymnasium.env

subgraph network_gym_client
gymnasium.env
adapter
northbound_interface[[northbound_interface]]
end



adapter --policy--> northbound_interface
northbound_interface --network_stats--> adapter


northbound_interface --env_config,policy--> northbound[[northbound_interface]]
northbound --network_stats--> northbound_interface




southbound --env_config,policy--> southbound_interface[[southbound_interface]]
southbound_interface --network_stats--> southbound


click gymnasium.env "../api/network_gym_client/env.html" _blank
style gymnasium.env fill:#1E90FF,color:white,stroke:white

click adapter "../api/network_gym_client/adapter.html" _blank
style adapter fill:#1E90FF,color:white,stroke:white

click northbound_interface "../api/network_gym_client/northbound_interface.html" _blank
style northbound_interface fill:#1E90FF,color:white,stroke:white

click southbound_interface "../api/network_gym_env/southbound_interface.html" _blank
style southbound_interface fill:#1E90FF,color:white,stroke:white

click simulator "../api/network_gym_env/simulator.html" _blank
style simulator fill:#1E90FF,color:white,stroke:white

click configure "../api/network_gym_env/configure.html" _blank
style configure fill:#1E90FF,color:white,stroke:white

```
This visual aid provides an overview of how the diverse components and interfaces of NetworkGym collaborate harmoniously, forming a streamlined and effective Simulation-as-a-Service framework tailored for the advancement of Network AI Research and Development. By clicking on the blue boxes, you can navigate directly to the corresponding API pages for the respective components.

## [Client API](../api/network_gym_client.md)
The NetworkGym client comprises three main elements: a **customized Gymnasium environment**, an **Adapter**, and the **Northbound** interface. The environment-specific Adapter is responsible for transforming the NetworkGym data format into the Gymnasium data format, allowing seamless communication with Gymnasium-compatible agents like stable-baselines3 and cleanRL. The Northbound Interface establishes the connection between the client and the server. It enables the client to select and configure the desired network environment. 

## [Server API](../api/network_gym_server.md)
The NetworkGym server component plays a central role in communication between the client and the environment. It utilizes the **Northbound** interface to interact with the client and the **Southbound** interface to communicate with the environment. Additionally, the server maintains a routing map, which keeps track of each active client and its assigned environment during a connected session.

## [Environment API](../api/network_gym_env.md)
The NetworkGym environment (either a simulator, emulator, or testbed) connects to the server through the **Southbound** interface. At present, the framework supports the **ns-3** based simulator, offering **three** distinct network environments.