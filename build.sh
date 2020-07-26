#/usr/bin/env bash

pushd executor && make && popd
printf "Finished compiling EXECUTOR\n"

pushd net_handler && make && popd
printf "Finished compiling NET HANDLER\n"

pushd shm_wrapper && make shm && popd
printf "Finished compiling SHM\n"

pushd shm_wrapper && make static_aux && popd
printf "Finished compiling STATIC_AUX\n"

pushd shm_wrapper && make static_dev && popd
printf "Finished compiling STATIC_DEV\n"

pushd shm_wrapper && make test1 && popd
printf "Finished compiling TEST1\n"

pushd shm_wrapper && make test2 && popd
printf "Finishing compiling TEST2\n"

pushd shm_wrapper && make print && popd
printf "Finishing compiling PRINT\n"

pushd dev_handler && make && popd

printf "Finished Building!!\n"
