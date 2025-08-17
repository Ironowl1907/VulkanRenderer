#!/bin/bash

./shaders/slang/build/slang-2025.14.3-linux-x86_64/bin/slangc shaders/shader.slang -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o shaders/slang.spv
