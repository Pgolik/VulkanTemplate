set shell := ["/usr/bin/bash", "-c"]

alias b := build
alias c := clean
alias r := run

build_dir := "./build/"
source_dir := "."

build:
    #!/usr/bin/env bash
    cmake -S {{source_dir}} -B {{build_dir}} -G Ninja
    cd {{build_dir}}
    ninja

clean:
    rm -r {{build_dir}}*

run:
    {{source_dir}}/artifacts/bin/VulkanTemplate