#!/usr/bin/env python3

"""
This script is based on this https://gist.github.com/davidbgk/b10113c3779b8388e96e6d0c44e03a74
and https://gist.github.com/bradmontgomery/2219997
The ngrok process that is started by Runtime when it boots up also starts a server on localhost
which broadcasts useful information about the tunnels that was created locally.
This script simply requests the names of the tunnels and their IP addresses and posts them as a
bare-minimum web page on the local network so that somebody who is on the local network that this
Raspberry Pi is connected to can view the names and IP addresses of the ngrok tunnels on the 
Raspberry Pi without being ssh'ed into the machine. 
The webpage will be at 192.168.0.1xx:8000 if connected to Motherbase in 101.
Accessing 192.168.0.1xx:8000/new will generate a new ngrok session before displayin the addresses.
"""

import requests
import json
import http.server
import socketserver
import time
from http import HTTPStatus
import os

PORT = 8000 # The port at which the webpage is hosted at on localhost
tcpSubstring = "tcp://"

# The ngrok IP addresses
publicTCPUrl = ""
publicSSHUrl = ""

def update_IP_addresses():
    """
    Updates the global variables containing the IP addresses.
    """
    global publicTCPUrl
    global publicSSHUrl
    while True:
        try:
            requestedTcpUrl = 'http://localhost:4040/api/tunnels/tcp' # url for the comms tunnel used for regular tcp data connection with dawn
            reqTCP = requests.get(url=requestedTcpUrl)
            parsedTCPRequest = reqTCP.json()
            publicTCPUrl = 'IP Address: ' + parsedTCPRequest["public_url"].replace(tcpSubstring, '') + '\n'

            requestedSshUrl = 'http://localhost:4040/api/tunnels/ssh' # url for the comms tunnel used for ssh
            reqSSH = requests.get(url=requestedSshUrl)
            parsedSSHRequest = reqSSH.json()
            publicSSHUrl = 'SSH Tunneling: ' + parsedSSHRequest["public_url"].replace(tcpSubstring, '') + '\n'

            break
        except KeyError:
            time.sleep(1)
            continue

# Declare the HTTP request handler when we access the webpage
class Handler(http.server.SimpleHTTPRequestHandler):
    # Helper function that indicates the webpage access is OK and an HTML document will be served
    def _set_headers(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()

    def _html(self):
        """
        Returns an HTML page with the ngrok IP addresses
        """
        content = f"""
        <html>
            <body>
                <h1>Runtime ngrok IP Addresses</h1>
                <p>
                    <b>
                        Put these IP addresses into Dawn to connect to the robot!<br>
                        Refresh this page for the most up-to-date addresses.<br>
                        Add "/new" after {PORT} to generate a new session.
                    </b>
                    <br>
                    <br>
                    {publicTCPUrl}<br>
                    {publicSSHUrl}
                </p>
            </body>
        </html>
        """
        return content.encode("utf8")

    def do_GET(self):
        """
        A GET request will display the IP addresses in an HTML page.
        If the path "/new" is supplied, ngrok will restart first.
        """
        self._set_headers()
        if self.path == "/new": # Restart ngrok
            self.wfile.write(str.encode("Generating a new session..."))
            os.system("sudo systemctl restart ngrok.service")
            time.sleep(5) # Wait for ngrok to properly restart
        update_IP_addresses()
        self.wfile.write(self._html())

# Serve the HTTP webpage
while True:
    try:
        httpd = socketserver.TCPServer(('', PORT), Handler)
        break
    except OSError: # Sometimes we get an "Address already in use" error. Just try again.
        time.sleep(1)
        continue
httpd.serve_forever()
