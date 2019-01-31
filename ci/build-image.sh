#!/bin/bash

dockerfile=Dockerfile

while getopts "c:n:f:a:" opt; do
    case $opt in
        c) caches+=("$OPTARG");;
        n) names+=("$OPTARG");;
        a) args+=("$OPTARG");;
        f) dockerfile=("$OPTARG")
        #...
    esac
done

shift $((OPTIND -1))
workdir=$1
build_arguments="--pull -f $dockerfile"

if [ ${#args[@]} -ne 0 ]; then
  build_arguments+="$( printf " --build-arg %s" "${args[@]}" )"
fi

if [ ${#caches[@]} -ne 0 ]; then
  build_arguments+="$( printf " --cache-from %s" "${caches[@]}" )"
fi

if [ ${#names[@]} -ne 0 ]; then
  build_arguments+="$( printf " --tag %s" "${names[@]}" )"
fi

build_arguments+=" $workdir"

for image in "${caches[@]}"; do
  echo "pulling $image as build cache"
  docker pull $image || true
done

echo "building with $build_arguments"
docker build $build_arguments || { echo 'docker build failed' ; exit 1; }

for image in "${names[@]}"; do
  echo "pushing $image"
  docker push $image || { echo 'docker push failed' ; exit 1; }
done
