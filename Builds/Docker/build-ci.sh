set -e

mkdir -p build/docker/
cp doc/divvyd-example.cfg build/clang.debug/divvyd build/docker/
cp Builds/Docker/Dockerfile-testnet build/docker/Dockerfile
mv build/docker/divvyd-example.cfg build/docker/divvyd.cfg
strip build/docker/divvyd
docker build -t divvy/divvyd:$CIRCLE_SHA1 build/docker/
docker tag divvy/divvyd:$CIRCLE_SHA1 divvy/divvyd:latest

if [ -z "$CIRCLE_PR_NUMBER" ]; then
  docker tag divvy/divvyd:$CIRCLE_SHA1 divvy/divvyd:$CIRCLE_BRANCH
fi
