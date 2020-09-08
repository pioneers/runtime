import os, tempfile
import threading
from signal import signal, SIGINT
from sys import exit
from pynput import keyboard
from pynput.keyboard import Listener, Key
from time import sleep

file = "gamepad_inputs.fifo"

def handler(signal_received, frame):
    print("Exiting gracefully")
    if (fifo.closed == False):
        fifo.close()
    exit(0)

def setup_fifo():
    if(os.path.exists(file)):
        os.remove(file)
    os.mkfifo(file, 0o660)
    global fifo
    fd = os.open(file, os.O_RDWR)
    fifo = os.fdopen(fd, 'w')

def set_controls():
    controls.add(keyboard.KeyCode(char='u'))
    controls.add(keyboard.KeyCode(char='i'))
    controls.add(keyboard.KeyCode(char='o'))
    controls.add(keyboard.KeyCode(char='p'))

    controls.add(keyboard.KeyCode(char='w'))
    controls.add(keyboard.KeyCode(char='a'))
    controls.add(keyboard.KeyCode(char='d'))
    controls.add(keyboard.KeyCode(char='s'))

    controls.add(keyboard.KeyCode(char='h'))
    controls.add(keyboard.KeyCode(char='j'))
    controls.add(keyboard.KeyCode(char='k'))
    controls.add(keyboard.KeyCode(char='l'))

    controls.add(keyboard.KeyCode(char='f'))
    controls.add(keyboard.KeyCode(char='g'))
    controls.add(keyboard.KeyCode(char='r'))
    controls.add(keyboard.KeyCode(char='t'))

    controls.add(keyboard.KeyCode(char='z'))

    special_keys.add(keyboard.Key.down)
    special_keys.add(keyboard.Key.up)
    special_keys.add(keyboard.Key.right)
    special_keys.add(keyboard.Key.left)

def on_press(key):
    mutex.acquire()
    if key in controls:
        keys_held_down.add(key.char)
    elif key in special_keys:
        if key == keyboard.Key.up:
            keys_held_down.add(":V")

        elif key == keyboard.Key.right:
            keys_held_down.add(":B")

        elif key == keyboard.Key.left:
            keys_held_down.add(":N")

        elif key == keyboard.Key.down:
            keys_held_down.add(":M")
    mutex.release()

def on_release(key):
    mutex.acquire()
    if key in controls:
        keys_held_down.remove(key.char)

    elif key == keyboard.Key.up:
        keys_held_down.remove(":V")

    elif key == keyboard.Key.right:
        keys_held_down.remove(":B")

    elif key == keyboard.Key.left:
        keys_held_down.remove(":N")

    elif key == keyboard.Key.down:
        keys_held_down.remove(":M")

    elif key == keyboard.Key.esc:
        fifo.close()
        return False
    mutex.release()

def keyboard_control():
    writer = threading.Thread(target = write_to_fifo)
    writer.start()
    with Listener(on_press = on_press,on_release = on_release) as listener:
        listener.join()

def write_to_fifo():
    while(True):
        mutex.acquire()
        for key_pressed in keys_held_down:
            fifo.write(key_pressed)
        mutex.release()
        sleep(0.05)


def main():
    signal(SIGINT, handler)

    global controls
    global special_keys
    special_keys = set()
    controls = set()
    setup_fifo()
    global keys_held_down
    keys_held_down = set()
    set_controls()
    global mutex
    mutex = threading.Lock()
    keyboard_control()

if __name__=="__main__":
    main()
