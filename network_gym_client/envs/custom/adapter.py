#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : adapter.py

import network_gym_client.adapter

import sys
from gymnasium import spaces
import numpy as np
import pandas as pd
import json
from pathlib import Path

class Adapter(network_gym_client.adapter.Adapter):
    """Custom env adapter.

    Args:
        Adapter (network_gym_client.adapter.Adapter): base class.
    """
    def __init__(self, config_json):
        """Initialize the adapter.

        Args:
            config_json (json): the configuration file
        """

        super().__init__(config_json)
        self.env = Path(__file__).resolve().parent.name
        self.num_features = 3
        self.size_per_feature = int(self.config_json['env_config']['num_users'])

        if config_json['env_config']['env'] != self.env:
            sys.exit("[ERROR] wrong environment Adapter. Configured environment: " + str(config_json['env_config']['env']) + " != Launched environment: " + str(self.env))

    def get_action_space(self):
        """Get action space for the custom env.

        Returns:
            spaces: action spaces
        """
        return spaces.Box(low=0, high=1,
                                        shape=(self.size_per_feature,), dtype=np.float32)

    #consistent with the get_observation function.
    def get_observation_space(self):
        """Get the observation space for custom env.

        Returns:
            spaces: observation spaces
        """
        return spaces.Box(low=0, high=1000,
                                            shape=(self.num_features, self.size_per_feature), dtype=np.float32)
    
    def get_observation(self, df):
        """Prepare observation for custom env.

        This function should return the same number of features defined in the :meth:`get_observation_space`.

        Args:
            df (pd.dataframe): network stats measurement

        Returns:
            spaces: observation spaces
        """
        print (df)

        df_measurement_1 = np.empty(self.size_per_feature, dtype=object)
        df_measurement_2 = np.empty(self.size_per_feature, dtype=object)
        df_measurement_3 = np.empty(self.size_per_feature, dtype=object)

        for index, row in df.iterrows():
            if row['source'] == 'test':
                if row['name'] == 'measurement_1':
                    df_measurement_1 = row['value']
                    self.action_data_format = row
                elif row['name'] == 'measurement_2':
                    df_measurement_2 = row['value']
                elif row['name'] == 'measurement_3':
                    df_measurement_3 = row['value']
        observation = np.vstack([df_measurement_1, df_measurement_2, df_measurement_3])
        print('Observation --> ' + str(observation))
        return observation

    def get_policy(self, action):
        """Prepare policy for the custom env.

        Args:
            action (spaces): action from the RL agent

        Returns:
            json: network policy
        """
        # you may also check other constraints for action... e.g., min, max.

        policy1 = json.loads(self.action_data_format.to_json())
        policy1["name"] = "srate"
        policy1["value"] = action.tolist()

        print('Action --> ' + str(policy1))
        return policy1

    def get_reward(self, df):
        """Prepare reward for the custom env.

        Args:
            df (pd.DataFrame): network stats

        Returns:
            spaces: reward spaces
        """

        #TODO: add a reward function for you customized env
        reward = 0

        # send info to wandb
        self.wandb_log_buffer_append({"reward": reward})

        return reward