#Copyright(C) 2024 Intel Corporation
#SPDX-License-Identifier: Apache-2.0
#File : start_server.py
import argparse
import time
import json
import pathlib
FILE_PATH = pathlib.Path(__file__).parent

from rich.live import Live
from rich.table import Table
from network_gym_server import ServerTask

def main():
    """main function"""

    f = open(FILE_PATH / 'network_gym_server/network_gym_server_config.json')
    args = arg_parser()
    if(args.dev == True):
        f = open(FILE_PATH / 'network_gym_server/network_gym_server_dev_config.json')
    config_json = json.load(f)
    server = ServerTask(config_json)
    server.start()

    #display worker status in a table.
    with Live(server.generate_table(), refresh_per_second=4) as live:
        while server.is_alive:
            time.sleep(0.4)
            live.update(server.generate_table())
    
def arg_parser():
    parser = argparse.ArgumentParser(description='NetworkGym Server')
    parser.add_argument('--dev', type=bool, required=False, default=True, help='Set to True for devlopment mode.')
    args = parser.parse_args()
    return args

if __name__ == "__main__":
    main()
