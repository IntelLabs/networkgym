#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : northbound_interface.py

import zmq
import sys
import threading
import time
from random import randint, random
import json
import pandas as pd

class MeasurementReport:
    """Data structure to store the network stats measurement report.
    """
    def __init__(self, ok_flag, df_list):
        """Initialize measurement report.

        Args:
            ok_flag (Bool): indicate whehter the measurement is valid.
            df_list (list[pandas.dataframe]): a list of dataframe that stores the network ststs
        """
        self.ok_flag = ok_flag
        self.df_list = df_list

class NorthboundInterface():
    """NetworkGym northbound interface client.
    
    Northbound interface connects the network gym client to the network gym server. Client sends the network policy to the Sever/Env.
    Sever/Env replies the network stats to the Client.

    """
    def __init__(self, id, config_json):
        """Initialize NorthboundInterface.

        Args:
            id (int): client ID
            config_json (json): configuration file
        """
        self.identity = u'%s-%d' % (config_json["session_name"], id)
        self.config_json=config_json
        self.socket = None
        self.end_ts = None # sync the timestamp between obs and action

    #connect to network gym server using ZMQ socket
    def connect(self):
        """Connect to the network gym server.
        """
        context = zmq.Context()
        self.socket = context.socket(zmq.DEALER)
        self.socket.plain_username = bytes(self.config_json["session_name"], 'utf-8')
        self.socket.plain_password = bytes(self.config_json["session_key"], 'utf-8')
        self.socket.identity = self.identity.encode('utf-8')
        self.socket.connect('tcp://localhost:'+str(self.config_json["algorithm_client_port"]))
        print('%s started' % (self.identity))
        print(self.identity + " Sending GMASim Start Request…")
        if self.config_json["session_name"] == "test":
            print("If no reposne after sending the start requst, the port forwarding may be broken...")
        else:
            print("If no response from the server, it could be the session_name and session_key is wrong or the port forwardng is broken. "
             +"You may change the 'session_name' and 'session_key' to 'test' to test port fowarding")
            
        gma_start_request = self.config_json["env_config"]
        self.socket.send(json.dumps(gma_start_request, indent=2).encode('utf-8'))#send start simulation request

    #send action to network gym server
    def send (self, policy):
        """Send the Policy to the server and environment.

        Args:
            policy (json): network policy
        """

        if self.config_json['env_config']['respond_action_after_measurement']:
            action_json = self.config_json["action_template"] #load the action format from template

            action_json["action_list"] = policy
            #print(action_json)
            json_str = json.dumps(action_json, indent=2)
            #print(identity +" Send: "+ json_str)
            self.socket.send(json_str.encode('utf-8')) #send action

    #receive a msg from network gym server
    def recv (self):
        """Receive a message from the network gym server.

        Returns:
            MeasurementReport: the network stats :class:`network_gym_client.MeasurementReport` from the environment
        """

        reply = self.socket.recv()
        relay_json = json.loads(reply)

        #print(relay_json)        

        if relay_json["type"] == "no-available-worker":
            # no available network gym worker, retry the request later
            print(self.identity+" Receive: "+reply.decode())
            print(self.identity+" "+"retry later...")
            quit()

        #elif relay_json["type"] == "env-end":
        #    # simulation end from the network gym server
        #    print(self.identity +" Receive: "+ reply.decode())
        #    print(self.identity+" "+"Simulation Completed.")
        #    #quit() quit the program in main function.
        #
        #    return None

        elif  relay_json["type"] == "env-measurement":
            return self.process_measurement(relay_json)

        elif relay_json["type"] == "env-error":
            # error happened. Check the error msg.
            print(self.identity +" Receive: "+ reply.decode())
            print(self.identity +" "+ "Simulation Stopped with ***[Error]***!")
            quit()
        else:
            # Unkown msg type, please check.This should not happen. 
            print(self.identity +" Receive: "+ reply.decode())
            print(self.identity +" "+ "***[ERROR]*** unkown msg type!")
            quit()
     
    def process_measurement (self, reply_json):
        df = pd.json_normalize(reply_json['metric_list']) 
        if 'end_ts' in df.columns:
            self.end_ts = int(df["end_ts"][0])
        ok_flag = True
        return MeasurementReport(ok_flag, df)

    '''
    #process measurement from network gym server
    def process_measurement (self, reply_json):
        """Process the raw network measurements (json) and translate them to pandas.dataframe format.

        Args:
            reply_json (json): the message from the server

        Returns:
            MeasurementReport: the network stats :class:`network_gym_client.MeasurementReport` from the environment
        """
        df_list = []
        ok_flag = True
        df = pd.json_normalize(reply_json['metric_list']) 
        # print(df)

        if self.config_json['env_config']['downlink']:
            df = df[df['direction'] == 'DL'].reset_index(drop=True)
        else:
            df = df[df['direction'] == 'UL'].reset_index(drop=True)
        
        df_phy = df[df['group'] == 'PHY'].reset_index(drop=True)
        df_phy_lte = df_phy[df_phy['cid'] == 'LTE'].reset_index(drop=True)

        df_phy_lte_max_rate = []
        df_phy_wifi_max_rate = []

        if not df_phy_lte.empty:
            # process PHY LTE measurement

            df_phy_lte_start_ts = df_phy_lte[df_phy_lte['name'] == 'start_ts'].reset_index(drop=True)
            df_phy_lte_end_ts = df_phy_lte[df_phy_lte['name'] == 'end_ts'].reset_index(drop=True)

            #check PHY LTE timestamps of all users are the same
            if 1==len(set(df_phy_lte_start_ts['value'])) and 1==len(set(df_phy_lte_end_ts['value'])):
                start_ts = df_phy_lte_start_ts['value'][0]
                end_ts = df_phy_lte_end_ts['value'][0]
                self.end_ts = end_ts
                df_phy_lte_max_rate = df_phy_lte[df_phy_lte['name'] == 'max_rate'].reset_index(drop=True)
                df_phy_lte_max_rate.insert(0,'end_ts', end_ts)
                df_phy_lte_max_rate.insert(0,'start_ts', start_ts)
                df_phy_lte_max_rate['unit'] = 'mbps'
                #print(df_phy_lte_max_rate)

                df_phy_lte_slice_id = df_phy_lte[df_phy_lte['name'] == 'slice_id'].reset_index(drop=True)
                df_phy_lte_slice_id.insert(0,'end_ts', end_ts)
                df_phy_lte_slice_id.insert(0,'start_ts', start_ts)
                #print(df_phy_lte_slice_id)

                df_phy_lte_rb_usage = df_phy_lte[df_phy_lte['name'] == 'rb_usage'].reset_index(drop=True)
                df_phy_lte_rb_usage.insert(0,'end_ts', end_ts)
                df_phy_lte_rb_usage.insert(0,'start_ts', start_ts)
                df_phy_lte_rb_usage['unit'] = '%'
                # print(df_phy_lte_rb_usage)

            else:
                print(self.identity+" "+"ERROR, PHY LTE timestamp is not the same")

        df_phy_wifi = df_phy[df_phy['cid'] == 'Wi-Fi'].reset_index(drop=True)
        if not df_phy_wifi.empty:
            # process PHY Wi-Fi measurement
            df_phy_wifi_start_ts = df_phy_wifi[df_phy_wifi['name'] == 'start_ts'].reset_index(drop=True)
            df_phy_wifi_end_ts = df_phy_wifi[df_phy_wifi['name'] == 'end_ts'].reset_index(drop=True)

            #check PHY Wi-Fi timestamps of all users are the same
            if 1==len(set(df_phy_wifi_start_ts['value'])) and 1==len(set(df_phy_wifi_end_ts['value'])):
                start_ts = df_phy_wifi_start_ts['value'][0]
                end_ts = df_phy_wifi_end_ts['value'][0]
                self.end_ts = end_ts
                df_phy_wifi_max_rate = df_phy_wifi[df_phy_wifi['name'] == 'max_rate'].reset_index(drop=True)
                df_phy_wifi_max_rate.insert(0,'end_ts', end_ts)
                df_phy_wifi_max_rate.insert(0,'start_ts', start_ts)
                df_phy_wifi_max_rate['unit'] = 'mbps'
                #print(df_phy_wifi_max_rate)

            else:
                print(self.identity+" "+"ERROR, PHY LTE timestamp is not the same")

        df_gma = df[df['group'] == 'GMA'].reset_index(drop=True)
        if not df_gma.empty:
            # process GMA measurement
            df_gma_start_ts = df_gma[df_gma['name'] == 'start_ts'].reset_index(drop=True)
            df_gma_end_ts = df_gma[df_gma['name'] == 'end_ts'].reset_index(drop=True)

            #check GMA timestamps of all users are the same
            if 1==len(set(df_gma_start_ts['value'])) and 1==len(set(df_gma_end_ts['value'])):
                start_ts = df_gma_start_ts['value'][0]
                end_ts = df_gma_end_ts['value'][0]

                self.end_ts = end_ts
                df_load = df_gma[df_gma['name'] == 'tx_rate'].reset_index(drop=True)
                df_load.insert(0,'end_ts', end_ts)
                df_load.insert(0,'start_ts', start_ts)
                df_load['unit'] = 'mbps'
                #print(df_load)

                df_rate = df_gma[df_gma['name'] == 'rate'].reset_index(drop=True)
                #df_rate = df_rate[df_rate['cid'] == 'All'].reset_index(drop=True)

                df_rate.insert(0,'end_ts', end_ts)
                df_rate.insert(0,'start_ts', start_ts)
                df_rate['unit'] = 'mbps'
                #print(df_rate)

                df_qos_rate = df_gma[df_gma['name'] == 'qos_rate'].reset_index(drop=True)

                df_qos_rate.insert(0,'end_ts', end_ts)
                df_qos_rate.insert(0,'start_ts', start_ts)
                df_qos_rate['unit'] = 'mbps'
                #print(df_qos_rate)

                df_owd = df_gma[df_gma['name'] == 'owd'].reset_index(drop=True)
                #df_owd = df_owd[df_owd['cid'] == 'All'].reset_index(drop=True)
                df_owd.insert(0,'end_ts', end_ts)
                df_owd.insert(0,'start_ts', start_ts)
                df_owd['unit'] = 'ms'
                #print(df_owd)

                df_max_owd = df_gma[df_gma['name'] == 'max_owd'].reset_index(drop=True)
                df_max_owd.insert(0,'end_ts', end_ts)
                df_max_owd.insert(0,'start_ts', start_ts)
                df_max_owd['unit'] = 'ms'
                #print(df_max_owd)

                df_split_ratio = df_gma[df_gma['name'] == 'split_ratio'].reset_index(drop=True)
                df_split_ratio.insert(0,'end_ts', end_ts)
                df_split_ratio.insert(0,'start_ts', start_ts)
                #print(df_split_ratio)

                df_ap_id = df_gma[df_gma['name'] == 'ap_id'].reset_index(drop=True)
                df_ap_id.insert(0,'end_ts', end_ts)
                df_ap_id.insert(0,'start_ts', start_ts)
                #print(df_ap_id)

                df_delay_violation = df_gma[df_gma['name'] == 'delay_violation'].reset_index(drop=True)
                df_delay_violation.insert(0,'end_ts', end_ts)
                df_delay_violation.insert(0,'start_ts', start_ts)
                df_delay_violation['unit'] = '%'

                #print(df_delay_violation)

                df_ok = df[df['name'] == 'measurement_ok'].reset_index(drop=True)
                df_ok.insert(0,'end_ts', end_ts)
                df_ok.insert(0,'start_ts', start_ts)
                
                if(df_ok['value'].min() < 1):
                    #print("[WARNING], some users may not have a valid measurement, for qos_steering case, the qos_test is not finished before a measurement return...")
                    #print(df_ok)
                    ok_flag = False
            else:
                print(self.identity+" "+"ERROR, GMA timestamp is not the same")
        
        #return True, df_phy_lte_max_rate, df_phy_wifi_max_rate, df_load, df_rate, df_qos_rate, df_owd, df_split_ratio

        df_list.append(df_phy_lte_max_rate)
        df_list.append(df_phy_wifi_max_rate)
        df_list.append(df_load)
        df_list.append(df_rate)
        df_list.append(df_qos_rate)
        df_list.append(df_owd)
        df_list.append(df_split_ratio)
        df_list.append(df_ap_id)
        df_list.append(df_phy_lte_slice_id)
        df_list.append(df_phy_lte_rb_usage)
        df_list.append(df_delay_violation)
        df_list.append(df_max_owd)

        return MeasurementReport(ok_flag, df_list)
        '''