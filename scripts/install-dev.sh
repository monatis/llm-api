#!/bin/bash

# install libasio-dev for crow
sudo apt install libasio-dev

# install websocat to test websocket connection
sudo wget -qO /usr/local/bin/websocat https://github.com/vi/websocat/releases/latest/download/websocat.x86_64-unknown-linux-musl
sudo chmod a+x /usr/local/bin/websocat
