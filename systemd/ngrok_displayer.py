#!/usr/bin/env python3

"""
This script is based on this https://gist.github.com/davidbgk/b10113c3779b8388e96e6d0c44e03a74
The ngrok process that is started by Runtime when it boots up also starts a server on localhost
which broadcasts useful information about the tunnels that was created locally.
This script simply requests the names of the tunnels and their IP addresses and posts them as a
bare-minimum web page on the local network so that somebody who is on the local network that this
Raspberry Pi is connected to can view the names and IP addresses of the ngrok tunnels on the 
Raspberry Pi without being ssh'ed into the machine. 
The webpage will be at 192.168.0.1xx:8000 if connected to Motherbase in 101.
"""

import requests
import json
import http.server
import socketserver
import time
from http import HTTPStatus

recv_data = False
tcpSubstring = "tcp://"

while recv_data == False:
    print(recv_data)
    try:
        requestedTcpUrl = 'http://localhost:4040/api/tunnels/tcp' # url for the comms tunnel used for regular tcp data connection with dawn
        reqTCP = requests.get(url=requestedTcpUrl)
        parsedTCPRequest = reqTCP.json()
        print(parsedTCPRequest)
        publicTCPUrl = 'IP Address: ' + parsedTCPRequest["public_url"].replace(tcpSubstring, '') + '\n'

        requestedUdpUrl = 'http://localhost:4040/api/tunnels/converted_udp' # url for the comms tunnel used for udp forwarding
        reqUDP = requests.get(url=requestedUdpUrl)
        parsedUDPRequest = reqUDP.json()
        publicUDPUrl = 'UDP Tunneling: ' +parsedUDPRequest["public_url"].replace(tcpSubstring, '') + '\n'

        requestedSshUrl = 'http://localhost:4040/api/tunnels/ssh' # url for the comms tunnel used for ssh
        reqSSH = requests.get(url=requestedSshUrl)
        parsedSSHRequest = reqSSH.json()
        publicSSHUrl = 'SSH Tunneling: ' + parsedSSHRequest["public_url"].replace(tcpSubstring, '') + '\n'
        recv_data = True
    except KeyError:
        time.sleep(1)
        continue

class Handler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        self.send_response(HTTPStatus.OK)
        self.end_headers()
        self.wfile.write(str.encode(publicTCPUrl))
        self.wfile.write(str.encode(publicUDPUrl))
        self.wfile.write(str.encode(publicSSHUrl))


httpd = socketserver.TCPServer(('', 8000), Handler)
httpd.serve_forever()
