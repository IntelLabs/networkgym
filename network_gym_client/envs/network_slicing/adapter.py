#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : adapter.py


import network_gym_client.adapter
import sys
from gymnasium import spaces
import numpy as np
from pathlib import Path
import pandas as pd
import json

class Adapter(network_gym_client.adapter.Adapter):
    """network_slicing environment adapter.

    Args:
        Adapter (network_gym_client.adapter.Adapter): the base class
    """
    def __init__(self, config_json):
        """Initialize the adapter.

        Args:
            config_json (json): the configuration file
        """
        super().__init__(config_json)
        self.env = Path(__file__).resolve().parent.name
        self.size_per_feature = len(self.config_json['env_config']['slice_list'])
        self.num_features = 3

        num_users = 0
        for item in self.config_json['env_config']['slice_list']:
            num_users += item['num_users']
        self.config_json['env_config']['num_users'] = num_users
        
        rbg_size = self.get_rbg_size(self.config_json['env_config']['LTE']['resource_block_num'])
        self.rbg_num = self.config_json['env_config']['LTE']['resource_block_num']/rbg_size

        if config_json['env_config']['env'] != self.env:
            sys.exit("[ERROR] wrong environment Adapter. Configured environment: " + str(config_json['env_config']['env']) + " != Launched environment: " + str(self.env))

    def get_action_space(self):
        """Get action space for network_slicing env.

        Returns:
            spaces: action spaces
        """

        return spaces.Box(low=0, high=1, shape=(self.size_per_feature,), dtype=np.float32)

    
    #consistent with the get_observation function.
    def get_observation_space(self):
        """Get observation space for network_slicing env.
        
        Returns:
            spaces: observation spaces
        """
        
        # for network slicing, the user number is configured using the slice list. Cannot use the argument parser!

        return spaces.Box(low=0, high=1000,
                                shape=(self.num_features, self.size_per_feature), dtype=np.float32)
    
    def get_observation(self, df):
        """Prepare observation for network_slicing env.

        This function should return the same number of features defined in the :meth:`get_observation_space`.

        Args:
            df (pd.DataFrame): the network stats measurements

        Returns:
            spaces: observation spaces
        """
        print (df)

        dl_cell_rb_usage = None
        dl_cell_tx_rate = None
        dl_cell_rate = None
        dl_cell_qos_rate = None
        dl_cell_delay_violation = None

        for index, row in df.iterrows():
            if row['source'] == 'lte':
                if row['name'] == 'dl::cell::max_rate':
                    self.action_data_format = row
                elif row['name'] == 'dl::cell::rb_usage':
                    dl_cell_rb_usage = row
            elif row['source'] == 'gma':
                if row['name'] == 'dl::cell::tx_rate':
                    dl_cell_tx_rate = row
                elif row['name'] == 'dl::cell::rate':
                    dl_cell_rate = row
                elif row['name'] == 'dl::cell::qos_rate':
                    dl_cell_qos_rate = row
                elif row['name'] == 'dl::cell::delay_violation':
                    dl_cell_delay_violation = row

        self.wandb_log_buffer_append(self.slice_df_to_dict(dl_cell_rb_usage))
        self.wandb_log_buffer_append(self.slice_df_to_dict(dl_cell_tx_rate))
        self.wandb_log_buffer_append(self.slice_df_to_dict(dl_cell_rate))
        self.wandb_log_buffer_append(self.slice_df_to_dict(dl_cell_qos_rate))
        self.wandb_log_buffer_append(self.slice_df_to_dict(dl_cell_delay_violation))

        #warning, need to modify the following if use more than one base stations.
        observation = np.vstack([pd.json_normalize(dl_cell_rate["value"])["value"].to_list(), pd.json_normalize(dl_cell_rb_usage["value"])["value"].to_list(), pd.json_normalize(dl_cell_delay_violation["value"])["value"].to_list()])
        print('Observation --> ' + str(observation))
        return observation

    def get_policy(self, action):
        """Prepare the network policy for network_slicing env.

        Args:
            action (spaces): the action from RL agent

        Returns:
            json: the network policy
        """
        # you may also check other constraints for action... e.g., min, max.
        
        # TODO: the sum of action should be smaller than 1!!!! Therefore the sum of scaled_action is smaller than the rbg_num
        scaled_action= np.interp(action, (0, 1), (0, self.rbg_num/self.size_per_feature))
        scaled_action = np.round(scaled_action).astype(int) # force it to be an integer.

        # this function will convert the action to a nested json format
        policy1 = json.loads(self.action_data_format.to_json())
        policy1["name"] = "drb_allocation"

        for item in policy1["value"]:
            item["value"] = np.zeros(len(scaled_action)).tolist()

        policy2 = json.loads(self.action_data_format.to_json())
        policy2["name"] = "prb_allocation"

        for item in policy2["value"]:
            item["value"] = scaled_action.tolist()

        policy3 = json.loads(self.action_data_format.to_json())
        policy3["name"] = "srb_allocation"

        for item in policy3["value"]:
            item["value"] = list(np.ones(len(scaled_action))*self.rbg_num)

        policy = [policy1, policy2, policy3]
        print('Action --> ' + str(policy))
        return policy

    def get_reward(self, df):
        """Prepare reward for the network_slicing env.

        Args:
            df (pd.DataFrame): network stats measurements

        Returns:
            spaces: reward spaces
        """

        #TODO: add a reward function for you customized env
        reward = 0

        # send info to wandb
        self.wandb_log_buffer_append({"reward": reward})

        return reward

    def slice_df_to_dict(self, df, id_name='id'):
        """Transform datatype from pandas.dataframe to dictionary.

        Args:
            df (pandas.dataframe): a pandas.dataframe object
            description (string): a descritption for the data

        Returns:
            dict : converted data with dictionary format
        """
        if df is None:
            return {}
        description = df['source'] + "::" + df['name']
        get_key = lambda u, v: description+"::"+id_name+f'={u}'+"::slice"+f'={v}'

        id_list = []
        slice_list = []
        value_list= []
        for i, item in enumerate(df['id']):
            for j, sub in enumerate(df['value'][i]['slice']):
                id_list.append(item)
                slice_list.append(sub)
                value_list.append(df['value'][i]['value'][j])

        #print(id_list)
        #print(slice_list)
        #print(value_list)

        dict_key = list(map(get_key, id_list, slice_list))

        #print(dict_key)
        #print(value_list)

        data = dict(zip(dict_key, value_list))
        #print(data)
        return data
    
    def get_rbg_size (self, bandwidth):
        """Compute the resource block group size based on the bandwith (RB number).

        This code is coppied from ns3.
        PF type 0 allocation RBG

        Args:
            bandwidth (int): the resouce block number

        Returns:
            int: the resouce block group size
        """
        # PF type 0 allocation RBG
        PfType0AllocationRbg = [10,26,63,110]      # see table 7.1.6.1-1 of 36.213

        for i in range(len(PfType0AllocationRbg)):
            if (bandwidth < PfType0AllocationRbg[i]):
                return (i + 1)
        return (-1)
