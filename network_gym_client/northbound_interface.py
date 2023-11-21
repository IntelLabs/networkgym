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

class NorthBoundClient():
    """NetworkGym northbound interface client.
    
    Northbound interface connects the network gym client to the network gym server. Client sends the network policy to the Sever/Env.
    Sever/Env replies the network stats to the Client.

    """
    def __init__(self, id, config_json):
        """Initialize NorthBoundClient.

        Args:
            id (int): client ID
            config_json (json): configuration file
        """
        self.identity = u'%s-%d' % (config_json["session_name"], id)
        self.config_json=config_json
        self.socket = None
        self.first_recv = True
        self.context = zmq.Context()
        self.context.setsockopt(zmq.LINGER, 500)

    #connect to network gym server using ZMQ socket
    def connect(self):
        """Connect to the network gym server. Send the Start Env request, where the configuration is loaded from json file.
        """
        self.socket = self.context.socket(zmq.DEALER)
        self.socket.plain_username = bytes(self.config_json["session_name"], 'utf-8')
        self.socket.plain_password = bytes(self.config_json["session_key"], 'utf-8')
        self.socket.identity = self.identity.encode('utf-8')
        self.socket.connect('tcp://localhost:'+str(self.config_json["server_port"]))
        #self.socket.connect('tcp://'+str(self.config_json["server_ip"])+':'+str(self.config_json["server_port"]))

        print('%s started' % (self.identity))
        print(self.identity + " send start request to local port (or forwarded port). Wait 5 seconds before trying the remote server.")
            
        self.gma_start_request = self.config_json["env_config"]
        self.socket.send(json.dumps(self.gma_start_request, indent=2).encode('utf-8'))#send start simulation request

    #send action to network gym server
    def send (self, policy):
        """Send the Policy to the server and environment.

        Args:
            policy (json): network policy
        """
        action_json = {}
        action_json["type"] = "env-action"
        action_json["action_list"] = policy
        #print(action_json)
        json_str = json.dumps(action_json, indent=2)
        #print(identity +" Send: "+ json_str)
        self.socket.send(json_str.encode('utf-8')) #send action

    #receive a msg from network gym server
    def recv (self):
        """Receive a message from the network gym server.

        Returns:
            pd.DataFrame: the network stats measurement from the environment
        """

        if self.first_recv:

            # listen to local port or port forwarding, if timeout, change to the remote server ip.
            poller = zmq.Poller()
            poller.register(self.socket, flags=zmq.POLLIN)
            if poller.poll(timeout=5000):
                poller.unregister(self.socket)
                # recv will be called later
            else:
                poller.unregister(self.socket)

                #raise IOError("Timeout processing auth request")

                #zmq_Response = {'test': 'test'}
                self.socket.close()
                self.socket = self.context.socket(zmq.DEALER)
                self.socket.plain_username = bytes(self.config_json["session_name"], 'utf-8')
                self.socket.plain_password = bytes(self.config_json["session_key"], 'utf-8')
                self.socket.identity = self.identity.encode('utf-8')

                self.socket.connect('tcp://'+str(self.config_json["server_ip"])+':'+str(self.config_json["server_port"]))
                #gma_start_request = self.config_json["env_config"]
                #self.socket.send(json.dumps(gma_start_request, indent=2).encode('utf-8'))#send start simulation request
                print(self.identity + " local port (or forwarded port) start request timeout.")

                print(self.identity + " send start request to remote server: "+str(self.config_json["server_ip"]))

                self.socket.send(json.dumps(self.gma_start_request, indent=2).encode('utf-8'))#send start simulation request

                poller.register(self.socket, flags=zmq.POLLIN)
                if poller.poll(timeout=5000):
                    poller.unregister(self.socket)
                    # recv will be called later
                else:
                    raise IOError("Cannot connect to local port, forwarded port, or the remote server! Check the parameters in common_config.json and the port forwarding.")
        self.first_recv = False
        reply = self.socket.recv()
        relay_json = json.loads(reply)

        #print(relay_json)        

        if relay_json["type"] == "no-available-worker":
            # no available network gym worker, retry the request later
            print(self.identity+" Receive: "+reply.decode())
            print(self.identity+" "+"retry later...")
            self.socket.close()
            self.context.term()
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
            self.socket.close()
            self.context.term()
            quit()
        else:
            # Unkown msg type, please check.This should not happen. 
            print(self.identity +" Receive: "+ reply.decode())
            print(self.identity +" "+ "***[ERROR]*** unkown msg type!")
            self.socket.close()
            self.context.term()
            quit()
     
    def process_measurement (self, reply_json):
        """Process the measurement.

        Args:
            reply_json (json): the network stats measurement

        Returns:
            pd.DataFrame: the processed network stats measurement
        """
        #print(reply_json)
        if(not reply_json['network_stats']):
            return None
        
        network_stats = pd.json_normalize(reply_json['network_stats']) 
        if "workload_stats" in reply_json:
            # workload measurement available
            print('Env (workload_stats) --> ' + str(reply_json['workload_stats']))
            #if "sim_time_lapse_ms" and "time_lapse_ms" in reply_json['workload_stats']:
            #    if reply_json['workload_stats']['time_lapse_ms']>0:
            #        print('Env Measurement --> Percentage of time spend on simulation: ' + str(int(100*reply_json['workload_stats']['sim_time_lapse_ms']/reply_json['workload_stats']['time_lapse_ms'])) + '%')
        return network_stats

    def close(self):
        self.socket.close()
        self.context.term()