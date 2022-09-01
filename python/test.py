import pickle
import socket
import json
import sys
import time
print("succesful started!")


#HOST = "127.0.0.1"  # The server's hostname or IP address
#PORT = 65285  # The port used by the server
HOST = "192.168.1.42"  # The server's hostname or IP address
PORT = 8888  # The port used by the server


start, stop, state, network, network_value, reload, info, events = {
    "path": "/actions/start", 
    "motor": 1,
    "steps": 5,
    "velocity": 200
}, {
    "path": "/actions/off"
}, {
    "path": "/actions/state"
}, {
    "path": "/service/settings/network",
    "ssid": "", # paste your values here
    "password": "",
    "ip": "",
    "mask": "",
    "gateway": ""
}, {
    "path": "/service/settings/network/value"
}, {
    "path": "/service/reload"
}, {
    "path": "/service/info"
}, {
    "path": "/events"
    # "destination": "/actions/state"
}
print('HOST', HOST)
print('PORT', PORT)


jsonResult = json.dumps(start)
try:
    sock = socket.socket()
except socket.error as err:
    print('Socket error because of %s' % (err))

try:
    sock.connect((HOST, PORT))
    sock.send(jsonResult.encode())
except socket.gaierror:

    print('There an error resolving the host')

    sys.exit()

print(jsonResult, 'was sent!')
# for x in range(2):
while(True):
    a = sock.recv(1024)
    print(a, 'was recieve!')
    time.sleep(1)
sock.close()
print('Connection terminated')
time.sleep(10)
