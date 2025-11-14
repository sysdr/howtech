#!/bin/bash

echo "🧹 Cleaning up DNS Resolution Delay Demo..."


echo "🛑 Stopping containers..."
docker-compose down -v

echo "🗑️  Removing images..."
docker-compose down --rmi all


echo "✅ Cleanup complete!"
