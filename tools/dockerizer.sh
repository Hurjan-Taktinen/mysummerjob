#!/bin/sh

while test $# -gt 0; do
    case "$1" in
        -h|--help)
            echo "TODO: write help"
            exit 0
            ;;
        -r|--rebuild)
            shift
            REBUILD=1
            ;;
        -n|--name)
            NAME="$1"
            shift
            ;;
        *)
            break
            ;;
    esac
done

# Set default name if not given
[ -z $NAME ] && NAME=mysummerjob

# Rebuild container if --rebuild flag was given
[ $REBUILD ] && docker image build -t $NAME .

OLDPATH=$PWD
cd $(git rev-parse --show-toplevel)

docker run -v $PWD:/app $NAME

cd $OLDPATH
