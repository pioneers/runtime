import os
import sys

def main():
    path = "/tmp/gamepad_inputs.fifo"
    with open(path, 'r') as reader:
        print(reader.read())
    fifo.close()
