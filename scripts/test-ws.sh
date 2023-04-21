#!/bin/bash

json='{"prompt":"tell me about artificial intelligence in one sentence."}'

# Send a WebSocket message to the server
echo "$json" | websocat ws://localhost:8080/chat|
    jq .token
