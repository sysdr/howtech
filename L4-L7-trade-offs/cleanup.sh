#!/bin/bash

echo "Stopping and removing containers..."
docker-compose down

echo "Cleaning up..."
docker-compose rm -f

echo "Done!"
