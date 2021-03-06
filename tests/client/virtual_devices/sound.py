# Reads pitches from virtual SoundDevice and plays them.
# The virtual SoundDevice is hosted in the Docker container
# This Python script should be ran on the host machine for sound output
# The virtual SoundDevice and this script are connected by a localhost TCP socket
# This acts as the socket server and should be ran before the SoundDevice

import socket       # for TCP
import struct       # for TCP decoding
import time         # for sleep
import numpy as np  # for generating sine wave
import pyaudio      # Third party library for audio

# TCP Setup
HOST = '127.0.0.1'
PORT = 5005
ADDRESS = (HOST, PORT)

# Sound parameters
volume = 0.5     # range [0.0, 1.0]
fs = 44100       # sampling rate, Hz, must be integer
duration = 0.75  # Duration of a note if not interrupted by the next

def main():
    # Get TCP connection
    connection = connect_tcp()

    # Initialize player
    p = pyaudio.PyAudio()
    # Initialize sound stream; for paFloat32 sample values must be in range [-1.0, 1.0]
    stream = p.open(format=pyaudio.paFloat32,
                    channels=1,
                    rate=fs,
                    output=True)

    # Read from socket for pitch; https://stackoverflow.com/a/33914393
    while True:
        # Receive a float
        buf = connection.recv(4)
        if (len(buf) != 4):
            print("No more data")
            break

        # Decode the float
        pitch = struct.unpack('f', buf)[0]
        if (pitch == 0):
            continue
        print("Received", pitch)

        # Play sound
        stream.write(generate_sound(pitch))

    connection.close()
    print("Connection closed.")

    # Clean up PyAudio
    stream.stop_stream()
    stream.close()
    p.terminate()

def connect_tcp():
    """
    Connects to ADDRESS via TCP, blocking until a client is accepted
    Returns:
        the socket that can be received from
    """
    # Create the TCP socket
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Bind the created socket to address
    server.bind(ADDRESS)
    # Listen for incoming SoundDevice connection
    server.listen()
    print("Listening for a client...");
    # Accept the socket connection
    connection, client_address = server.accept()
    print("Connected to", client_address)
    server.close()
    return connection

def generate_sound(pitch):
    """
    Returns a sound wave given a pitch (frequency)
    Arguments:
        pitch: The pitch to play in Hertz (float)
    Returns:
        A sound wave that can be written to a PyAudio stream
    """
    # See https://stackoverflow.com/a/27978895 for generating sound
    samples = (np.sin(2*np.pi*np.arange(fs*duration)*pitch/fs)).astype(np.float32)
    return volume * samples

if __name__ == "__main__":
    main()
