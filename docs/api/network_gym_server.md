---
title: network_gym_server
---

# network_gym_server

```{mermaid}
flowchart LR

subgraph network_gym_server
northbound[[northbound Interface]] <--> southbound[[southbound Interface]]
end

```

The NetworkGym Server includes two components, the **northbound** interface manages the connections with the clients, and the **southbound** interface manages the connections with the environments.
When a client connects, the server selects an idle environment instance and add the client to envrionment mapping to the routing table.

```{note}
At present, we do not plan to release the network_gym_server software.
```