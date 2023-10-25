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
import json
import csv

class Adapter(network_gym_client.adapter.Adapter):
    """rmcat env adapter.

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
        self.size_per_feature = int(self.config_json['env_config']['nada_flows'])
        if config_json['env_config']['env'] != self.env:
            sys.exit("[ERROR] wrong environment Adapter. Configured environment: " + str(config_json['env_config']['env']) + " != Launched environment: " + str(self.env))
        
        FILE_PATH = Path(__file__).parent
        #Append the bw_trace to json file.
        data = {}
        # Open a csv reader called DictReader
        with open(FILE_PATH / config_json['env_config']['bw_trace_file'], encoding='utf-8') as csvf:
            csvReader = csv.DictReader(csvf)
            # Convert each row into a dictionary 
            # and add it to data
            for rows in csvReader:
                for key, value in rows.items():
                    if key not in data:
                        data[key] = [float(value)]
                    else:
                        data[key].append(float(value))
               
            config_json['env_config']['bw_trace'] = data

    def get_action_space(self):
        """Get action space for the rmcat env.

        Returns:
            spaces: action spaces
        """
        RMCAT_CC_DEFAULT_RMIN = 150000  # in bps: 150Kbps 
        RMCAT_CC_DEFAULT_RMAX = 1500000. # in bps: 1.5Mbps
        return spaces.Box(low=RMCAT_CC_DEFAULT_RMIN, high=RMCAT_CC_DEFAULT_RMAX,
                                        shape=(self.size_per_feature,), dtype=np.float32)

    #consistent with the get_observation function.
    def get_observation_space(self):
        """Get the observation space for rmcat env.

        Returns:
            spaces: observation spaces
        """
        return spaces.Box(low=0, high=1000,
                                            shape=(self.num_features, self.size_per_feature), dtype=np.float32)
    
    def get_observation(self, df):
        """Prepare observation for rmcat env.

        This function should return the same number of features defined in the :meth:`get_observation_space`.

        Args:
            df (pd.dataframe): network stats measurement

        Returns:
            spaces: observation spaces
        """
        #print (df)
        row_loglen = None
        row_qdel = None
        row_rtt = None
        row_ploss = None
        row_plr = None
        row_xcurr = None
        row_rrate = None
        row_srate = None

        rtt_value = np.empty(self.size_per_feature, dtype=object)
        xcurr_value = np.empty(self.size_per_feature, dtype=object)
        rrate_value = np.empty(self.size_per_feature, dtype=object)

        for index, row in df.iterrows():
            if row['source'] == 'rmcat':
                if row['name'] == 'loglen':
                    row_loglen = row
                elif row['name'] == 'qdel':
                    row_qdel = row
                elif row['name'] == 'rtt':
                    row_rtt = row
                    rtt_value = row['value']
                    self.action_data_format = row
                elif row['name'] == 'ploss':
                    row_ploss = row
                elif row['name'] == 'plr':
                    row_plr = row
                elif row['name'] == 'xcurr':
                    row_xcurr = row
                    xcurr_value = row['value']
                elif row['name'] == 'rrate':
                    row_rrate = row
                    rrate_value = row['value']
                elif row['name'] == 'srate':
                    row_srate = row
        
        
        self.wandb_log_buffer_append(self.df_to_dict(row_loglen)) 
        
        self.wandb_log_buffer_append(self.df_to_dict(row_qdel))

        self.wandb_log_buffer_append(self.df_to_dict(row_rtt))

        self.wandb_log_buffer_append(self.df_to_dict(row_ploss)) 
        
        self.wandb_log_buffer_append(self.df_to_dict(row_plr))

        self.wandb_log_buffer_append(self.df_to_dict(row_xcurr))

        self.wandb_log_buffer_append(self.df_to_dict(row_rrate)) 
        
        self.wandb_log_buffer_append(self.df_to_dict(row_srate))

        observation = np.vstack([rtt_value, rrate_value, xcurr_value])
        print('Observation --> ' + str(observation))
        return observation

    def get_policy(self, action):
        """Prepare policy for the rmcat env.

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
        """Prepare reward for the rmcat env.

        Args:
            df (pd.DataFrame): network stats

        Returns:
            spaces: reward spaces
        """

        #TODO: add a reward function for you rmcat env
        reward = 0

        # send info to wandb
        self.wandb_log_buffer_append({"reward": reward})

        return reward