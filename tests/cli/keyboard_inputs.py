import os, tempfile
import threading
from signal import signal, SIGINT
from sys import exit
from pynput import keyboard
from pynput.keyboard import Listener, Key
from time import sleep
import socket       # for TCP
import struct       # for TCP encoding

file = "gamepad_inputs.fifo"
# TCP Setup
HOST = '127.0.0.1'
PORT = 5006
ADDRESS = (HOST, PORT)

def connect_tcp():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print('waiting for server')
    sock.connect(ADDRESS)
    print('connected to server')
    return sock

def handler(signal_received, frame):
    print("Exiting gracefully")
    exit(0)


def activate_bit(on):
    bits[on] = "1"

def deactivate_bit(off):
    bits[off] = "0"

def set_controls():
    global controls
    controls = {
        keyboard.KeyCode(char="k") : 0,
        keyboard.KeyCode(char="l") : 1,
        keyboard.KeyCode(char="j") : 2,
        keyboard.KeyCode(char="i") : 3,
        keyboard.KeyCode(char="u") : 4,
        keyboard.KeyCode(char="o") : 5,
        keyboard.KeyCode(char="y") : 6,
        keyboard.KeyCode(char="p") : 7,
        keyboard.KeyCode(char="n") : 8,
        keyboard.KeyCode(char="m") : 9,
        keyboard.KeyCode(char="q") : 10,
        keyboard.KeyCode(char="e") : 11,
        keyboard.KeyCode(char="t") : 12,
        keyboard.KeyCode(char="g") : 13,
        keyboard.KeyCode(char="f") : 14,
        keyboard.KeyCode(char="h") : 15,
        keyboard.KeyCode(char="z") : 16,
        keyboard.KeyCode(char="d") : 17,
        keyboard.KeyCode(char="a") : 18,
        keyboard.KeyCode(char="s") : 19,
        keyboard.KeyCode(char="w") : 20,
        keyboard.Key.left : 21,
        keyboard.Key.right : 22,
        keyboard.Key.down : 23,
        keyboard.Key.up : 24
    }

def on_press(key):
    mutex.acquire()
    if key in controls:
        activate_bit(controls[key])
    mutex.release()

def on_release(key):
    mutex.acquire()
    if key in controls:
        deactivate_bit(controls[key])

    elif key == keyboard.Key.esc:
        mutex.release()
        return False
    mutex.release()

def keyboard_control():
    writer = threading.Thread(target = write_to_socket)
    writer.start()
    with Listener(on_press = on_press,on_release = on_release) as listener:
        listener.join()

def write_to_socket():
    sock = connect_tcp()
    while(True):
        mutex.acquire()
        string_to_send = ''.join(bits)
        print("sending", string_to_send.encode())
        sock.send(string_to_send.encode())
        mutex.release()
        sleep(0.05)


def main():
    signal(SIGINT, handler)
    global bits
    bits = list("00000000000000000000000000000000")
    set_controls()
    global mutex
    mutex = threading.Lock()
    keyboard_control()

if __name__=="__main__":
    main()
