import importlib
import re
import string
import sys
from collections import defaultdict

""" Student code parser to determine params used """

def get_all_params(code_file):
    code = importlib.import_module(code_file)
    Robot = MockRobot()
    Gamepad = MockGamepad()
    var = [n for n in dir(code) if not n.startswith("_")]
    mod = sys.modules[__name__]
    for v in var:
        setattr(mod, v, getattr(code, v))
    with open(f"{code_file}.py", "r") as f:
        for line in f:
            if "Robot.set_value" in line or "Robot.get_value" in line:
                line = line.lstrip()
                comment = line.find("#")
                if comment != -1:
                    line = line[:comment]
                exec(line)
    return Robot.device_params

class MockRobot:

    def __init__(self):
        self.device_params = defaultdict(set)

    def get_value(self, id, param):
        self.device_params[id].add(param)
        return 0

    def set_value(self, id, param, val):
        self.device_params[id].add(param)

class MockGamepad:
    def get_value(self, param):
        pass

if __name__ == "__main__":
    print(get_all_params(sys.argv[1]))
