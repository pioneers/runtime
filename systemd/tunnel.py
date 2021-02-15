#!/usr/bin/env python3
import requests
import json

requestedUrl = 'http://localhost:4040/api/tunnels/comms' # url for the comms tunnel used for udp forwarding
req = requests.get(url=requestedUrl)
parsedRequest = req.json()
publicUrl = parsedRequest["public_url"]

file = open("temp_ngrok_url.txt", "w")
file.write(publicUrl)
file.close()

