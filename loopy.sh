#!/bin/bash

IP=192.168.7.60

while true
do
curl "$IP/exec?c=0x01&p1=0x00"; echo

curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x02; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl $IP/exec?c=0x03; echo
curl "$IP/exec?c=0x03"; echo

curl "$IP/exec?c=0x01&p1=0x05"; echo
curl "$IP/exec?c=0x01&p1=0x0A"; echo
curl "$IP/exec?c=0x01&p1=0x0F"; echo
curl "$IP/exec?c=0x01&p1=0x10"; echo
curl "$IP/exec?c=0x01&p1=0x15"; echo
curl "$IP/exec?c=0x01&p1=0x1A"; echo
curl "$IP/exec?c=0x01&p1=0x1F"; echo
curl "$IP/exec?c=0x01&p1=0x20"; echo
curl "$IP/exec?c=0x01&p1=0x2F"; echo
curl "$IP/exec?c=0x03"; echo
curl "$IP/exec?c=0x03"; echo
curl "$IP/exec?c=0x03"; echo
curl "$IP/exec?c=0x01&p1=0x00"; echo

done


