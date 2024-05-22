#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : network_gym_server.py
import zmq
import zmq.auth
from zmq.auth.thread import ThreadAuthenticator
import threading
import time
import json
#from influxdb_client import InfluxDBClient, Point
#from influxdb_client.client.write_api import SYNCHRONOUS
import pathlib
FILE_PATH = pathlib.Path(__file__).parent
import csv

from rich.live import Live
from rich.table import Table

WORKER_TIMEOUT_S = 60

'''
class influxdb_thread(threading.Thread):
    def __init__(self, addr, identity, metric, influxdb_config_json):
        threading.Thread.__init__(self)
        self.addr = addr
        self.identity = identity
        self.metric = metric
        self.influxdb_config_json = influxdb_config_json
 
        # helper function to execute the threads

    def run (self):
        # move the parameters to a configure file...

        # You can generate a Token from the "Tokens Tab" in the UI
        token = self.influxdb_config_json["token"]
        ip_address = self.influxdb_config_json["ip_address"]
        port = self.influxdb_config_json["port"]
        org = self.influxdb_config_json["org"]
        bucket = self.influxdb_config_json["bucket"]
        
        url="http://" + ip_address + ":"+str(port)

        client = InfluxDBClient(url=url, token=token)

        write_api = client.write_api(write_options=SYNCHRONOUS)

        #data = "mem,host=host1 used_percent=23.43234543"
        point_list = []
        for element in self.metric['metric_list']:
            #print(element)
            point = Point(element["group"])\
                .tag("direction", element["direction"])\
                .tag("cid", element["cid"])\
                .tag("user", element["user"])\
                .tag("env", self.addr)\
                .tag("algorithm", self.identity)\
                .field(element["name"], element["value"])
            point_list.append(point)
        
        write_api.write(bucket, org, point_list)

        #data = "GMA,cid=Wi-Fi,client=0 test_value=1"
        print('InfluxDB finished write')
'''
class ServerTask(threading.Thread):
    """ServerTask"""
    def __init__(self, config_json):
        threading.Thread.__init__ (self)
        self.config_json = config_json
        self.available_workers_env_list = {} # store the worker -> env list dict
        self.available_workers_last_ts_dict = {} # store the worker -> last hello msg ts
        self.client_to_worker_dict = {} # store the active client -> worker mapping.
        self.client_to_env_dict = {} # store the client -> env dict.
        self.busy_workers_last_ts_dict = {} # store the worker -> last env measurement ts

    def generate_table(self):
        table = Table()
        table.add_column("Worker")
        table.add_column("Status")
        table.add_column("Time since Last Seen (seconds)")
        table.add_column("Environment")
        for key, value in self.available_workers_env_list.items():
            time_diff =  time.time() - self.available_workers_last_ts_dict[key]
            table.add_row(str(key.decode()), '[green]idle', str(time_diff), str(value))

        for client_id, worker_addr in list(self.client_to_worker_dict.items()):
            time_diff =  time.time() - self.busy_workers_last_ts_dict[worker_addr]
            table.add_row(str(worker_addr.decode()), "[blue]client: "+str(client_id.decode()), str(time_diff), str(self.client_to_env_dict[client_id]))

        return table
    
    def run(self):
        context = zmq.Context()
        context.setsockopt(zmq.LINGER, 10000)
        password_file = FILE_PATH / "network_gym_accounts.csv"
        password_dict = {}
        client_to_max_instance_dict = {}
        # opening the file using "with"
        # statement
        with open(password_file, 'r') as data:
            csvreader = csv.reader(data)
            # This skips the first row of the CSV file.
            next(csvreader)
            for line in csvreader:
                #print(line)
                password_dict[line[0]] = line[1]
                client_to_max_instance_dict[line[0]] = int(line[2])

        #print(password_dict)
        print("Max instances per client:")
        print(client_to_max_instance_dict)

        # Start an authenticator for this context.
        auth = ThreadAuthenticator(context)
        auth.start()
        # auth.allow('127.0.0.1')
        # Instruct authenticator to handle PLAIN requests
        auth.configure_plain(domain='*', passwords=password_dict)

        # Router handover allow the server to handover the connect to the new connection with the same connection "ID".
        context.setsockopt(zmq.ROUTER_HANDOVER, 1)
        frontend = context.socket(zmq.ROUTER)
        frontend.plain_server = True  # must come before bind
        frontend.bind('tcp://*:'+str(self.config_json["network_gym_client_port"]))

        backend = context.socket(zmq.ROUTER)
        backend.plain_server = True  # must come before bind
        backend.bind('tcp://*:'+str(self.config_json["network_gym_sim_port"]))

        #frontend connects to network gym clients.
        #backend connects to network gym env workers.

        poll_both = zmq.Poller()
        poll_both.register(frontend, zmq.POLLIN)
        poll_both.register(backend, zmq.POLLIN)

        #available_workers are the idle network gym env workers (connected to server, but no simulation running).
        available_workers = []
        #when a client request a new simulation, we assign a dedicated worker to that client. Every worker in self.client_to_worker_dict is busy.
        #This mapping info is stored in self.client_to_worker_dict, we remove the mapping when the simulation ends or an error occurs.
        #a worker can be only inside the available_workers or self.client_to_worker_dict, cannot be in both!!!!!!!!

        while True:

            socks = dict(poll_both.poll())

            #if not receiving env-hello message from a worker after WORKER_TIMEOUT_S, remove the timeout worker.
            current_time = time.time()
            expired_addr =  []
            for addr, ts in self.available_workers_last_ts_dict.items():
                if ts+WORKER_TIMEOUT_S <= current_time:
                    expired_addr.append(addr)
                    
            for addr in expired_addr:
                available_workers.remove(addr)
                print("delete timeout worker:"+addr.decode())
                del self.available_workers_env_list[addr]
                del self.available_workers_last_ts_dict[addr]

            # Handle worker activity on backend
            if socks.get(backend) == zmq.POLLIN:
                msg = backend.recv_multipart()
                print("[backend] Rx:")
                if len(msg) < 3:
                    print ("[Error] Ignore msg with wrong size:" + str(len(msg)))
                    print(msg)
                    continue
                #print(msg)
                address = msg[0]
                identity = msg[1]
                # Everything after the second (delimiter) frame is reply
                reply = msg[2]
                relay_json = json.loads(reply)  

                if relay_json["type"] == "env-hello":
                    #the env-hello does not carry the correct (algorithm client) identiy since the worker did not assign to any client yet.
                    print(msg)
                    if address not in available_workers:
                        available_workers.append(address)

                    env_list = relay_json["env_list"]

                    # only the official_session_name is allowed to use names other than custom_evn_name
                    # all other users only use "session_name" combined with custom_env_name as the env name.
                    words = address.decode().split('-')
                    env_session_name = words[0]
                    if env_session_name != self.config_json["official_session_name"]:
                        #not official session, set to the default custom environment_list.
                        env_list = [env_session_name + "-" + self.config_json["custom_env_name"]]
                    else:
                        # offcial session, also add the session name in front of the custom_env_name, the other names keep the same.
                        for i in range(len(env_list)):
                            if env_list[i] == self.config_json["custom_env_name"]:
                                env_list[i] = env_session_name + "-" + self.config_json["custom_env_name"]
                    self.available_workers_env_list[address] = env_list
                    self.available_workers_last_ts_dict[address] = current_time
                    if address in self.client_to_worker_dict.values():
                        print("[WARNING]: simulation is not stopped! This may happen if the 'env-end' msg is not implemented at the env worker, or something is wrong with the env worker:" + address.decode())
                    #error handling for busy workers, remove the client to worker routing rule
                    for client_id, addr in list(self.client_to_worker_dict.items()): # use list to create a copy since we are deleting element in the self.client_to_worker_dict
                        if address == addr:
                            #send error msg to algorithm client to stop current simulation
                            frontend.send_multipart([client_id, b'{"type":"env-error", "error_msg": "Worker Restarted, Try Again."}'])
                            del self.client_to_worker_dict[client_id]
                            break
                
                elif relay_json["type"] == "env-error":
                    #env-error may not carries the correct identity (of algorithm client).
                    #Simulation end with error. delete worker infomation and mapping.
                    #the worker will reconnect later and send hello msg to indicate idle state ...
                    print(msg)

                    #error handling for idle workers
                    if address in available_workers:
                        available_workers.remove(address)
                        del self.available_workers_env_list[address]
                        del self.available_workers_last_ts_dict[address]
                    
                    #error handling for busy workers
                    
                    for client_id, addr in list(self.client_to_worker_dict.items()): # use list to create a copy since we are deleting element in the self.client_to_worker_dict
                        if address == addr:
                            frontend.send_multipart([client_id, reply])#relay error msg to algorithm client to stop current simulation
                            del self.client_to_worker_dict[client_id]
                            break  
                        
                elif  relay_json["type"] == "env-end":
                    #simulation end. delete worker infomation and mapping.
                    #the worker will reconnect later and send hello msg to indicate idle state ...
                    print(msg)

                    #this should not happen, cannot stop simulation from idle worker.
                    if address in available_workers:
                        available_workers.remove(address)
                        print("[WARNING] 'env-end' msg from an idle worker. This should not happen!")
                        del self.available_workers_env_list[address]
                        del self.available_workers_last_ts_dict[address]
                    
                    #stop simulation from a busy worker.
                    # delete the routing table based on the client identity.
                    if identity in self.client_to_worker_dict.keys():
                        del self.client_to_worker_dict[identity]

                    # delete the routing table based on the worker address. Just in case the worker forgot to include the client identity.
                    for key, value in list(self.client_to_worker_dict.items()): # use list to create a copy since we are deleting element in the self.client_to_worker_dict
                        if value == address:
                            del self.client_to_worker_dict[identity]
                            break
                    # no need to send to client.
                    #frontend.send_multipart([identity, reply])
                elif relay_json["type"] == "env-measurement":
                    #measurement from network gym simlulation
                    print("Relay Measurement from: " + str(address)+ " to Algorithm Client: " + str(identity))
                    frontend.send_multipart([identity, reply])#relay measurement to the algorithm
                    #influxdb = influxdb_thread(address.decode(), identity.decode(), relay_json, self.config_json["influxdb"])#save to influxdb in a new thread
                    #influxdb.start()
                    self.busy_workers_last_ts_dict[address] = current_time
                else:
                    print("[WARNING] Unkown message type, relay to algorithm client!")
                    print(msg)
                    frontend.send_multipart([identity, reply])
                
                #print("available_workers:"+str(self.available_workers_env_list) + " | self.client_to_worker_dict:"+str(self.client_to_worker_dict))


            if socks.get(frontend) == zmq.POLLIN:
                #  Get client request, route to first available worker
                msg = frontend.recv_multipart()
                print("[forntend] Rx:")
                if len(msg) < 2:
                    print ("[Error] Ignore msg with wrong size:" + str(len(msg)))
                    print(msg)
                    continue
                #print(msg)

                identity = msg[0]
                reply = msg[1]
                relay_json = json.loads(reply)
                words = identity.decode().split('-')
                client_account_id = words[0]
                instance_id = words[-1]
                #print("client:" + str(client_account_id) + " instance:" + str(instance_id))

                # we limit the max number of instance for users. i.e., a user can only use instance id that within the range of [0, max_instance-1]

                if client_account_id in client_to_max_instance_dict.keys():
                    if int(instance_id) >= int(client_to_max_instance_dict[client_account_id]):
                        #instance id greater than max allowed instance id... return error
                        print("Reject this msg. client(" + str(client_account_id) + ") instance id(" + str(instance_id) + ") should be smaller thant the max instance id(" + str(int(client_to_max_instance_dict[client_account_id])-1)+")")
                        frontend.send_multipart([identity, b'{"type":"env-error", "error_msg": "Please reduce --client_id, e.g., --client_id=0. We are limiting the number of instances launched per client. Please contact us to add more instances."}'])
                        continue
                else:
                    #unkown user, do not process
                    frontend.send_multipart([identity, b'{"type":"env-error", "error_msg": "Unkown user account"}'])
                    continue


                if relay_json["type"] == "env-start":
                    if identity in self.client_to_worker_dict.keys():
                        #find routing rule for this client, this should not happen.... Error handle
                        worker_addr = self.client_to_worker_dict[identity]
                        print("[Error] find mapping of client:" + identity.decode()+ " to worker: " + worker_addr.decode())
                        # relay to network gym simlulation worker anyway. the network gym simlulation worker will treat the env-start as unexpected msg and stop simulation...
                        request = [worker_addr] + msg # add the worker's address for routing
                        backend.send_multipart(request) #relay the request to assigned worker.

                        frontend.send_multipart([identity, b'{"type":"env-error", "error_msg": "NetworkGym Client - Worker Mapping Exits (Client was force quited, e.g., ctrl+c!). Restart the client."}'])
                        del self.client_to_worker_dict[identity] 
                    else:
                        #find a new worker for this client

                        env_name = relay_json["env"]
                        if env_name == self.config_json["custom_env_name"]:
                            # custom env, append the session_name in the front.
                            env_name = client_account_id + "-" + self.config_json["custom_env_name"]
                        if available_workers:
                            # find the first worker that supports the env in his env_list
                            found_worker = False
                            for worker_addr in available_workers:
                                print(env_name)
                                #print(self.available_workers_env_list[worker_addr])
                                if env_name in self.available_workers_env_list[worker_addr]:
                                     #idle workers for this env available
                                    available_workers.remove(worker_addr) # assign a new worker for this client
                                    self.busy_workers_last_ts_dict[worker_addr] = self.available_workers_last_ts_dict[worker_addr]
                                    del self.available_workers_env_list[worker_addr]
                                    del self.available_workers_last_ts_dict[worker_addr]
                                    self.client_to_worker_dict[identity]=worker_addr # add this client -> worker mapping to dict
                                    self.client_to_env_dict[identity] = env_name
                                    print("Assinged client:" + identity.decode()+ " to worker: " + worker_addr.decode())
                                    request = [worker_addr] + msg # add the worker's address for routing
                                    backend.send_multipart(request) #relay the request to assigned worker.
                                    found_worker = True
                                    break
                            
                            if not found_worker:
                                #among the available workers, the requested env is not supported in their env_list
                                return_msg = '{"type":"no-available-worker", "msg": "No available workers for env: ' +env_name+ '. Available_workers and their [env list]:'+str(self.available_workers_env_list)+'"}'
                                msg[1] = return_msg.encode('utf-8')
                                print("worker available, but cannot find workers for this env: " + env_name)
                                frontend.send_multipart(msg)
                        else:
                            #no available workers...
                            msg[1] = b'{"type":"no-available-worker"}' # return no available worker msg to client.
                            print("no-available-worker for " + identity.decode())
                            frontend.send_multipart(msg)
                

                elif relay_json["type"] == "env-action":
                    if identity in self.client_to_worker_dict:
                        # find a client->worker routing rule based on existing mapping dict, i.e., a simulation for that algorithm client is running
                        worker_addr = self.client_to_worker_dict[identity]
                        print("Relay msg, find mapping of client:" + identity.decode()+ " to worker: " + worker_addr.decode())
                        request = [worker_addr] + msg # add the worker's address for routing
                        backend.send_multipart(request) #relay the request to assigned worker.
                    else:
                        print("[ERROR] Algorithm client to network gym simlulation worker mapping removed!")
                        #unkown message type
                        msg[1] = b'{"type":"env-error","error_msg":"algorithm client to network gym env worker mapping removed."}' # return error
                        print("unkown-msg for " + identity.decode())
                        frontend.send_multipart(msg)
                else:
                    print("[ERROR] Unkown message type!")
                    #unkown message type
                    msg[1] = b'{"type":"env-error", "error_msg": "unknown msg type"}' # return unkonw msg type
                    print("unkown-msg for " + identity.decode())
                    frontend.send_multipart(msg)

                #print("available_workers:"+str(self.available_workers_env_list) + " | self.client_to_worker_dict:"+str(self.client_to_worker_dict))
                #print("self.available_workers_last_ts_dict:"+str(self.available_workers_last_ts_dict))

        frontend.close()
        backend.close()
        context.term()
        # stop auth thread
        auth.stop()

