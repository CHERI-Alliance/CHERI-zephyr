# CHERI-Zephyr-v0.3.0
# Copyright (c) 2025 University of Birmingham, Added to support CHERI spec
#
# SPDX-License-Identifier: Apache-2.0
#

#################################################################################
# Stage 1: Build tool image
#################################################################################
FROM cheri-zephyr-west AS cheri-zephyr-devenv

ARG HOME_DIR="/home/user/"

WORKDIR $HOME_DIR

# This will copy as root, but chown to user:user
USER root
COPY --chown=user:user zephyr $HOME_DIR/zephyrproject/zephyr

# Install for non root to avoid permissions warnings
USER user
#-----------------------------------
# Zephyr set up

WORKDIR $HOME_DIR/zephyrproject

RUN . .venv/bin/activate && \
	west init -l zephyr && \
	west update && \
	west zephyr-export

RUN echo ". ./codasip.sh" >> $HOME_DIR/.bashrc
#-----------------------------------

CMD ["bash"]
#################################################################################
