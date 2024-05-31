#Copyright(C) 2024 Intel Corporation
#SPDX-License-Identifier: GPL-2.0
#https://spdx.org/licenses/GPL-2.0.html
#File : network_gym_sim.py

import os
import time
import pathlib
from subprocess import Popen, PIPE, CalledProcessError
import json

FILE_PATH = pathlib.Path(__file__).parent

def build_ns3(config = True, build= True):
    os.chdir(str(FILE_PATH))
    if config:
        os.system('./ns3 configure --build-profile=optimized')
    if build:
        os.system('./ns3 build')

def NetworkGymSim(env_identity, config_json, client_identity, msg_json):

    output_folder = env_identity
    os.chdir(str(FILE_PATH))
    os.system('rm -r '+output_folder)
    os.system('mkdir '+output_folder)

    config_json["env_identity"] = env_identity
    config_json["client_identity"] = client_identity
    config_json_object = json.dumps(config_json, indent=4)

    with open(output_folder+"/gym-configure.json", "w") as outfile:
        outfile.write(config_json_object)

    msg_json_object = json.dumps(msg_json, indent=4)
 
    with open(output_folder+"/env-configure.json", "w") as outfile:
        outfile.write(msg_json_object)

    ns3_command = './ns3 run scratch/unified-network-slicing.cc --cwd='+output_folder
    print(ns3_command)

    with Popen(ns3_command, shell=True, stdout=PIPE, stderr=PIPE, cwd=str(FILE_PATH), bufsize=1, universal_newlines=True) as p:
        for line in p.stdout:
            print(line, end='') # process line here
        if p.returncode != 0:
            output, error = p.communicate()
            print("[ns3 stopped] %d %s %s" % (p.returncode, output, error))
            if error:
                raise Exception("[ns3 crashed] %d %s %s" % (p.returncode, output, error))
