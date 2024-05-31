#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_client_demo.py

from network_gym_client import load_config_file
from network_gym_client import Env as NetworkGymEnv

client_id = 0
env_name = "nqos_split"
config_json = load_config_file(env_name)
config_json["rl_config"]["agent"] = "system_default"
# Create the environment
env = NetworkGymEnv(client_id, config_json) # make a network env using pass client id and configure file arguments.

num_steps = 1000
obs, info = env.reset()

for step in range(num_steps):

    #action = env.action_space.sample()  # agent policy that uses the observation and info.
    action = [] # environment will use system default is no action is provided.
    obs, reward, terminated, truncated, info = env.step(action)

    # If the environment is end, exit
    if terminated:
        break

    # If the episode is up (environment still running), then start another one
    if truncated:
        obs, info = env.reset()


