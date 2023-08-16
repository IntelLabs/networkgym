---
title: Env
---

# Env

## network_gym_client.Env

```{eval-rst}
.. autoclass:: network_gym_client.Env
```

### Methods

```{eval-rst}
.. autofunction:: network_gym_client.Env.reset
.. autofunction:: network_gym_client.Env.step
```

### Attributes

```{eval-rst}
.. autoattribute:: network_gym_client.Env.action_space

    The Space object corresponding to valid actions, all valid actions should be contained with the space. For example, if the action space is of type `Discrete` and gives the value `Discrete(2)`, this means there are two valid discrete actions: 0 & 1.

    .. code::

        >>> env.action_space
        Discrete(2)
        >>> env.observation_space
        Box(-3.4028234663852886e+38, 3.4028234663852886e+38, (4,), float32)

.. autoattribute:: network_gym_client.Env.observation_space

    The Space object corresponding to valid observations, all valid observations should be contained with the space. For example, if the observation space is of type :class:`Box` and the shape of the object is ``(4,)``, this denotes a valid observation will be an array of 4 numbers. We can check the box bounds as well with attributes.

    .. code::

        >>> env.observation_space.high
        array([4.8000002e+00, 3.4028235e+38, 4.1887903e-01, 3.4028235e+38], dtype=float32)
        >>> env.observation_space.low
        array([-4.8000002e+00, -3.4028235e+38, -4.1887903e-01, -3.4028235e+38], dtype=float32)

.. autoattribute:: network_gym_client.Env.northbound_interface_client

   The Northbound Interface Client object connects and communicates with the server. 

.. autoattribute:: network_gym_client.Env.adapter

   The Environment Adapter object traslate the dataformat between the gymnasium and network_gym. 

```
