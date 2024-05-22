#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : configure.py

import zmq
import threading
import json
import traceback
import pathlib
import socket
FILE_PATH = pathlib.Path(__file__).parent
from network_gym_env.southbound_interface import *

"""
Flow Chart:
-------------------------------------------------
env_config.connect()
[every T seconds]:
    env_config -- "env-hello" --> server
env_config <-- "env-start" -- server
env_config.close()

env_sim.connect()
[every interval]:
    env_sim -- "env-measurement" --> server
    env_sim <-- "env-action" -- server
env_sim.close()

env_config.connect()
env_config -- "env-end" --> server
[every T seconds]:
    env_config -- "env-hello" --> server
...
-------------------------------------------------
"""

class Configure(threading.Thread):
    """Environment Configure Component.

    1. The "env_config" establishes a connection with the server and periodically dispatches the message "env-hello."
    2. Upon reception of an "env-start" signal, the "env_config" terminates its socket connection and initiates the "env_sim" simulator.
    3. Upon launching, the "env_sim" establishes a connection with the server, utilizing the identical identification as the "env_config." Subsequently, it commences the exchange of measurement/action messages with the client.
    4. Following the conclusion of the "env_sim" simulation, the socket is closed, and the "env_config" reconnects.


    From the server's point, the "env_config" and "env_sim" are perceived as identical entities.
    The division between the "env_config" and "env_sim" components provides the advantage of facilitating straightforward expansion of the "env_sim" to other simulators (e.g., ns-3) or test environments, all the while utilizing the same underlying "env_config" code.
    """
    def __init__(self, id, NetworkGymSim, env_list=['custom']):
        """Initialize custom environment.

        Args:
            id (int): environment identity
            NetworkGymSim (simulator): network simulator
            env_list (list[str]): a list of supported environments, non-offical account can only use 'custom' as env name
        """
        threading.Thread.__init__ (self)

        #common_config.json is shared by all environments
        f = open(FILE_PATH / 'common_config.json')
        self.config_json = json.load(f)
        self.identity = u'%s-%d-%s' % (self.config_json["session_name"], id, socket.gethostname())
        self.env_list = env_list
        self.NetworkGymSim = NetworkGymSim
        self.context = zmq.Context()
        self.context.setsockopt(zmq.LINGER, 10000)
    def run(self):
        """Run the environement configure.
        """

        # connect to server via southbound Interface
        identity = str(self.identity)
        env_config = southbound_connect(identity, self.config_json, self.context)
        print(identity + ': env_config socket connected.')

        poller = zmq.Poller()
        poller.register(env_config, flags=zmq.POLLIN)
        
        try:
            while True:
                # Send Hello msg.
                hello_msg = json.loads('{"type":"env-hello"}')
                hello_msg["env_list"] = self.env_list # add supported env_list to hello msg
                env_config.send_multipart([b'', json.dumps(hello_msg, indent=2).encode('utf-8')])# Hello msg does not include client
                print(identity + ': send env-hello msg.')
                print(hello_msg)

                # resend hello msg after 5 seconds if no msg is received.
                # [Warning] always use a poll with timeout before receiving a msg (recv_multipart)! otherwise, it may stuck forever!
                if poller.poll(timeout=5*1000):

                    # received a new msg
                    msg = env_config.recv_multipart()

                    if len(msg) < 2:
                        print ("[Error] Ignore msg with wrong size:" + str(len(msg)))
                        print(msg)
                        continue

                    msg_json = json.loads(msg[1])
                    if "type" in msg_json and msg_json["type"] == "env-start":
                        # In idle mode (periodic sending env-hello msg), the first msg should be "env-start"
                        # check if the env is supported (in the env_list)
                        if "env" not in msg_json or msg_json["env"] not in self.env_list:
                            # not supported env
                            print("Unkown Environment!")
                            error_msg = json.loads('{"type":"env-error", "error_msg": "Unkown Environment!"}')
                            msg[1]=json.dumps(error_msg, indent=2).encode('utf-8')
                            env_config.send_multipart(msg)
                            continue

                        print(identity + ': Recv.')
                        print(msg_json)
                        poller.unregister(env_config)
                        env_config.close()

                        print(identity + ': env_config socket closed.')

                        # start simulator ------------------->
                        # The simulator will start a new socket to send measurement and receive action.
                        # use try except such that if there is an error in the simulator, the thread will not stop and we can report the error to the client.
                        sim_error_msg = ''
                        try:
                            self.NetworkGymSim(identity, self.config_json, msg[0].decode(), msg_json) # replace it with your own simulator.
                        except Exception:
                            traceback.print_exc()
                            sim_error_msg = traceback.format_exc()
                        # simulator terminated <-------------------

                        # env_config reconnect the server.
                        env_config = southbound_connect(identity, self.config_json, self.context)
                        print(identity + ': env_config socket connected.')
                        
                        poller.register(env_config, flags=zmq.POLLIN)

                        if sim_error_msg != '':
                            # simualtor crashed with error msg, relay to error msg to client
                            error_msg = json.loads('{"type":"env-error"}')
                            error_msg["error_msg"] = sim_error_msg
                            msg[1]=json.dumps(error_msg, indent=2).encode('utf-8')
                            env_config.send_multipart(msg)# Env End msg
                        else:
                            # no error, send env-end to terminate client to env worker mapping
                            end_msg = json.loads('{"type":"env-end"}')
                            env_config.send_multipart([msg[0], json.dumps(end_msg, indent=2).encode('utf-8')])# Env End msg

                    else:
                        print("Unkown MSG type, Expecting env-start!")
                        error_msg = json.loads('{"type":"env-error", "error_msg": "Unkown MSG type, Expecting env-start!"}')
                        msg[1]=json.dumps(error_msg, indent=2).encode('utf-8')
                        env_config.send_multipart(msg)
        except:
            poller.unregister(env_config)
            env_config.close()
            self.context.term()
