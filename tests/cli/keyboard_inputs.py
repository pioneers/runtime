import os, tempfile
from pynput import keyboard
from pynput.keyboard import Listener, Key

file = "/tmp/gamepad_inputs.fifo"

def setup_fifo():
    print(file)
    if(os.path.exists(file)):
        print("removing")
        os.remove(file)
    os.mkfifo(file, 0o660)
    global fifo
    print(file)
    fd = os.open(file, os.O_RDWR)
    fifo = os.fdopen(fd, 'w')
    print("FIFO opened")

def set_controls():
    controls.add(keyboard.KeyCode(char='u'))
    controls.add(keyboard.KeyCode(char='i'))
    controls.add(keyboard.KeyCode(char='o'))
    controls.add(keyboard.KeyCode(char='p'))
    controls.add(keyboard.KeyCode(char='w'))
    controls.add(keyboard.KeyCode(char='a'))
    controls.add(keyboard.KeyCode(char='d'))
    controls.add(keyboard.KeyCode(char='s'))

    special_keys.add(keyboard.Key.down)
    special_keys.add(keyboard.Key.up)
    special_keys.add(keyboard.Key.right)
    special_keys.add(keyboard.Key.left)
    print("set up done")

def on_press(key):
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

    for key_pressed in keys_held_down:
        fifo.write(key_pressed)
        print(key_pressed)
def on_release(key):
    if key in controls:
        keys_held_down.remove(key.char)
        print("key released")

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

def keyboard_control():
    print("controlling")
    with Listener(on_press = on_press,on_release = on_release) as listener:
        listener.join()

def main():
    global controls
    global special_keys
    special_keys = set()
    controls = set()

    setup_fifo()

    global keys_held_down
    keys_held_down = set()
    set_controls()
    keyboard_control()

if __name__=="__main__":
    main()
