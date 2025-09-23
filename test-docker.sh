#!/bin/bash

# Build and run tests in Alpine Docker container
# Usage: ./test-docker.sh [command]
# Examples:
#   ./test-docker.sh                                    # Run tests
#   ./test-docker.sh npm run eslint                     # Run linter
#   ./test-docker.sh sh                                 # Interactive shell
#   ./test-docker.sh node examples/segfault-json.js     # Run a specific example

set -e

IMAGE_NAME="segfault-raub-test"
COMMAND=${*:-"npm run test-ci"}

echo "Building Docker image for Alpine 3.22 with Node 20..."
docker build -f Dockerfile.test -t $IMAGE_NAME .

echo "Running tests in Alpine container..."
if [ "$1" = "sh" ]; then
    docker run --rm -it $IMAGE_NAME sh
else
    docker run --rm $IMAGE_NAME sh -c "$COMMAND"
fi