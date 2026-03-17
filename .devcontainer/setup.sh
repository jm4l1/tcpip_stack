#!/bin/bash
set -eux

rm -rf /root/.bash_history
ln -s /home/user/.realhome/.bash_history /root/.bash_history

rm -rf /root/.ssh
ln -s  /home/user/.realhome/.ssh /root/.ssh
