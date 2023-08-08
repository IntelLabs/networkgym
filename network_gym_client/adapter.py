#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : adapter.py

import sys
import wandb
import numpy as np
import pandas as pd
import json

class Adapter:
    """The base class for environment data format adapter.

    This class is an data format "adapter" between the gymnasium environment and network_gym environment.
    It transforms the network stats measurements (json) to obs and reward (Spaces).
    It also transforms the action (Spaces) to a policy (json) that can be applied to the network.
    """
    def __init__(self, config_json):
        """Initialize Adapter.
        
        Args:
            config_json (json): the configuration file
        """
        self.config_json = None
        self.wandb_log_buffer = None
        self.wandb = wandb
        self.config_json = config_json

        rl_alg = config_json['rl_config']['agent'] 

        config = {
            "policy_type": "MlpPolicy",
            "env_id": "network_gym_client",
            "RL_algo" : rl_alg
        }

        self.wandb.init(
            # name=rl_alg + "_" + str(config_json['env_config']['num_users']) + "_LTE_" +  str(config_json['env_config']['LTE']['resource_block_num']),
            #name=rl_alg + "_" + str(config_json['env_config']['num_users']) + "_" +  str(config_json['env_config']['LTE']['resource_block_num']),
            name=rl_alg,
            project="network_gym_client",
            config=config,
            sync_tensorboard=True,  # auto-upload sb3's tensorboard metrics
            # save_code=True,  # optional
        )
    
    def wandb_log_buffer_append (self, info):
        """Add to wandb log buffer, the info will be send to wandb later in the :meth:`wandb_log` function

        Args:
            info (dict): information to append to the buffer.
        """
        if info:
            # info not empty!
            if not self.wandb_log_buffer:
                self.wandb_log_buffer = info
            else:
                self.wandb_log_buffer.update(info)

    def wandb_log (self):
        """Send the log information to WanDB.
        """
        # send info to wandb
        #print(self.wandb_log_buffer)
        self.wandb.log(self.wandb_log_buffer)
        self.wandb_log_buffer = None

    def df_to_dict(self, df, description):
        """Transform datatype from pandas.dataframe to dictionary.

        Args:
            df (pandas.dataframe): a pandas.dataframe object
            description (string): a descritption for the data

        Returns:
            dict : converted data with dictionary format
        """
        df_cp = df.copy()
        df_cp['user'] = df_cp['user'].map(lambda u: f'UE{u}_'+description)
        # Set the index to the 'user' column
        df_cp = df_cp.set_index('user')
        # Convert the DataFrame to a dictionary
        data = df_cp['value'].to_dict()
        return data

    def fill_empty_feature(self, feature, value):
        """Fill the  missing measurements with a input value

        Args:
            feature (pd.DataFrame): feature from the measurement
            value (int): the value to fill the missing measurement

        Returns:
            list: results after replace missing measurements with value
        """
        
        #Fill the missing data with the input value.
        
        if len(feature) > self.config_json['env_config']['num_users']:
            print ("[WARNING] This feature has more user than input!!")
            print(feature)
        elif len(feature) == self.config_json['env_config']['num_users']:
            # measurement size match the user number
            return feature[:]["value"]
        if len(feature)> 0:
            # some of the user's data are missing, fill with input value.
            print("[WARNING] some users of a feature returns empty measurement.")
            feature = feature.set_index("user")
            feature = feature.reindex(list(range(self.config_json['env_config']['num_users'])),fill_value=value)
            data = feature[:]["value"]
            return data
        else:
            # all user's data are missing, fill the entire feature will input value.
            print("[WARNING] all users of a feature returns empty measurement.")
            emptyFeatureArray = np.empty([self.config_json['env_config']['num_users'],], dtype=int)
            emptyFeatureArray.fill(value)
            return emptyFeatureArray
    def get_nested_json_policy (self, action_name, tags, action, user_name='user'):
        """Convert the gymnasium action space to nested json format

        Args:
            action_name (str): name of the action
            tags (dict): custom tags for this action
            action (Spaces): action from the rl agent

        Returns:
            json: a nested json policy for the network
        """

        data = []
        for user_id in range(len(action)):
            data.append([user_id, action[user_id]])
        df = pd.DataFrame(data, columns=[user_name, 'value'])
        for key, value in reversed(tags.items()):
            df.insert(0, key, value)
        df.insert(0,'name', action_name)# the name of your action.

        # the following code tranform the json to a nested structure for lower overhead.
        group_by_list = list(tags.keys())
        group_by_list.insert(0, 'name')
        json_data = (df.groupby(group_by_list)
                .apply(lambda x: x[[user_name, 'value', ]].to_dict(orient='list'))
                .reset_index()
                .rename(columns={0: ''})
                .to_json(orient='records'))

        policy = json.loads(json_data)
        return policy