set shell := ["/usr/bin/bash", "-c"]

alias b := build
alias c := clean
alias r := run

build_dir := "./build/"
source_dir := "."
bin_dir := "./artifacts/bin/"

build:
    #!/usr/bin/env bash
    if [ ! -f {{build_dir}} ]; then
        mkdir {{build_dir}}
    fi
    cmake -S {{source_dir}} -B {{build_dir}} -G Ninja
    cd {{build_dir}}
    ninja

clean:
    rm -r {{build_dir}}*

run:
    {{bin_dir}}VulkanTemplate