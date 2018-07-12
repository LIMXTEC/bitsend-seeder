#!/bin/bash
# Copyright (c) 2018 The BitSend BSD Core Developers (dalijolijo)
# Script bsd-seeder-build.sh
#set -e

#
# Define Variables for BSD Seeder
#
NEW_SEED="testing"
VPS_IP="seed.mybitsend.com"
EMAIL="test@test.com"
DNS_PORT="53"
DOCKER_REPO="dalijolijo"
GIT_REPO="dalijolijo"
GIT_PROJECT="bitsend-seeder"
IMAGE_NAME="bitsend-seeder"
CONTAINER_NAME="bitsend-seeder"
CONFIG="./bsd-seeder.conf"

#
# Color definitions
#
RED='\033[0;31m'
GREEN='\033[0;32m'
NO_COL='\033[0m'
BSD_COL='\033[0;34m'

clear
printf "\n\nRUN ${BSD_COL}BITSEND (BSD)${NO_COL} SEEDER IN DOCKER CONTAINER\n"

#
# Check if bsd-seeder.conf already exist.
#
REUSE="No"
if [ -f "$CONFIG" ]
then
        printf "\nSetup Config file"
        printf "\n-----------------"
        printf "\nFound $CONFIG on your system.\n"
        printf "\nDo you want to re-use this existing config file?\n"
        printf "Enter [Y]es or [N]o and Hit [ENTER]: "
        read REUSE
else
        echo "new_seed_hostname=" > ${CONFIG}
        echo "vps_ip=" >> ${CONFIG}
        echo "email_address=" >> ${CONFIG}
        echo "dns_port_number=53" >> ${CONFIG}
fi


if [[ $REUSE =~ "N" ]] || [[ $REUSE =~ "n" ]]; then
        printf "\nEnter your new ${BSD_COL}BitSend${NO_COL} Seed Hostname and Hit [ENTER]: "
        read NEW_SEED
        if [ -z "$NEW_SEED" ]; then
                printf "${RED}No Seed Hostname specified!\n\n${NO_COL}"
                exit 1
        fi
        sed -i "s/^\(new_seed_hostname=\).*/new_seed_hostname=$NEW_SEED/g" ${CONFIG}
        printf "Enter the VPS IP Address for your ${BSD_COL}BitSend${NO_COL} Seeder and Hit [ENTER]: "
        read VPS_IP
        if [ -z "$VPS_IP" ]; then
                printf "${RED}No VPS IP specified!\n\n${NO_COL}"
                exit 1
        fi
        sed -i "s/^\(vps_ip=\).*/vps_ip=$VPS_IP/g" ${CONFIG}
        printf "Enter the E-Mail Address for your ${BSD_COL}BitSend${NO_COL} Seeder and Hit [ENTER]: "
        read EMAIL
        if [ -z "$EMAIL" ]; then
                printf "${RED}No E-Mail Address specified! Will use empty@test.com\n\n${NO_COL}"
                EMAIL="empty@test.com"
        fi
        sed -i "s/^\(email_address=\).*/email_address=$EMAIL/g" ${CONFIG}
        printf "Enter the DNS Port (default 53) for your ${BSD_COL}BitSend${NO_COL} Seeder and Hit [ENTER]: "
        read DNS_PORT
        if [ -z "$DNS_PORT" ]; then
                DNS_PORT="53"
        fi
        sed -i "s/^\(dns_port_number=\).*/dns_port_number=$DNS_PORT/g" ${CONFIG}
else
        source $CONFIG
        NEW_SEED=$(echo $new_seed_hostname)
        VPS_IP=$(echo $vps_ip)
        EMAIL=$(echo $email_address)
        DNS_PORT=$(echo $dns_port_number)
fi

#
# Docker Installation
#
if ! type "docker" > /dev/null; then
  curl -sSL https://get.docker.com | sh
fi

#
# Firewall Setup
#
printf "\nDownload needed Helper-Scripts"
printf "\n------------------------------\n"
wget https://raw.githubusercontent.com/${GIT_REPO}/${GIT_PROJECT}/master/check_os.sh -O check_os.sh
chmod +x ./check_os.sh
source ./check_os.sh
rm ./check_os.sh
wget https://raw.githubusercontent.com/${GIT_REPO}/${GIT_PROJECT}/master/firewall_config.sh -O firewall_config.sh
chmod +x ./firewall_config.sh
source ./firewall_config.sh ${DNS_PORT}
rm ./firewall_config.sh

#
# Run the docker container from BSD Seeder Docker Image
#
printf "\nStart ${BSD_COL}BitSend (BDS)${NO_COL} Seeder Docker Container"
printf "\n-------------------------------------------\n"
sudo docker ps | grep ${CONTAINER_NAME} >/dev/null
if [ $? -eq 0 ]; then
    printf "${RED}Conflict! The container name \'${CONTAINER_NAME}\' is already in use.${NO_COL}\n"
    printf "\nDo you want to stop the running container to start the new one?\n"
    printf "Enter [Y]es or [N]o and Hit [ENTER]: "
    read STOP

    if [[ $STOP =~ "Y" ]] || [[ $STOP =~ "y" ]]; then
        docker stop ${CONTAINER_NAME}
    else
        printf "\nDocker Setup Result"
        printf "\n-------------------\n"
        printf "${RED}Canceled the Docker Setup without starting ${BSD_COL}BitSend (BDS)${NO_COL} Seeder Docker Container.${NO_COL}\n\n"
        exit 1
    fi
fi
docker rm ${CONTAINER_NAME} 2>/dev/null

#
# Run BSD Seeder Docker Container
#
docker run \
 --rm \
 -p ${DNS_PORT}:${DNS_PORT} \
 --detach \
 --name ${CONTAINER_NAME} \
 ${DOCKER_REPO}/${IMAGE_NAME} -h ${NEW_SEED} -n ${VPS_IP} -m ${EMAIL} -p ${DNS_PORT}

#
# Show result and give user instructions
#
sleep 5
clear
printf "\n${BSD_COL}BitSend (BSD)${GREEN} Seeder Docker Solution${NO_COL}\n"
sudo docker ps | grep ${CONTAINER_NAME} >/dev/null
if [ $? -ne 0 ];then
    printf "${RED}Sorry! Something went wrong. :(${NO_COL}\n"
else
    printf "${GREEN}GREAT! Your ${BSD_COL}BitSend (BSD)${GREEN} Seeder Docker is running now! :)${NO_COL}\n"
    printf "\nShow your running Docker Container \'${CONTAINER_NAME}\' with ${GREEN}'docker ps'${NO_COL}\n"
    sudo docker ps | grep ${CONTAINER_NAME}
    printf "\nJump inside the ${BSD_COL}BitSend (BSD)${NO_COL} Seeder Docker Container with ${GREEN}'docker exec -it ${CONTAINER_NAME} bash'${NO_COL}\n"
    printf "\nCheck Log Output of ${BSD_COL}BitSend (BSD)${NO_COL} Seeder with ${GREEN}'docker logs ${CONTAINER_NAME}'${NO_COL}\n"
    printf "${GREEN}HAVE FUN!${NO_COL}\n\n"
fi

