#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_client_sequential_training.py

# In this example, we perform sequential training for two environment sessions.
# the first session lasts for 3 episodes and the second session lasts for 1 episodes.

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

    if terminated:
        # environment terminates, exit the for loop
        break

    if truncated:
        # episode truncates, start the next episode
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

    if terminated:
        # environment terminates, exit the for loop
        break

    if truncated:
        # episode truncates, start the next episode
        obs, info = env.reset()