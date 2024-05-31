#Copyright(C) 2024 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : dummy_sim.py


"""
1. Upon launching, the "env_sim" establishes a connection with the server, utilizing the identical identification as the "env_config."
    Subsequently, it commences the exchange of measurement/action messages with the client.
2. Following the conclusion of the "env_sim" simulation, the socket is closed, and the "env_config" reconnects.


Flow Chart:
-------------------------------------------------

env_sim.connect()
[every interval]:
    env_sim -- "env-measurement" --> server
    env_sim <-- "env-action" -- server
env_sim.close()

-------------------------------------------------
"""

import pandas as pd
import json 
import random
import zmq
from network_gym_env.southbound_interface import *

class DummySim:
    """A dummy simulator that generate random measurement samples. When the simulation terminates, resume to the env_config.

        The simulator will create a new env_sim socket using the env_idenntity and connects to the env_port.
        The simulator is configured using the config_json file.
        The first measurement will be send to the client_identity.
    """
    def __init__(self, env_identity, config_json, client_identity, msg_json):
        """Initilize dummy simulator.

        Args:
            env_identity (str): the identity of the environement socket
            config_json (json): env configuration
            client_identity (str): the identity of the client socket who started the env
            msg_json (json): msg from the cliet.
        """
        # use the config_json to config the simulator
        self.interval = msg_json['measurement_interval_ms'] + msg_json['measurement_guard_interval_ms'] # measurement interval
        self.start_ts = msg_json['measurement_start_time_ms'] # start timestamp of a measurement
        self.end_ts = self.start_ts + self.interval # end timestamp of a measurement
        self.sim_end_ts = msg_json['env_end_time_ms']
        self.num_users = msg_json['num_users']
        self.start_simulation(env_identity, config_json, client_identity)

    def start_simulation(self, env_identity, config_json, client_identity):
        """Start simulation. Connect to the server using SouthBound API. Report network stats measurment and receive action.

        Args:
            env_identity (str): the identity of the environement socket
            config_json (json): env configuration
            client_identity (str): the identity of the client socket who started the env
        """
        # open a socket with the same env_identity and connect to the same env_port.

        env_sim = southbound_connect(env_identity, config_json)
        print(env_identity + ': env_sim socket connected.')

        poller = zmq.Poller()
        poller.register(env_sim, flags=zmq.POLLIN)
        
        # running simualtor
        while True:
            dummy_report = json.loads('{"type":"env-measurement"}')
            dummy_report["network_stats"] = self.run_one_interval()

            # the first part of the msg is the client_identity, the second part is the measurement report.
            msg = [client_identity.encode('utf-8'), json.dumps(dummy_report, indent=2).encode('utf-8')]
            env_sim.send_multipart(msg) # send measurement
            #print(identity + ': Send.')
            #print(json_formatted_str)

            if self.end_ts >= self.sim_end_ts:
                # simulation ends. stop the while loop
                break
            
            # simulation continues, wait for a new action.
            # [Warning] always use a poll with timeout before receiving a msg (recv_multipart)! otherwise, it may stuck forever!
            if poller.poll(timeout=30*1000):

                msg = env_sim.recv_multipart() # receives an action
                action_json = json.loads(msg[1])
                if action_json["type"] == "env-action":
                    #dummy simulator do not take any action.
                    action_df = pd.json_normalize(action_json["action_list"]) 
                    print(action_df)
                else:
                    print("Unkown MSG type, Expecting env-action!")
                    error_msg = json.loads('{"type":"env-error", "error_msg": "Unkown MSG type, Expecting env-action!"}')
                    msg[1]=json.dumps(error_msg, indent=2).encode('utf-8')
                    env_sim.send_multipart(msg)
                    break
            else:
                print("Timeout: No env-action received!")
                error_msg = json.loads('{"type":"env-error", "error_msg": "Timeout: No env-action received!"}')
                msg[1]=json.dumps(error_msg, indent=2).encode('utf-8')
                env_sim.send_multipart(msg)
                break
        poller.unregister(env_sim)
        env_sim.close()
        print(env_identity + ': env_sim socket closed.')
    
    def run_one_interval(self):
        """Running the simulator for one interval.

        Returns:
            json: the measurement list
        """
        # you can modify the tags for your own measurement
        tags = {}

        # all tags are optional
        self.start_ts += self.interval
        tags['ts']=self.end_ts
        self.end_ts = self.start_ts + self.interval
        tags['source']='test'        

        output1 = self.generate_dummy_measurement('measurement_1', tags, self.num_users);
        output2 = self.generate_dummy_measurement('measurement_2', tags, self.num_users);
        output3 = self.generate_dummy_measurement('measurement_3', tags, self.num_users);
        output4 = self.generate_dummy_measurement('measurement_4', tags, self.num_users);

        merged_list = output1 + output2 + output3 + output4

        #print(terminated)
        #print(merged_list)
        #json_formatted_str = json.dumps(merged_list, indent=2)
        #print(json_formatted_str)


        #data_recv = pd.json_normalize(merged_list)
        #print(data_recv)

        #data_recv_flat = data_recv.explode(column=['user', 'value'])
        #print(data_recv_flat)
        return merged_list

        
    def generate_dummy_measurement(self, name, tags, num_users):
        """Generate random measurement.

        Args:
            name (str): the name of measurement
            tags (dict): the tags added to the measurement, e.g., timestamps
            num_users (int): the number of users

        Returns:
            json: the measurement results
        """
        #print (name)
        #print (tags)
        #print (user_num)

        data = []
        for id in range(num_users):
            data.append([id, random.randint(3, 9)])
        df = pd.DataFrame(data, columns=['id', 'value'])
        for key, value in reversed(tags.items()):
            df.insert(0, key, value)
        df.insert(0,'name', name)
        #print(df)

        #tranform the json to a nested structure for lower overhead
        group_by_list = list(tags.keys())
        group_by_list.insert(0, 'name')
        json_data = (df.groupby(group_by_list)
                .apply(lambda x: x[['id', 'value', ]].to_dict(orient='list'))
                .reset_index()
                .rename(columns={0: ''})
                .to_json(orient='records'))

        json_object = json.loads(json_data)
        
        return json_object

