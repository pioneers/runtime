#!/usr/bin/env python3
import requests
import json
import http.server
import socketserver
import time
from http import HTTPStatus

recv_data = False

while recv_data == False:
    print(recv_data)
    try:
        requestedTcpUrl = 'http://localhost:4040/api/tunnels/tcp' # url for the comms tunnel used for regular tcp data connection with dawn
        reqTCP = requests.get(url=requestedTcpUrl)
        parsedTCPRequest = reqTCP.json()
        print(parsedTCPRequest)
        publicTCPUrl = 'TCP Port: ' + parsedTCPRequest["public_url"] + '\n'

        requestedUdpUrl = 'http://localhost:4040/api/tunnels/converted_udp' # url for the comms tunnel used for udp forwarding
        reqUDP = requests.get(url=requestedUdpUrl)
        parsedUDPRequest = reqUDP.json()
        publicUDPUrl = 'UDP Port: ' +parsedUDPRequest["public_url"] + '\n'

        requestedSshUrl = 'http://localhost:4040/api/tunnels/ssh' # url for the comms tunnel used for ssh
        reqSSH = requests.get(url=requestedSshUrl)
        parsedSSHRequest = reqSSH.json()
        publicSSHUrl = 'SSH Port: ' + parsedSSHRequest["public_url"] + '\n'
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
