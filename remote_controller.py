import requests
import argparse
import time

parser = argparse.ArgumentParser(prog="Remote Rbr5ucKy Controller")
parser.add_argument('--ip')
parser.add_argument('file')

args = parser.parse_args()

url = f"http://{args.ip}/s?t="

with open(args.file, "r") as f:
    while True:
        line = f.readline().strip()
        if not line:
            break
        if line.startswith("DELAY"):
            time.sleep(int(line[6:])/1000)
        else:
            req_url = url + line 
            requests.get(req_url)
