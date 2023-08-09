Search.setIndex({"docnames": ["api/network_gym_client", "api/network_gym_client/adapter", "api/network_gym_client/adapter/network_slicing", "api/network_gym_client/adapter/nqos_split", "api/network_gym_client/adapter/qos_steer", "api/network_gym_client/env", "api/network_gym_client/northbound_interface", "api/network_gym_env", "api/network_gym_server", "content/motivation", "content/overview", "content/quickstart", "environments/mx_network_slicing", "environments/mx_network_slicing/cellular_network_slicing", "environments/mx_traffic_management", "environments/mx_traffic_management/mx_qos_traffic_steering", "environments/mx_traffic_management/mx_traffic_splitting", "index", "old-index", "tutorials/handling_time_limits", "tutorials/training_agents"], "filenames": ["api/network_gym_client.md", "api/network_gym_client/adapter.md", "api/network_gym_client/adapter/network_slicing.md", "api/network_gym_client/adapter/nqos_split.md", "api/network_gym_client/adapter/qos_steer.md", "api/network_gym_client/env.md", "api/network_gym_client/northbound_interface.md", "api/network_gym_env.md", "api/network_gym_server.md", "content/motivation.md", "content/overview.md", "content/quickstart.md", "environments/mx_network_slicing.md", "environments/mx_network_slicing/cellular_network_slicing.md", "environments/mx_traffic_management.md", "environments/mx_traffic_management/mx_qos_traffic_steering.md", "environments/mx_traffic_management/mx_traffic_splitting.md", "index.md", "old-index.rst", "tutorials/handling_time_limits.md", "tutorials/training_agents.md"], "titles": ["network_gym_client", "Adapter", "network_slicing Adapter", "nqos_split Adapter", "qos_steer Adapter", "Env", "NorthboundInterface", "network_gym_env", "network_gym_server", "Motivation", "Overview", "Quickstart", "Network Slicing", "Cellular Network Slicing", "Traffic Management", "Multi-Access QoS Traffic Steering", "Multi-Access Traffic Splitting", "NetworkGym: Revolutionizing Network AI Research and Development", "Welcome to Network Gym\u2019s documentation!", "Handling Time Limits", "Training Agents"], "terms": {"networkgymcli": [0, 10, 13, 15, 16], "includ": [0, 7, 8, 11, 17, 20], "three": [0, 10, 13, 15, 16, 17, 19], "compon": [0, 7, 8], "custom": [0, 5, 10, 11, 13, 15, 16, 17, 19], "gymnasium": [0, 1, 5, 10, 11, 20], "env": [0, 2, 3, 4, 6, 7, 11, 13, 15, 16, 19, 20], "adapt": [0, 5, 10, 13, 15, 16], "northbound": [0, 5, 6, 8, 10, 11], "interfac": [0, 5, 6, 7, 8, 11, 17], "The": [0, 5, 7, 8, 10, 11, 13, 15, 16, 17, 19, 20], "inheret": 0, "environ": [0, 1, 2, 4, 5, 6, 7, 8, 9, 12, 13, 14, 15, 16, 17, 19, 20], "class": [0, 2, 3, 4, 5, 6, 11], "commun": [0, 5, 7, 10, 11], "agent": [0, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 19], "us": [0, 5, 9, 11, 12, 13, 15, 16, 17, 19, 20], "standard": [0, 17], "e": [0, 5, 9, 13, 15, 16, 17, 19], "g": [0, 5, 9, 15, 16, 17], "exchang": [0, 11], "ob": [0, 1, 5, 11, 19], "reward": [0, 1, 5, 11, 19], "action": [0, 1, 2, 3, 4, 5, 12, 14, 17, 19, 20], "reset": [0, 5, 11, 19], "step": [0, 5, 13, 15, 16, 19], "function": [0, 2, 3, 4, 11, 13, 15, 16], "import": [0, 11, 13, 15, 16, 19], "gym": [0, 5, 6], "follow": [0, 5, 13, 15, 16], "def": 0, "__init__": 0, "self": [0, 1, 2, 3, 4, 5, 6], "arg1": 0, "arg2": 0, "super": 0, "return": [0, 1, 2, 3, 4, 5, 6, 11], "observ": [0, 2, 3, 4, 5, 17, 20], "termin": [0, 5, 11, 13, 15, 16], "truncat": [0, 5, 11, 13, 15, 16], "info": [0, 5, 11, 19, 20], "seed": [0, 5], "none": [0, 5], "option": [0, 5, 7, 11, 13, 15, 16, 19, 20], "transform": [0, 1, 10, 17], "data": [0, 1, 4, 6, 10, 11, 13, 15, 17, 20], "format": [0, 1, 10, 11], "from": [0, 1, 2, 3, 4, 5, 6, 11, 13, 19, 20], "network_gym": [0, 1, 5], "other": [0, 13, 15, 16, 19], "wai": [0, 5, 16], "around": [0, 17], "network": [0, 1, 2, 3, 4, 5, 6, 7, 10, 11, 15, 16], "stat": [0, 1, 2, 3, 4, 6, 7], "chang": [0, 19], "polici": [0, 1, 2, 3, 4, 6, 7, 11, 20], "connect": [0, 5, 6, 7, 8, 10, 11, 14, 15, 16, 17], "client": [0, 5, 6, 8, 11, 13, 15, 16, 17], "server": [0, 5, 6, 7, 8, 11, 13, 15, 16, 17], "configur": [0, 1, 2, 3, 4, 5, 6, 10, 11, 13, 15, 16, 17, 19], "paramet": [0, 1, 2, 3, 4, 5, 6, 7, 13, 15, 16, 17, 19], "between": [0, 1, 5, 7, 10, 16, 17], "envrion": [0, 8], "network_gym_cli": [1, 2, 3, 4, 11, 13, 15, 16, 19], "config_json": [1, 2, 3, 4, 5, 6, 11, 13, 15, 16, 19], "sourc": [1, 2, 3, 4, 5, 6, 17, 20], "thi": [1, 2, 3, 4, 5, 10, 11, 12, 13, 14, 15, 16, 17, 19], "i": [1, 2, 3, 5, 6, 10, 11, 13, 15, 16, 17, 19, 20], "an": [1, 5, 8, 10, 11, 13, 15, 16, 17, 19, 20], "It": [1, 5, 10, 11, 13, 15, 16, 17, 19], "measur": [1, 2, 3, 4, 5, 6, 10, 11, 13], "json": [1, 2, 3, 4, 5, 6, 11, 13, 15, 16, 19], "space": [1, 2, 3, 4, 5, 17], "also": [1, 5, 9, 11, 13, 15, 16, 17, 19], "can": [1, 5, 10, 11, 13, 15, 16, 17, 19, 20], "appli": 1, "initi": [1, 2, 3, 4, 5, 6, 13], "file": [1, 2, 3, 4, 5, 6, 11, 13, 15, 16, 19], "wandb_log": 1, "send": [1, 5, 6, 11, 17, 20], "log": [1, 5, 16], "inform": [1, 5, 11, 15, 16, 17], "wandb": [1, 11], "df_to_dict": 1, "df": [1, 2, 3, 4], "descript": 1, "datatyp": 1, "panda": [1, 2, 4, 6], "datafram": [1, 2, 3, 4, 6], "dictionari": [1, 5, 11], "object": [1, 5, 13], "string": 1, "descritpt": 1, "dict": [1, 5, 11], "convert": 1, "base": [2, 3, 4, 10, 11, 16, 17], "get_observ": [2, 3, 4, 13, 15, 16], "prepar": [2, 3, 4], "should": [2, 3, 4, 5, 11, 13], "same": [2, 3, 4, 13], "number": [2, 3, 4, 5, 11, 13, 15, 16, 19], "featur": [2, 3, 4, 13, 15, 16, 17], "defin": [2, 3, 4, 5, 11, 13, 15, 16, 17, 20], "get_observation_spac": [2, 3, 4], "df_list": [2, 4, 6], "get_reward": [2, 3, 4, 13, 15, 16], "pd": [2, 3, 4], "get_polici": [2, 3, 4], "rl": [2, 3, 4, 5, 11, 17, 20], "No": [2, 11, 20], "yet": 2, "get_action_spac": [2, 3, 4], "get": [2, 3, 4, 5], "get_rbg_siz": 2, "bandwidth": [2, 15], "comput": [2, 11, 13, 15, 16, 17], "resourc": [2, 12, 13, 15, 16, 17], "block": [2, 13], "group": [2, 13], "size": 2, "bandwith": 2, "rb": [2, 13], "code": [2, 7, 17, 20], "coppi": 2, "ns3": [2, 9], "pf": 2, "type": [2, 5, 11, 17], "0": [2, 3, 5, 11, 13, 15, 16, 19], "alloc": [2, 12, 13, 17], "rbg": 2, "int": [2, 5, 6], "resouc": 2, "netowrk_util": [3, 16], "throughput": [3, 13, 15, 16], "delai": [3, 5, 13, 15, 16, 20], "alpha": 3, "5": [3, 13, 15, 16], "calcul": [3, 4, 15], "util": [3, 10, 13, 15, 16], "specifi": [3, 5, 11, 13, 15, 16, 19], "valu": [3, 5, 11, 13, 15, 16], "balanc": 3, "default": [3, 4, 5, 11, 13, 17], "float": 3, "repres": [3, 11, 13, 15, 16], "bit": 3, "per": [3, 4, 13, 15, 16, 19], "second": [3, 11, 15, 16, 19], "metric": [3, 11], "list": [4, 5, 6, 11], "calculate_wifi_qos_user_num": [4, 15], "qos_rat": [4, 15], "qo": [4, 11, 14, 17], "user": [4, 5, 11, 13, 14, 15, 16, 17, 20], "over": [4, 14, 15, 16], "wi": [4, 14, 15, 16], "fi": [4, 14, 15, 16], "rate": [4, 13, 15, 16], "doubl": 4, "id": [5, 6], "networkgym": [5, 6, 13, 15, 16, 19, 20], "initil": 5, "intern": [5, 11], "state": [5, 11, 20], "": [5, 11, 13, 15, 16, 19, 20], "prng": 5, "np_random": 5, "If": [5, 11, 19], "doe": [5, 17], "alreadi": [5, 15], "have": [5, 11, 13, 15, 16, 17, 19, 20], "pass": 5, "chosen": 5, "some": [5, 11], "entropi": 5, "timestamp": 5, "dev": 5, "urandom": 5, "howev": 5, "ha": [5, 11, 13, 15, 16, 19], "you": [5, 10, 11, 13, 15, 16, 19, 20], "integ": 5, "even": 5, "exist": 5, "usual": 5, "want": 5, "right": [5, 13], "after": [5, 11, 13, 15, 16, 19], "been": [5, 11, 20], "never": 5, "again": 5, "pleas": [5, 17], "refer": [5, 11, 17, 19, 20], "minim": [5, 16], "exampl": [5, 7, 11], "abov": [5, 17], "see": [5, 10, 11, 13, 15, 16], "paradigm": 5, "addit": [5, 11, 17], "how": [5, 10, 11, 15, 17, 19], "depend": 5, "specif": [5, 9, 10, 13, 17, 19, 20], "obstyp": 5, "contain": [5, 11, 13], "auxiliari": 5, "complement": 5, "analog": 5, "run": [5, 11, 13, 15, 16, 19], "one": [5, 11, 13, 16, 19], "timestep": [5, 11], "dynam": [5, 17], "gamsim": 5, "check": 5, "last": [5, 19], "episod": [5, 11, 19], "done": [5, 11], "acttyp": 5, "provid": [5, 7, 10, 11, 15, 16, 17, 19, 20], "updat": [5, 11, 14, 16], "element": [5, 10, 11], "observation_spac": [5, 11], "next": [5, 11], "due": 5, "supportsfloat": 5, "result": [5, 11, 13, 15, 16, 19], "take": [5, 11, 13, 15], "bool": [5, 6], "whether": 5, "reach": [5, 11, 15], "under": 5, "mdp": 5, "task": 5, "which": [5, 10, 11, 13, 15, 17], "posit": [5, 13, 15, 16], "neg": 5, "goal": [5, 15, 16], "move": 5, "lava": 5, "sutton": 5, "barton": 5, "gridworld": 5, "true": [5, 11], "need": [5, 9, 11, 13], "call": [5, 11], "condit": 5, "outsid": 5, "scope": 5, "satisfi": 5, "typic": 5, "timelimit": [5, 11], "could": [5, 11], "indic": [5, 6, 13, 15], "physic": [5, 17], "go": 5, "out": [5, 11], "bound": [5, 11], "end": [5, 11, 17, 19], "prematur": 5, "befor": 5, "diagnost": 5, "help": [5, 11], "debug": 5, "learn": [5, 9, 17, 20], "raw": 5, "flag": 5, "deprec": 5, "A": [5, 11, 13, 15, 16, 17, 19], "boolean": 5, "case": [5, 11, 13, 17], "further": 5, "undefin": 5, "wa": 5, "remov": 5, "openai": 5, "v26": 5, "favor": 5, "signal": [5, 11], "mai": [5, 11, 19], "emit": 5, "differ": [5, 10, 11, 15, 17, 19], "reason": [5, 17], "mayb": 5, "underli": 5, "solv": 5, "successfulli": 5, "certain": [5, 13, 15, 16, 19], "exceed": [5, 11], "simul": [5, 7, 9, 10, 17], "enter": 5, "invalid": 5, "action_spac": [5, 11, 19, 20], "correspond": 5, "valid": [5, 6, 11], "all": [5, 11, 12, 13, 15, 16], "For": [5, 11, 15, 17], "discret": [5, 11], "give": 5, "2": [5, 13, 15, 16, 17], "mean": [5, 17], "ar": [5, 11, 13, 15, 16, 17], "two": [5, 7, 8, 10, 19], "1": [5, 11, 13, 15, 16, 19], "box": [5, 11, 13, 15, 16], "3": [5, 10, 11, 13, 15, 16, 17, 19], "4028234663852886e": 5, "38": 5, "4": [5, 13], "float32": [5, 13, 15, 16], "shape": [5, 11, 13, 15, 16], "denot": [5, 13, 15, 16, 19], "arrai": [5, 11, 13, 20], "we": [5, 7, 8, 11, 13, 15, 16, 17, 19], "well": 5, "high": [5, 16, 17], "8000002e": 5, "00": 5, "4028235e": 5, "1887903e": 5, "01": 5, "dtype": 5, "low": [5, 16], "northbound_interface_cli": 5, "traslat": 5, "dataformat": 5, "sever": [6, 17], "repli": 6, "measurementreport": 6, "ok_flag": 6, "structur": 6, "store": [6, 11], "report": 6, "whehter": 6, "stst": 6, "recv": 6, "receiv": [6, 7, 11, 15, 16], "messag": 6, "process_measur": 6, "reply_json": 6, "networkgymenv": [7, 11, 13, 15, 16, 19], "southbound": [7, 8, 10], "At": [7, 8, 10, 13, 15, 16, 17, 19], "present": [7, 8, 10, 15], "onli": [7, 15], "support": [7, 10, 11, 15, 17], "emul": [7, 10], "testb": [7, 10], "releas": [7, 8], "futur": 7, "todo": 7, "networkgymserv": [8, 10, 11], "manag": [8, 15, 16, 17], "when": [8, 13, 15, 16, 17, 20], "select": [8, 10, 11, 13, 14, 15, 16], "idl": 8, "instanc": [8, 11, 15, 17], "add": [8, 11], "map": [8, 10, 11], "rout": [8, 10], "tabl": 8, "do": 8, "plan": [8, 11], "softwar": [8, 11, 17], "real": 9, "world": [9, 17], "dataset": 9, "control": [9, 15, 17], "oper": [9, 13, 15, 16, 17, 19], "difficult": 9, "acquir": 9, "align": 9, "usag": [9, 13], "requir": [9, 15], "itself": 9, "enough": 9, "train": [9, 13, 15, 16, 17], "test": [9, 15], "model": [9, 17], "reinforc": [9, 17, 20], "etc": [9, 17], "tool": [9, 17], "often": 9, "veri": 9, "complex": 9, "especi": 9, "research": [9, 10, 11], "lack": [9, 15], "common": 9, "simpl": [9, 11], "api": [9, 11, 20], "evalu": [9, 15], "benchmark": 9, "algorithm": [9, 11, 13, 17], "framework": [10, 17], "consist": [10, 11, 17], "essenti": [10, 17], "each": [10, 11, 12, 13, 14, 15, 16, 17, 19, 20], "plai": [10, 17], "crucial": [10, 17], "role": [10, 17, 20], "system": [10, 11, 13, 17], "addition": [10, 11, 17], "encompass": 10, "facilit": [10, 11, 17], "seamless": [10, 17], "interact": [10, 17, 19, 20], "within": [10, 11, 13], "graphic": 10, "represent": [10, 17], "below": [10, 11, 19, 20], "illustr": [10, 19], "architectur": [10, 17], "In": [10, 11, 12, 13, 14, 15, 16, 19], "work": 10, "togeth": 10, "creat": [10, 11, 17, 20], "cohes": 10, "effici": [10, 13, 15, 17, 20], "servic": [10, 12, 13, 15, 17], "ai": [10, 11], "develop": [10, 11], "compris": 10, "main": 10, "establish": [10, 11], "enabl": [10, 11, 17, 20], "desir": [10, 15], "respons": 10, "allow": [10, 11, 17, 20], "compat": 10, "like": [10, 13, 17], "stabl": [10, 17], "baselines3": [10, 17], "cleanrl": [10, 17], "central": 10, "maintain": 10, "keep": 10, "track": 10, "activ": [10, 11, 20], "its": [10, 13, 15, 17], "assign": [10, 13, 15, 16], "dure": [10, 15], "session": [10, 11, 13, 15, 16, 19], "either": [10, 11, 13, 15, 16], "through": [10, 11, 13, 15, 16, 17, 19], "n": [10, 11, 13, 15, 16, 17], "offer": [10, 17, 20], "distinct": [10, 13, 17], "furthermor": 10, "ten": 10, "statist": 10, "enhanc": [10, 11, 17], "capabl": [10, 11, 17], "machin": [11, 13, 15, 16, 17], "requst": 11, "collabor": 11, "mlwin": 11, "ring": 11, "univers": 11, "public": 11, "internet": 11, "intel": 11, "ani": [11, 15], "issu": 11, "feel": 11, "free": 11, "u": 11, "netaigym": 11, "gmail": 11, "com": 11, "launch": 11, "sim": 11, "aa": 11, "devcloud": 11, "make": 11, "avail": [11, 15, 20], "meanwhil": [11, 13], "networkgymsim": 11, "new": 11, "5g": 11, "o": [11, 17], "ran": [11, 17], "distribut": [11, 13, 15, 16, 17], "energi": [11, 17], "save": [11, 17], "predict": 11, "more": [11, 13, 15, 16, 17], "onc": [11, 13, 15, 16, 19], "gain": 11, "begin": 11, "download": [11, 13, 15, 16], "your": [11, 17], "prompt": 11, "collect": [11, 17, 20], "them": [11, 13, 15], "forward": 11, "open": [11, 17], "non": 11, "relev": 11, "transmit": [11, 13, 15, 16], "continu": [11, 13, 15, 16, 19], "conveni": 11, "visual": 11, "websit": 11, "python": [11, 17], "demonstr": [11, 19], "straightforward": 11, "packag": 11, "client_id": [11, 13, 15, 16, 19], "argument": 11, "classic": 11, "loop": [11, 17], "implement": [11, 13, 15], "load_config_fil": [11, 13, 15, 16, 19], "env_nam": [11, 19], "nqos_split": [11, 16, 19], "num_step": [11, 19], "1000": [11, 19], "rang": [11, 13, 19, 20], "sampl": [11, 19, 20], "exit": [11, 13, 15, 16, 19], "break": [11, 19], "epsiod": [11, 19], "up": [11, 19], "still": [11, 19], "anoth": [11, 19], "first": [11, 15, 16, 19], "load": [11, 13, 15, 16], "name": 11, "keyword": 11, "associ": 11, "dedic": [11, 12, 13], "worker": 11, "until": 11, "parallel": 11, "obtain": 11, "perform": [11, 12, 14, 15, 16, 17, 19], "As": 11, "along": 11, "One": 11, "multipl": [11, 13, 15, 16], "trucat": 11, "fix": 11, "And": 11, "restart": 11, "termiant": 11, "handl": [11, 13, 15, 16], "time": [11, 13, 15, 16], "limit": [11, 13, 15, 16], "detail": [11, 13, 15, 16, 17, 20], "everi": 11, "attribut": 11, "understand": 11, "expect": 11, "input": 11, "output": 11, "respect": [11, 13], "random": [11, 13, 15, 16], "instead": 11, "prefer": 11, "empti": [11, 17, 20], "shown": [11, 20], "np": [11, 20], "both": [11, 13, 14, 15, 16, 19], "inherit": 11, "major": 11, "possibl": [11, 15], "might": 11, "describ": 11, "dimension": 11, "where": [11, 13, 16], "upper": 11, "lower": 11, "our": [11, 13], "shift": 11, "tupl": 11, "multibinari": 11, "binari": 11, "multidiscret": [11, 15], "wrapper": [11, 13, 15, 16], "maximum": [11, 13, 15], "clipact": 11, "clip": 11, "li": [11, 17], "rescaleact": [11, 13, 15, 16], "rescal": [11, 13, 15, 16], "lie": 11, "interv": 11, "timeawareobserv": 11, "about": [11, 13, 15, 16, 17], "index": [11, 18], "ensur": [11, 13, 15, 16], "transit": 11, "markov": 11, "flattenobserv": 11, "flatten": 11, "normalizeobserv": [11, 13, 15, 16], "normal": [11, 13, 15, 16], "t": [11, 13], "coordin": 11, "center": [11, 17], "unit": 11, "varianc": 11, "arg": 11, "normal_obs_env": 11, "cellular": [12, 15, 16], "period": [12, 14, 15, 16], "among": [12, 13], "priorit": [12, 13, 15], "share": [12, 13], "while": [12, 13, 19], "strive": [12, 13], "meet": [12, 13, 15], "level": [12, 13, 15], "agreement": [12, 13], "entir": [13, 15, 16, 19], "partit": 13, "tailor": [13, 17], "serv": [13, 15, 17], "percept": 13, "possess": 13, "abil": 13, "great": 13, "design": [13, 17, 20], "belong": 13, "mac": 13, "schedul": [13, 15], "proport": 13, "fair": 13, "guarante": 13, "equit": 13, "imparti": 13, "inf": [13, 15, 16], "config": [13, 15, 16], "network_sl": 13, "pose": 13, "challeng": [13, 15], "compet": 13, "finit": 13, "primari": [13, 15], "strateg": [13, 16], "access": [13, 17], "vlab": [13, 15, 16], "multidimension": 13, "ndarrai": [13, 15, 16], "five": 13, "total": [13, 15], "mbp": [13, 15, 16], "fraction": 13, "express": 13, "percentag": 13, "sum": 13, "across": [13, 15], "equal": [13, 16], "averag": 13, "violat": [13, 15], "wise": 13, "record": 13, "experienc": 13, "moment": 13, "millisecond": 13, "min": [13, 15, 16], "max": [13, 15, 16], "m": 13, "note": [13, 15, 16, 19], "exce": 13, "surpass": 13, "appropri": 13, "scale": 13, "down": 13, "softmax": 13, "adher": 13, "constraint": [13, 15], "mathemat": 13, "sum_": 13, "a_i": 13, "quad": 13, "frac": 13, "j": 13, "a_j": 13, "num": [13, 16], "account": 13, "factor": 13, "cost": 13, "formul": 13, "r": [13, 16], "left": 13, "text": 13, "_i": 13, "lambda": 13, "cdot": 13, "delay_violation_r": 13, "gamma": 13, "rb_usage_r": 13, "adjust": [13, 15], "emphasi": 13, "given": [13, 15, 16], "By": [13, 17], "consider": 13, "uniform": [13, 15, 16], "2d": [13, 15, 16], "plane": [13, 15, 16], "x": [13, 15, 16], "y": [13, 15, 16], "boundari": [13, 15, 16], "veloc": [13, 15, 16], "episodes_per_sess": [13, 15, 16, 19], "steps_per_episod": [13, 15, 16, 19], "l": [13, 15, 16, 19], "happen": [13, 15, 16], "length": [13, 15, 16, 19], "gener": [13, 15, 16, 19], "subsequ": [13, 15, 16, 19], "cannot": [13, 15, 16, 19], "reconfigur": [13, 15, 16, 19], "point": [13, 15, 16, 19], "program": [13, 15, 16, 17, 19], "mx": [14, 15, 16], "split": 14, "ratio": [14, 16], "consid": [14, 15, 16], "lte": [14, 15, 16], "steer": [14, 17], "link": [14, 15, 16], "part": [15, 16], "dimens": 15, "qos_steer": 15, "scenario": 15, "randomli": [15, 16], "intellig": 15, "direct": 15, "aim": [15, 16], "achiev": [15, 16], "best": 15, "qualiti": 15, "regard": 15, "halt": 15, "admiss": 15, "capac": [15, 16], "10": 15, "accommod": 15, "drop": 15, "remain": 15, "On": 15, "hand": 15, "contend": 15, "without": 15, "restrict": 15, "section": [15, 19], "qos_requir": 15, "test_duration_m": 15, "500": 15, "durat": 15, "delay_bound_m": 15, "100": 15, "flow": 15, "delay_violation_target": 15, "02": 15, "target": 15, "loss_target": 15, "001": 15, "loss": 15, "packet": 15, "otherwis": 15, "met": 15, "set": 15, "zero": 15, "estim": [15, 16], "channel": [15, 16, 17], "third": [15, 16], "combin": [15, 16], "mechan": 15, "maxim": [15, 16], "lead": 15, "essenc": 15, "optim": [15, 17, 20], "indirectli": 15, "overal": 15, "problem": 16, "station": 16, "closest": 16, "handov": 16, "disabl": 16, "latenc": 16, "transmitt": 16, "f": 16, "owd": 16, "innov": 17, "core": 17, "simplifi": 17, "known": 17, "integr": [17, 20], "variou": 17, "four": 17, "purpos": [17, 20], "offlin": [17, 20], "flexibl": [17, 20], "own": [17, 20], "special": [17, 20], "demo": [17, 20], "power": 17, "advanc": 17, "stabil": 17, "reliabl": 17, "clean": [17, 20], "tutori": [17, 20], "guidanc": 17, "instruct": [17, 20], "effect": 17, "remot": 17, "act": 17, "intermediari": 17, "leverag": 17, "traffic": 17, "slice": 17, "comprehens": 17, "realist": 17, "cut": 17, "edg": 17, "field": 17, "overview": 17, "page": [17, 18], "There": 17, "compel": 17, "languag": 17, "platform": 17, "separ": 17, "seamlessli": [17, 20], "c": 17, "divers": 17, "technologi": 17, "decoupl": 17, "deploy": 17, "thei": 17, "deploi": 17, "ml": 17, "workload": 17, "focus": 17, "layer": 17, "involv": 17, "abstract": 17, "aspect": 17, "endeavor": 17, "full": 17, "stack": 17, "cloud": 17, "fidel": 17, "proprietari": 17, "applic": 17, "xapp": 17, "rapp": 17, "ric": 17, "digit": 17, "twin": [17, 20], "modul": 18, "search": 18, "outlin": 19, "env_config": 19, "start": 19, "showcas": 19, "process": 19, "fulfil": 20, "To": 20, "simpli": 20, "modifi": 20, "trigger": 20, "art": 20, "sota": 20, "These": 20, "popular": 20, "ones": 20, "ppo": 20, "proxim": 20, "ddpg": 20, "deep": 20, "determinist": 20, "gradient": 20, "sac": 20, "soft": 20, "actor": 20, "critic": 20, "td3": 20, "a2c": 20, "advantag": 20, "moreov": 20}, "objects": {"network_gym_client": [[1, 0, 1, "", "Adapter"], [5, 0, 1, "", "Env"], [6, 0, 1, "", "MeasurementReport"], [6, 0, 1, "", "NorthboundInterface"]], "network_gym_client.Adapter": [[1, 1, 1, "", "df_to_dict"], [1, 1, 1, "", "wandb_log"]], "network_gym_client.Env": [[5, 2, 1, "", "action_space"], [5, 2, 1, "", "adapter"], [5, 2, 1, "", "northbound_interface_client"], [5, 2, 1, "", "observation_space"], [5, 1, 1, "", "reset"], [5, 1, 1, "", "step"]], "network_gym_client.NorthboundInterface": [[6, 1, 1, "", "connect"], [6, 1, 1, "", "process_measurement"], [6, 1, 1, "", "recv"], [6, 1, 1, "", "send"]], "network_gym_client.envs.network_slicing": [[2, 0, 1, "", "Adapter"]], "network_gym_client.envs.network_slicing.Adapter": [[2, 1, 1, "", "get_action_space"], [2, 1, 1, "", "get_observation"], [2, 1, 1, "", "get_observation_space"], [2, 1, 1, "", "get_policy"], [2, 1, 1, "", "get_rbg_size"], [2, 1, 1, "", "get_reward"]], "network_gym_client.envs.nqos_split": [[3, 0, 1, "", "Adapter"]], "network_gym_client.envs.nqos_split.Adapter": [[3, 1, 1, "", "get_action_space"], [3, 1, 1, "", "get_observation"], [3, 1, 1, "", "get_observation_space"], [3, 1, 1, "", "get_policy"], [3, 1, 1, "", "get_reward"], [3, 1, 1, "", "netowrk_util"]], "network_gym_client.envs.qos_steer": [[4, 0, 1, "", "Adapter"]], "network_gym_client.envs.qos_steer.Adapter": [[4, 1, 1, "", "calculate_wifi_qos_user_num"], [4, 1, 1, "", "get_action_space"], [4, 1, 1, "", "get_observation"], [4, 1, 1, "", "get_observation_space"], [4, 1, 1, "", "get_policy"], [4, 1, 1, "", "get_reward"]]}, "objtypes": {"0": "py:class", "1": "py:function", "2": "py:attribute"}, "objnames": {"0": ["py", "class", "Python class"], "1": ["py", "function", "Python function"], "2": ["py", "attribute", "Python attribute"]}, "titleterms": {"network_gym_cli": [0, 5, 6], "adapt": [1, 2, 3, 4], "The": 1, "base": 1, "class": [1, 7, 10], "method": [1, 2, 3, 4, 5, 6], "addit": [1, 2, 3, 4, 6], "network_sl": 2, "reward": [2, 3, 4, 13, 15, 16], "nqos_split": 3, "qos_steer": 4, "env": 5, "attribut": 5, "northboundinterfac": 6, "network_gym_env": 7, "networkgym": [7, 9, 10, 11, 17], "uml": 7, "diagram": [7, 10], "network_gym_serv": 8, "motiv": 9, "where": 9, "i": 9, "data": 9, "network": [9, 12, 13, 17, 18, 20], "ai": [9, 17], "develop": [9, 17], "challeng": 9, "why": [9, 17], "overview": 10, "compon": [10, 17], "interfac": 10, "client": 10, "server": 10, "environ": [10, 11], "quickstart": 11, "access": [11, 15, 16], "testb": 11, "via": 11, "vlab": 11, "basic": 11, "usag": 11, "upon": 11, "start": [11, 13, 15, 16], "networkgymcli": 11, "follow": 11, "seri": 11, "step": 11, "occur": 11, "dure": 11, "simul": 11, "process": 11, "repeat": 11, "when": 11, "conclud": 11, "initi": 11, "interact": 11, "explain": 11, "code": [11, 19], "action": [11, 13, 15, 16], "observ": [11, 13, 15, 16], "space": [11, 13, 15, 16], "modifi": 11, "slice": [12, 13], "cellular": 13, "descript": [13, 15, 16], "prerequisit": [13, 15, 16], "argument": [13, 15, 16], "state": [13, 15, 16], "episod": [13, 15, 16], "end": [13, 15, 16], "traffic": [14, 15, 16], "manag": 14, "multi": [15, 16], "qo": 15, "steer": 15, "split": 16, "transit": 16, "dynam": 16, "revolution": 17, "research": 17, "agent": [17, 20], "option": 17, "gymnasium": 17, "api": 17, "kei": 17, "divid": 17, "scope": 17, "limit": [17, 19], "In": 17, "out": 17, "welcom": 18, "gym": 18, "": 18, "document": 18, "indic": 18, "tabl": 18, "handl": 19, "time": 19, "truncat": 19, "termin": 19, "sequenti": 19, "train": [19, 20], "exampl": 19, "python": 19, "system": 20, "default": 20, "custom": 20, "algorithm": 20, "stabl": 20, "baselines3": 20, "cleanrl": 20}, "envversion": {"sphinx.domains.c": 2, "sphinx.domains.changeset": 1, "sphinx.domains.citation": 1, "sphinx.domains.cpp": 8, "sphinx.domains.index": 1, "sphinx.domains.javascript": 2, "sphinx.domains.math": 2, "sphinx.domains.python": 3, "sphinx.domains.rst": 2, "sphinx.domains.std": 2, "sphinx.ext.viewcode": 1, "sphinx": 57}, "alltitles": {"network_gym_client": [[0, "network-gym-client"]], "network_slicing Adapter": [[2, "network-slicing-adapter"]], "": [[2, "id1"], [3, "id1"], [4, "id1"]], "Methods": [[2, "methods"], [3, "methods"], [4, "methods"], [5, "methods"], [6, "methods"], [1, "methods"]], "Reward Methods": [[2, "reward-methods"], [4, "reward-methods"]], "Additional Methods": [[2, "additional-methods"], [3, "additional-methods"], [4, "additional-methods"], [6, "additional-methods"], [1, "additional-methods"]], "nqos_split Adapter": [[3, "nqos-split-adapter"]], "Reward Method": [[3, "reward-method"]], "qos_steer Adapter": [[4, "qos-steer-adapter"]], "Env": [[5, "env"]], "network_gym_client.Env": [[5, "network-gym-client-env"]], "Attributes": [[5, "attributes"]], "NorthboundInterface": [[6, "northboundinterface"]], "network_gym_client.NorthboundInterface": [[6, "network-gym-client-northboundinterface"]], "network_gym_env": [[7, "network-gym-env"]], "NetworkGym UML Class Diagram": [[7, "networkgym-uml-class-diagram"]], "network_gym_server": [[8, "network-gym-server"]], "Motivation": [[9, "motivation"]], "Where is Data?": [[9, "where-is-data"]], "Network AI Developer Challenges (Why NetworkGym?)": [[9, "network-ai-developer-challenges-why-networkgym"]], "Overview": [[10, "overview"]], "NetworkGym: Components and Interfaces": [[10, "networkgym-components-and-interfaces"]], "Client Component": [[10, "client-component"]], "Server Component": [[10, "server-component"]], "Environment Component": [[10, "environment-component"]], "Class Diagram": [[10, "class-diagram"]], "Quickstart": [[11, "quickstart"]], "Accessing the NetworkGym Testbed via vLab": [[11, "accessing-the-networkgym-testbed-via-vlab"]], "Basic Usage": [[11, "basic-usage"]], "\u25b6\ufe0f Upon starting the NetworkGymClient, the following series of steps occur:": [[11, null]], "\ud83d\udd01 During the simulation, the process repeats as follows:": [[11, null]], "\u23f9\ufe0f When the NetworkGym or the simulation concludes:": [[11, null]], "Initializing Environments": [[11, "initializing-environments"]], "Interacting with the Environment": [[11, "interacting-with-the-environment"]], "Explaining the code": [[11, "explaining-the-code"]], "Action and observation spaces": [[11, "action-and-observation-spaces"]], "Modifying the environment": [[11, "modifying-the-environment"]], "Network Slicing": [[12, "network-slicing"]], "Cellular Network Slicing": [[13, "cellular-network-slicing"]], "Description": [[13, "description"], [15, "description"], [16, "description"]], "Prerequisite": [[13, "prerequisite"], [15, "prerequisite"], [16, "prerequisite"]], "Observation Space": [[13, "observation-space"], [15, "observation-space"], [16, "observation-space"]], "Action Space": [[13, "action-space"], [15, "action-space"], [16, "action-space"]], "Reward": [[13, "reward"], [15, "reward"], [16, "reward"]], "Arguments": [[13, "arguments"], [15, "arguments"], [16, "arguments"]], "Starting State": [[13, "starting-state"], [15, "starting-state"], [16, "starting-state"]], "Episode End": [[13, "episode-end"], [15, "episode-end"], [16, "episode-end"]], "Traffic Management": [[14, "traffic-management"]], "Multi-Access QoS Traffic Steering": [[15, "multi-access-qos-traffic-steering"]], "Multi-Access Traffic Splitting": [[16, "multi-access-traffic-splitting"]], "Transition Dynamics": [[16, "transition-dynamics"]], "Handling Time Limits": [[19, "handling-time-limits"]], "Truncation": [[19, "truncation"]], "Termination": [[19, "termination"]], "Sequential Training Example": [[19, "sequential-training-example"]], "Python code:": [[19, "python-code"]], "Training Agents": [[20, "training-agents"]], "System Default Agent": [[20, "system-default-agent"]], "Custom Algorithm Agent": [[20, "custom-algorithm-agent"]], "Stable-Baselines3 Network Agent": [[20, "stable-baselines3-network-agent"]], "CleanRL Network Agent": [[20, "cleanrl-network-agent"]], "Adapter": [[1, "adapter"]], "The Base Class": [[1, "the-base-class"]], "NetworkGym: Revolutionizing Network AI Research and Development": [[17, "networkgym-revolutionizing-network-ai-research-and-development"]], "Network Agent Options": [[17, "network-agent-options"]], "Gymnasium API": [[17, "gymnasium-api"]], "Key Components of NetworkGym": [[17, "key-components-of-networkgym"]], "NetworkGym: Why Divide?": [[17, "networkgym-why-divide"]], "NetworkGym: Scope and Limitations": [[17, "networkgym-scope-and-limitations"]], "\u2714\ufe0f In-Scope": [[17, "in-scope"]], "\u274c Out-of-Scope": [[17, "out-of-scope"]], "Welcome to Network Gym\u2019s documentation!": [[18, "welcome-to-network-gym-s-documentation"]], "Indices and tables": [[18, "indices-and-tables"]]}, "indexentries": {"adapter (class in network_gym_client)": [[1, "network_gym_client.Adapter"]], "df_to_dict() (in module network_gym_client.adapter)": [[1, "network_gym_client.Adapter.df_to_dict"]], "wandb_log() (in module network_gym_client.adapter)": [[1, "network_gym_client.Adapter.wandb_log"]], "adapter (class in network_gym_client.envs.network_slicing)": [[2, "network_gym_client.envs.network_slicing.Adapter"]], "get_action_space() (in module network_gym_client.envs.network_slicing.adapter)": [[2, "network_gym_client.envs.network_slicing.Adapter.get_action_space"]], "get_observation() (in module network_gym_client.envs.network_slicing.adapter)": [[2, "network_gym_client.envs.network_slicing.Adapter.get_observation"]], "get_observation_space() (in module network_gym_client.envs.network_slicing.adapter)": [[2, "network_gym_client.envs.network_slicing.Adapter.get_observation_space"]], "get_policy() (in module network_gym_client.envs.network_slicing.adapter)": [[2, "network_gym_client.envs.network_slicing.Adapter.get_policy"]], "get_rbg_size() (in module network_gym_client.envs.network_slicing.adapter)": [[2, "network_gym_client.envs.network_slicing.Adapter.get_rbg_size"]], "get_reward() (in module network_gym_client.envs.network_slicing.adapter)": [[2, "network_gym_client.envs.network_slicing.Adapter.get_reward"]], "adapter (class in network_gym_client.envs.nqos_split)": [[3, "network_gym_client.envs.nqos_split.Adapter"]], "get_action_space() (in module network_gym_client.envs.nqos_split.adapter)": [[3, "network_gym_client.envs.nqos_split.Adapter.get_action_space"]], "get_observation() (in module network_gym_client.envs.nqos_split.adapter)": [[3, "network_gym_client.envs.nqos_split.Adapter.get_observation"]], "get_observation_space() (in module network_gym_client.envs.nqos_split.adapter)": [[3, "network_gym_client.envs.nqos_split.Adapter.get_observation_space"]], "get_policy() (in module network_gym_client.envs.nqos_split.adapter)": [[3, "network_gym_client.envs.nqos_split.Adapter.get_policy"]], "get_reward() (in module network_gym_client.envs.nqos_split.adapter)": [[3, "network_gym_client.envs.nqos_split.Adapter.get_reward"]], "netowrk_util() (in module network_gym_client.envs.nqos_split.adapter)": [[3, "network_gym_client.envs.nqos_split.Adapter.netowrk_util"]], "adapter (class in network_gym_client.envs.qos_steer)": [[4, "network_gym_client.envs.qos_steer.Adapter"]], "calculate_wifi_qos_user_num() (in module network_gym_client.envs.qos_steer.adapter)": [[4, "network_gym_client.envs.qos_steer.Adapter.calculate_wifi_qos_user_num"]], "get_action_space() (in module network_gym_client.envs.qos_steer.adapter)": [[4, "network_gym_client.envs.qos_steer.Adapter.get_action_space"]], "get_observation() (in module network_gym_client.envs.qos_steer.adapter)": [[4, "network_gym_client.envs.qos_steer.Adapter.get_observation"]], "get_observation_space() (in module network_gym_client.envs.qos_steer.adapter)": [[4, "network_gym_client.envs.qos_steer.Adapter.get_observation_space"]], "get_policy() (in module network_gym_client.envs.qos_steer.adapter)": [[4, "network_gym_client.envs.qos_steer.Adapter.get_policy"]], "get_reward() (in module network_gym_client.envs.qos_steer.adapter)": [[4, "network_gym_client.envs.qos_steer.Adapter.get_reward"]], "env (class in network_gym_client)": [[5, "network_gym_client.Env"]], "action_space (network_gym_client.env attribute)": [[5, "network_gym_client.Env.action_space"]], "adapter (network_gym_client.env attribute)": [[5, "network_gym_client.Env.adapter"]], "northbound_interface_client (network_gym_client.env attribute)": [[5, "network_gym_client.Env.northbound_interface_client"]], "observation_space (network_gym_client.env attribute)": [[5, "network_gym_client.Env.observation_space"]], "reset() (in module network_gym_client.env)": [[5, "network_gym_client.Env.reset"]], "step() (in module network_gym_client.env)": [[5, "network_gym_client.Env.step"]], "measurementreport (class in network_gym_client)": [[6, "network_gym_client.MeasurementReport"]], "northboundinterface (class in network_gym_client)": [[6, "network_gym_client.NorthboundInterface"]], "connect() (in module network_gym_client.northboundinterface)": [[6, "network_gym_client.NorthboundInterface.connect"]], "process_measurement() (in module network_gym_client.northboundinterface)": [[6, "network_gym_client.NorthboundInterface.process_measurement"]], "recv() (in module network_gym_client.northboundinterface)": [[6, "network_gym_client.NorthboundInterface.recv"]], "send() (in module network_gym_client.northboundinterface)": [[6, "network_gym_client.NorthboundInterface.send"]]}})