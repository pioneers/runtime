# Dummy program to write pitches to sound.py via TCP
# This should be ran after running sound.py

import socket # for TCP
import struct # for TCP encoding
import time

# The string of notes to play. Whitespace indicates a pause
NOTES = 'CC GG AA G  FF EE DD C'
PAUSE_DURATION = 0.125  # The duration (s) to wait during a whitespace
NOTE_DURATION = 0.75    # The duration that a note is held before playing the next

# TCP Setup
HOST = 'localhost'
PORT = 5000
ADDRESS = (HOST, PORT)

# Mapping notes to pitches
# https://www.szynalski.com/tone-generator/
MAP = {
    'C' : 261,
    'D' : 294,
    'E' : 330,
    'F' : 349,
    'G' : 392,
    'A' : 440,
    'B' : 494,
    ' ' : -1
}

def main():
    # Initialize TCP socket
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Request connection to sound.py
    client.connect(ADDRESS)

    # Write pitches
    for note in NOTES:
        if (note == ' '):
            time.sleep(PAUSE_DURATION)
            continue;
        packet = struct.pack('!f', MAP[note]) # Encode i as a float https://docs.python.org/3/library/struct.html
        bytes_sent = client.send(packet)
        print("Wrote %d bytes" % bytes_sent)
        time.sleep(NOTE_DURATION)

    client.close()

if __name__ == "__main__":
    main()
