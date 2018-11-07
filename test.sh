#!/bin/bash


sleep 1
kill -SIGUSR1 $(pgrep cat$)
lsof -p $(pgrep hello)
