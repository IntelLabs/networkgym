#Copyright(C) 2023 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : dummy_worker.py

import zmq
import json

def southbound_connect(identity, config_json, context):
    """Connect to the server via southbound interface.

    Args:
        identity (str): the environment indentity
        config_json (json): configuration file for southbound interface

    Returns:
        socket: zmq socket for southbound
    """
    sb_socket = context.socket(zmq.DEALER)
    sb_socket.plain_username = bytes(config_json["session_name"], 'utf-8')
    sb_socket.plain_password = bytes(config_json["session_key"], 'utf-8')
    
    sb_socket.identity = identity.encode('utf-8')
    sb_socket.connect('tcp://localhost:'+str(config_json["env_port"]))
    return sb_socket