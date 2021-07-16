#!/bin/bash

# The build file is build on the mac , if you want to build on the linux,
# it's better to change the build result dylib to .so
# build gflags
function build_gflags() 
{
  local source_dir
  local build_dir

  cd third-party/gflags
  if  [ -e "gflags" ] && [ -e "gflags/build" ];then
    echo "gflags has been builded."
    return
  fi

  source_dir=$(pwd)
  build_dir=$source_dir/c_build

  mkdir -p "$build_dir"/ \
    && cd "$build_dir" \
    && cmake .. -DCMAKE_INSTALL_PREFIX="$build_dir" -DBUILD_SHARED_LIBS=1 -DBUILD_STATIC_LIBS=1 \
    && make \
    && make install \
    && cd ../../../
}

# build tbb
function build_tbb()
{
  local source_dir
  local build_dir

  cd third-party/oneTBB
  if  [ -e "oneTBB" ] && [ -e "gflags/build" ];then
    echo "gflags has been builded."
    return
  fi

  source_dir=$(pwd)
  build_dir=$source_dir/build

  mkdir -p "$build_dir"/ \
    && cd "$build_dir" \
    && cmake .. -DCMAKE_BUILD_TYPE=Release \
    && make -j5 \
    && cd ../../../
}

# build threadpool
function build_threadpool() 
{
  cd third-party/threadpool
  make clean
  make all
  cd ../../
}

current_path=$(pwd)
build_gflags
cd $current_path
build_tbb
cd $current_path
build_threadpool
cd $current_path

make cache_bench
